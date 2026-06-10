/* eslint-disable max-lines */
/*
 * 集中一个文件：manifest 加载、SW host、content script shim、DNR 规则、
 * IPC handlers。按功能拆分 4 个模块得不偿失，反而让端到端调试更麻烦。
 */
const crypto = require("node:crypto");
const fs = require("node:fs");
const os = require("node:os");
const path = require("node:path");
const { BrowserWindow, ipcMain } = require("electron");

/* M2.5 E-1/E-2 Chrome Manifest V3 扩展兼容层（MVP / PoC）
 *
 * 实现思路：
 *   1. 启动时扫描 userData/extensions/<id>/manifest.json；登记登记信息
 *   2. 将每个 manifest 的 content_scripts / declarative_net_request /
 *      background.service_worker 映射到 Astra 能支持的最小实现：
 *        - content_scripts: session.setPreloads 注入 preload shim，
 *          shim 在 DOMContentLoaded 后把匹配到的 <script> 注入主世界
 *        - DNR: 在 session.webRequest.onBeforeRequest 里匹配 URL 模式，
 *          支持 block / allow / upgradeScheme / redirect
 *        - background: 隐藏 BrowserWindow 加载 SW shim，preload 暴露
 *          chrome.storage / chrome.runtime / chrome.tabs / chrome.dnr 最小命名空间
 *   3. 暴露 IPC 给 Renderer 侧 Extensions 面板：list/enable/disable/installFromFolder/uninstall
 *
 * 这是 PoC 不是完整 CWS 兼容层；但能跑通 Dark Reader / uBlock Lite 最小 smoke。
 * 通过 astra://flags → mv3-extensions 默认关闭；用户手动开启。
 */

const STATE_FILENAME = "astra-extensions-state.json";
const EXTENSIONS_DIRNAME = "extensions";
const EXT_STORAGE_DIRNAME = "extension-storage";

/** @type {Map<string, LoadedExtension>} */
const LOADED_EXTENSIONS = new Map();

/** @typedef {{
 *   id: string;
 *   manifest: Record<string, unknown> & { manifest_version: number; name: string; version: string };
 *   directory: string;
 *   enabled: boolean;
 * }} LoadedExtension
 */

/* --------- 文件系统 / manifest --------- */

function extensionsDir(userDataDir) {
  return path.join(userDataDir, EXTENSIONS_DIRNAME);
}
function ensureExtensionsDir(userDataDir) {
  try { fs.mkdirSync(extensionsDir(userDataDir), { recursive: true }); } catch { /* ignore */ }
}
function stateFilePath(userDataDir) {
  return path.join(userDataDir, STATE_FILENAME);
}
function storageDir(userDataDir) {
  return path.join(userDataDir, EXT_STORAGE_DIRNAME);
}

function readJson(filePath) {
  try { return JSON.parse(fs.readFileSync(filePath, "utf8")); } catch { return null; }
}
function writeJson(filePath, obj) {
  try {
    fs.mkdirSync(path.dirname(filePath), { recursive: true });
    fs.writeFileSync(filePath, JSON.stringify(obj, null, 2));
    return true;
  } catch { return false; }
}
function readState(userDataDir) {
  const raw = readJson(stateFilePath(userDataDir));
  return raw && typeof raw === "object" ? raw : {};
}
function writeState(userDataDir, state) { writeJson(stateFilePath(userDataDir), state); }

function sanitiseId(raw) {
  return String(raw || "")
    .replace(/[^a-z0-9_-]/gi, "-")
    .slice(0, 64) || "extension-" + Date.now().toString(36);
}

function slugify(input) {
  return String(input || "")
    .toLowerCase()
    .replace(/[^a-z0-9]+/g, "-")
    .replace(/^-|-$/g, "") || "unnamed";
}

function isValidManifest(manifest) {
  return Boolean(
    manifest &&
    typeof manifest === "object" &&
    manifest.manifest_version === 3 &&
    typeof manifest.name === "string" &&
    typeof manifest.version === "string"
  );
}

function scanAndLoad(userDataDir) {
  ensureExtensionsDir(userDataDir);
  const state = readState(userDataDir);
  let dirs;
  try {
    dirs = fs.readdirSync(extensionsDir(userDataDir), { withFileTypes: true }).filter((d) => d.isDirectory());
  } catch {
    dirs = [];
  }
  for (const entry of dirs) {
    const dir = path.join(extensionsDir(userDataDir), entry.name);
    const manifest = readJson(path.join(dir, "manifest.json"));
    if (!isValidManifest(manifest)) continue;
    const id = sanitiseId(manifest.id || entry.name);
    const enabled = state[id] && typeof state[id].enabled === "boolean" ? state[id].enabled : true;
    LOADED_EXTENSIONS.set(id, { id, manifest, directory: dir, enabled });
  }
}

/* --------- Renderer side 渲染 列表 --------- */

function listForUi() {
  return Array.from(LOADED_EXTENSIONS.values()).map((ext) => ({
    id: ext.id,
    name: ext.manifest.name,
    version: ext.manifest.version,
    description: typeof ext.manifest.description === "string" ? ext.manifest.description : "",
    enabled: ext.enabled
  }));
}

function setExtensionEnabled(userDataDir, id, enabled) {
  const ext = LOADED_EXTENSIONS.get(id);
  if (!ext) return false;
  ext.enabled = Boolean(enabled);
  const state = readState(userDataDir);
  state[id] = { ...(state[id] || {}), enabled: ext.enabled };
  writeState(userDataDir, state);
  return true;
}

function uninstallExtension(userDataDir, id) {
  const ext = LOADED_EXTENSIONS.get(id);
  if (!ext) return { ok: false, reason: "not-found" };
  try { fs.rmSync(ext.directory, { recursive: true, force: true }); }
  catch (err) { return { ok: false, reason: err && err.message || "rm-error" }; }
  LOADED_EXTENSIONS.delete(id);
  const state = readState(userDataDir);
  delete state[id];
  writeState(userDataDir, state);
  return { ok: true };
}

function copyRecursive(src, dest) {
  const entries = fs.readdirSync(src, { withFileTypes: true });
  for (const entry of entries) {
    const s = path.join(src, entry.name);
    const d = path.join(dest, entry.name);
    if (entry.isDirectory()) {
      try { fs.mkdirSync(d, { recursive: true }); copyRecursive(s, d); } catch { /* ignore */ }
    } else if (entry.isFile()) {
      try { fs.copyFileSync(s, d); } catch { /* ignore */ }
    }
  }
}

function installFromFolder(userDataDir, folderPath) {
  if (typeof folderPath !== "string" || !folderPath) return { ok: false, reason: "bad-path" };
  let stat;
  try { stat = fs.statSync(folderPath); } catch { return { ok: false, reason: "not-found" }; }
  if (!stat.isDirectory()) return { ok: false, reason: "not-directory" };
  const manifest = readJson(path.join(folderPath, "manifest.json"));
  if (!isValidManifest(manifest)) return { ok: false, reason: "invalid-manifest" };
  const id = sanitiseId(manifest.id || slugify(manifest.name));
  const destDir = path.join(extensionsDir(userDataDir), id);
  try { fs.mkdirSync(destDir, { recursive: true }); copyRecursive(folderPath, destDir); }
  catch (err) { return { ok: false, reason: err && err.message || "copy-error" }; }
  LOADED_EXTENSIONS.set(id, { id, manifest, directory: destDir, enabled: true });
  const state = readState(userDataDir);
  state[id] = { ...(state[id] || {}), enabled: true };
  writeState(userDataDir, state);
  return { ok: true, id };
}

/* --------- content script injection via session preloads --------- */

function writeTmp(name, contents) {
  const tmpDir = path.join(os.tmpdir(), "astra-mv3");
  try { fs.mkdirSync(tmpDir, { recursive: true }); } catch { /* ignore */ }
  const filePath = path.join(tmpDir, name);
  try { fs.writeFileSync(filePath, contents); } catch { /* ignore */ }
  return filePath;
}

function shimContentScripts() {
  /** @type {Array<{ extId: string; matches: string[]; fullPath: string; cssPaths: string[] }>} */
  const scripts = [];
  for (const ext of LOADED_EXTENSIONS.values()) {
    if (!ext.enabled) continue;
    const blocks = Array.isArray(ext.manifest.content_scripts) ? ext.manifest.content_scripts : [];
    for (const block of blocks) {
      if (!block || typeof block !== "object") continue;
      const matches = Array.isArray(block.matches) ? block.matches.filter((m) => typeof m === "string") : [];
      const js = Array.isArray(block.js) ? block.js : [];
      const cssPaths = Array.isArray(block.css) ? block.css.filter((c) => typeof c === "string") : [];
      for (const jsFile of js) {
        if (typeof jsFile !== "string") continue;
        const full = path.join(ext.directory, jsFile);
        try { if (!fs.existsSync(full)) continue; } catch { continue; }
        scripts.push({ extId: ext.id, matches, fullPath: full, cssPaths: cssPaths.map((c) => path.join(ext.directory, c)) });
      }
    }
  }
  if (scripts.length === 0) return null;
  const fingerprint = crypto
    .createHash("sha1")
    .update(scripts.map((s) => s.extId + "|" + s.fullPath + "|" + JSON.stringify(s.matches)).join("::"))
    .digest("hex")
    .slice(0, 10);
  const MATCH_GLOB_FN = String(function matchGlobRaw(value, pattern) {
    try {
      const specials = /[.+^${}()|[\]\\]/g;
      const p = String(pattern).replace(specials, "\\$&").replace(/\*/g, ".*");
      return new RegExp("^" + p + "$").test(value);
    } catch { return false; }
  });
  const bundle = `
"use strict";
const fs = require("node:fs");
const SCRIPTS = ${JSON.stringify(scripts)};
const matchGlob = ${MATCH_GLOB_FN};
function anyMatch(value, patterns) {
  if (!Array.isArray(patterns) || patterns.length === 0) return true;
  return patterns.some((p) => matchGlob(value, p));
}
function injectStyles(cssPaths, doc) {
  for (let i = 0; i < cssPaths.length; i++) {
    const p = cssPaths[i];
    try {
      const styles = fs.readFileSync(p, "utf8");
      if (!styles) continue;
      const style = doc.createElement("style");
      style.setAttribute("data-astra-mv3-css", "1");
      style.textContent = styles;
      (doc.head || doc.documentElement).appendChild(style);
    } catch { /* ignore */ }
  }
}
document.addEventListener("DOMContentLoaded", () => {
  const loc = document.location.href;
  const doc = document;
  for (let i = 0; i < SCRIPTS.length; i++) {
    if (!anyMatch(loc, SCRIPTS[i].matches)) continue;
    if (Array.isArray(SCRIPTS[i].cssPaths)) injectStyles(SCRIPTS[i].cssPaths, doc);
    let code = "";
    try { code = fs.readFileSync(SCRIPTS[i].fullPath, "utf8"); } catch { continue; }
    if (!code) continue;
    const s = doc.createElement("script");
    s.textContent = code;
    s.setAttribute("data-astra-mv3-content", SCRIPTS[i].extId);
    (doc.head || doc.documentElement).appendChild(s);
  }
}, { once: true });
`;
  return writeTmp(`astra-mv3-contentshim-${fingerprint}.js`, bundle);
}

function attachContentScripts(session) {
  const shimPath = shimContentScripts();
  try {
    const current = (typeof session.getPreloads === "function" ? session.getPreloads() : []) || [];
    const filtered = current.filter((p) => !p.includes("astra-mv3-contentshim-"));
    session.setPreloads(shimPath ? [...filtered, shimPath] : filtered);
  } catch (err) {
    console.warn("[mv3] setPreloads error:", err && err.message);
  }
}

/* --------- DNR --------- */

/** @type {Map<string, Array<{ id: number; priority?: number; action: Record<string, unknown>; condition: Record<string, unknown> }>>} */
const DNR_RULES = new Map();

function compileDnr() {
  DNR_RULES.clear();
  for (const ext of LOADED_EXTENSIONS.values()) {
    if (!ext.enabled) continue;
    const manifest = ext.manifest;
    if (!manifest || !manifest.declarative_net_request || typeof manifest.declarative_net_request !== "object") continue;
    const resources = Array.isArray(manifest.declarative_net_request.rule_resources) ? manifest.declarative_net_request.rule_resources : [];
    for (const res of resources) {
      if (!res || typeof res !== "object") continue;
      if (res.enabled === false) continue;
      if (typeof res.path !== "string") continue;
      const rules = readJson(path.join(ext.directory, res.path));
      if (!Array.isArray(rules)) continue;
      const bucket = DNR_RULES.get(ext.id) || [];
      for (const rule of rules) {
        if (!rule || typeof rule !== "object") continue;
        bucket.push({
          id: Number(rule.id) || bucket.length + 1,
          priority: Number(rule.priority) || 1,
          action: rule.action && typeof rule.action === "object" ? rule.action : {},
          condition: rule.condition && typeof rule.condition === "object" ? rule.condition : {}
        });
      }
      DNR_RULES.set(ext.id, bucket);
    }
  }
}

function matchCondition(details, condition) {
  if (!condition || typeof condition !== "object") return false;
  const url = String(details.url || "");
  const initiator = String(details.initiator || "");
  const resourceType = String(details.resourceType || "").toLowerCase();
  if (typeof condition.urlFilter === "string") {
    if (!matchGlob(url, condition.urlFilter)) return false;
  }
  if (Array.isArray(condition.initiatorDomains) && condition.initiatorDomains.length > 0) {
    if (!condition.initiatorDomains.some((d) => matchGlob(initiator, d))) return false;
  }
  if (Array.isArray(condition.excludedInitiatorDomains)) {
    if (condition.excludedInitiatorDomains.some((d) => matchGlob(initiator, d))) return false;
  }
  if (Array.isArray(condition.resourceTypes) && condition.resourceTypes.length > 0) {
    if (!condition.resourceTypes.includes(resourceType)) return false;
  }
  return true;
}

function matchGlob(value, pattern) {
  try {
    const p = String(pattern)
      .replace(/[.+^${}()|[\]\\]/g, "\\$&")
      .replace(/\*/g, ".*");
    return new RegExp(p).test(value);
  } catch { return false; }
}

function applyAction(details, action) {
  if (!action || typeof action !== "object") return undefined;
  switch (action.type) {
    case "block":
      return { cancel: true };
    case "redirect": {
      const redirect = action.redirect;
      if (redirect && typeof redirect === "object") {
        if (typeof redirect.url === "string") return { redirectURL: redirect.url };
        if (typeof redirect.extensionPath === "string") return { redirectURL: redirect.extensionPath };
        if (redirect.transform && typeof redirect.transform === "string") return { redirectURL: redirect.transform };
      } else if (typeof redirect === "string") {
        return { redirectURL: redirect };
      }
      return undefined;
    }
    case "upgradeScheme": {
      try {
        const u = new URL(details.url);
        if (u.protocol === "http:") {
          u.protocol = "https:";
          return { redirectURL: u.toString(), statusCode: 307 };
        }
      } catch { /* ignore */ }
      return undefined;
    }
    case "allow":
    case "allowAllRequests":
      return { cancel: false };
    default:
      return undefined;
  }
}

function attachDnr(session) {
  const total = Array.from(DNR_RULES.values()).reduce((sum, rules) => sum + rules.length, 0);
  if (total === 0) return;
  try {
    const webRequest = session.webRequest;
    if (!webRequest || typeof webRequest.onBeforeRequest !== "function") return;
    const listener = (details, callback) => {
      for (const [_extId, rules] of DNR_RULES.entries()) {
        for (const rule of rules) {
          if (matchCondition(details, rule.condition)) {
            const applied = applyAction(details, rule.action);
            if (applied !== undefined) { callback(applied); return; }
          }
        }
      }
      callback({ cancel: false });
    };
    try {
      // Newer Electron supports (filter, listener); older builds accept just listener.
      webRequest.onBeforeRequest({ urls: ["<all_urls>"] }, listener);
    } catch {
      try { webRequest.onBeforeRequest(listener); } catch { /* ignore */ }
    }
  } catch (err) {
    console.warn("[mv3] DNR attach failed:", err && err.message);
  }
}

/* --------- SW Host --------- */

/** @type {Map<string, { window: unknown; startedAt: number }>} */
const SW_HOSTS = new Map();

function writeSwPreload(userDataDir, ext, swScriptPath) {
  const storageFile = path.join(storageDir(userDataDir), `${ext.id}.json`);
  const bundle = `
"use strict";
const fs = require("node:fs");
const { contextBridge, ipcRenderer } = require("electron");

const storageFile = ${JSON.stringify(storageFile)};
function readStorage() { try { return JSON.parse(fs.readFileSync(storageFile, "utf8")); } catch { return {}; } }
function writeStorage(obj) {
  try {
    fs.mkdirSync(require("node:path").dirname(storageFile), { recursive: true });
    fs.writeFileSync(storageFile, JSON.stringify(obj));
  } catch { /* ignore */ }
}
function normalizeKeys(keys) {
  if (keys == null) return null;
  if (typeof keys !== "object") return null;
  return keys;
}
function storageGet(keys) {
  const all = readStorage();
  if (keys == null) return all;
  const arr = Array.isArray(keys) ? keys : Object.keys(keys);
  const defaults = Array.isArray(keys) ? null : keys;
  const res = {};
  for (const k of arr) res[k] = all[k] ?? (defaults ? defaults[k] : undefined);
  return res;
}

const STORAGE_LOCAL_SHIM = {
  get: (keys, cb) => {
    const res = storageGet(normalizeKeys(keys));
    if (typeof cb === "function") queueMicrotask(() => cb(res));
    return Promise.resolve(res);
  },
  set: (items, cb) => {
    const cur = readStorage();
    Object.assign(cur, items || {});
    writeStorage(cur);
    if (typeof cb === "function") queueMicrotask(cb);
    return Promise.resolve();
  },
  remove: (keys, cb) => {
    const cur = readStorage();
    const arr = Array.isArray(keys) ? keys : [keys];
    for (const k of arr) delete cur[k];
    writeStorage(cur);
    if (typeof cb === "function") queueMicrotask(cb);
    return Promise.resolve();
  },
  clear: (cb) => {
    writeStorage({});
    if (typeof cb === "function") queueMicrotask(cb);
    return Promise.resolve();
  }
};

const MANIFEST = ${JSON.stringify(ext.manifest)};
const EXT_ID = ${JSON.stringify(ext.id)};

contextBridge.exposeInMainWorld("chrome", {
  runtime: {
    id: EXT_ID,
    getManifest: () => MANIFEST,
    getURL: (p) => "chrome-extension://" + EXT_ID + "/" + (p || "").replace(/^\\//, ""),
    lastError: null,
    sendMessage: (target, message, options, callback) => {
      const resolved = typeof message === "undefined" ? target : message;
      const cb = typeof callback === "function" ? callback : (typeof options === "function" ? options : (typeof target === "function" ? target : undefined));
      ipcRenderer.send("mv3:runtime-message", { from: EXT_ID, message: resolved });
      if (typeof cb === "function") queueMicrotask(() => cb({ ok: true }));
      return Promise.resolve({ ok: true });
    },
    onMessage: {
      addListener: (listener) => {
        if (typeof listener !== "function") return;
        ipcRenderer.on("mv3:runtime-message", (_e, payload) => {
          try { listener(payload && payload.message, { id: payload && payload.from }, () => {}); }
          catch { /* ignore */ }
        });
      },
      removeListener: () => {}
    }
  },
  storage: {
    local: STORAGE_LOCAL_SHIM,
    sync: {
      get: (keys, cb) => { const r = storageGet(normalizeKeys(keys)); if (typeof cb === "function") queueMicrotask(() => cb(r)); return Promise.resolve(r); },
      set: (items, cb) => STORAGE_LOCAL_SHIM.set(items, cb),
      remove: (keys, cb) => STORAGE_LOCAL_SHIM.remove(keys, cb),
      clear: (cb) => STORAGE_LOCAL_SHIM.clear(cb)
    }
  },
  declarativeNetRequest: {
    getAvailableStaticRuleCount: () => Promise.resolve(1000),
    getEnabledRulesets: () => Promise.resolve([]),
    updateStaticRules: () => Promise.resolve()
  },
  tabs: {
    query: (info, cb) => {
      const tabs = [{ id: 1, url: "", active: true, index: 0, windowId: 0, title: "" }];
      if (typeof cb === "function") queueMicrotask(() => cb(tabs));
      return Promise.resolve(tabs);
    },
    create: (properties, cb) => {
      ipcRenderer.invoke("mv3:create-tab", properties || {}).catch(() => {});
      if (typeof cb === "function") queueMicrotask(() => cb({ id: Date.now() }));
      return Promise.resolve({ id: Date.now() });
    },
    sendMessage: (tabId, message, options, cb) => {
      ipcRenderer.send("mv3:send-message", { tabId, message });
      if (typeof cb === "function") queueMicrotask(cb);
    },
    onUpdated: { addListener: () => {}, removeListener: () => {} }
  }
});

// 执行扩展 background service_worker 脚本
(function runSw() {
  const SCRIPT_PATH = ${JSON.stringify(swScriptPath)};
  try {
    const code = fs.readFileSync(SCRIPT_PATH, "utf8");
    const fn = new Function("chrome", "self", "globalThis", "window", code);
    fn(chrome, globalThis, globalThis, globalThis);
  } catch (err) {
    console.error("[mv3-sw]", EXT_ID, err && err.stack || err);
  }
})();
`;
  return writeTmp(`astra-mv3-sw-${ext.id}.js`, bundle);
}

function startServiceWorkerHosts(userDataDir) {
  stopServiceWorkerHosts();
  for (const ext of LOADED_EXTENSIONS.values()) {
    if (!ext.enabled) continue;
    const manifest = ext.manifest;
    const sw = manifest.background && typeof manifest.background === "object" && typeof manifest.background.service_worker === "string"
      ? manifest.background.service_worker
      : null;
    if (!sw) continue;
    const scriptPath = path.join(ext.directory, sw);
    try { if (!fs.existsSync(scriptPath)) continue; } catch { continue; }
    const preloadPath = writeSwPreload(userDataDir, ext, scriptPath);
    let win;
    try {
      win = new BrowserWindow({
        show: false,
        width: 1,
        height: 1,
        webPreferences: {
          preload: preloadPath,
          partition: `persist:astra-sw-${ext.id}`,
          contextIsolation: true,
          sandbox: false,
          webviewTag: false,
          nodeIntegration: false,
          backgroundThrottling: false
        }
      });
      void win.loadURL(`data:text/html;charset=utf-8,<title>SW host ${ext.manifest.name}</title>`).catch(() => {});
      SW_HOSTS.set(ext.id, { window: win, startedAt: Date.now() });
    } catch (err) {
      console.warn("[mv3] SW host failed:", ext.id, err && err.message);
    }
  }
}

function stopServiceWorkerHosts() {
  for (const host of SW_HOSTS.values()) {
    try { if (host.window && typeof host.window.destroy === "function") host.window.destroy(); }
    catch { /* ignore */ }
  }
  SW_HOSTS.clear();
}

/* --------- 统一 attach session： content scripts + DNR --------- */

function attachSession(session) {
  if (!session) return;
  attachContentScripts(session);
  attachDnr(session);
}

function attachAllSessions() {
  const sessions = new Set();
  try {
    sessions.add(require("electron").session.defaultSession);
    for (const win of BrowserWindow.getAllWindows()) {
      try {
        if (win && !win.isDestroyed() && win.webContents) {
          sessions.add(win.webContents.session);
          for (const child of (win.webContents.getAllWebContents && win.webContents.getAllWebContents()) || []) {
            if (child && typeof child.isDestroyed === "function" && !child.isDestroyed() && child.session) sessions.add(child.session);
          }
        }
      } catch { /* ignore */ }
    }
  } catch { /* ignore */ }
  for (const s of sessions) attachSession(s);
}

/* --------- IPC handlers --------- */

function installIpc(userDataDir) {
  ipcMain.handle("mv3:list-extensions", () => listForUi());
  ipcMain.handle("mv3:enable-extension", (_e, id, enabled) => {
    const ok = setExtensionEnabled(userDataDir, id, enabled);
    if (ok) reload(userDataDir);
    return ok;
  });
  ipcMain.handle("mv3:uninstall-extension", (_e, id) => {
    const res = uninstallExtension(userDataDir, id);
    if (res.ok) reload(userDataDir);
    return res;
  });
  ipcMain.handle("mv3:install-from-folder", (_e, folderPath) => {
    const res = installFromFolder(userDataDir, folderPath);
    if (res.ok) reload(userDataDir);
    return res;
  });
  ipcMain.handle("mv3:pick-folder-for-install", async (event) => {
    try {
      const { dialog } = require("electron");
      const win = require("electron").BrowserWindow.fromWebContents(event.sender);
      const result = await dialog.showOpenDialog(win, {
        title: "Select unpacked extension directory (manifest.json folder)",
        properties: ["openDirectory"],
        buttonLabel: "Install"
      });
      if (result.canceled || !Array.isArray(result.filePaths) || result.filePaths.length === 0) {
        return { canceled: true };
      }
      const picked = result.filePaths[0];
      const installResult = installFromFolder(userDataDir, picked);
      if (installResult.ok) reload(userDataDir);
      return { canceled: false, folder: picked, ...installResult };
    } catch (err) {
      return { canceled: false, ok: false, reason: err && err.message || "picker-error" };
    }
  });
  ipcMain.handle("mv3:create-tab", (_e, properties) => {
    try {
      // 让 renderer 决定新 tab：用 main-action broadcast 新 tab
      const { BrowserWindow: BW } = require("electron");
      for (const win of BW.getAllWindows()) {
        try { if (win && !win.isDestroyed()) win.webContents.send("main-action", "new-tab", []); } catch { /* ignore */ }
      }
      if (properties && typeof properties.url === "string") {
        for (const win of BW.getAllWindows()) {
          try { if (win && !win.isDestroyed()) win.webContents.send("open-url-in-new-tab", properties.url); } catch { /* ignore */ }
        }
      }
    } catch { /* ignore */ }
    return true;
  });
}

/* --------- 入口：受 astra://flags.mv3-extensions 控制 --------- */

let BOOTSTRAPPED = false;

function bootstrap(userDataDir, flags) {
  if (BOOTSTRAPPED) return;
  BOOTSTRAPPED = true;
  if (!flags || !flags["mv3-extensions"]) return;
  scanAndLoad(userDataDir);
  compileDnr();
  startServiceWorkerHosts(userDataDir);
  installIpc(userDataDir);
  // Attach 到 defaultSession；后续 web-contents-created 里会再 attach
  try { attachAllSessions(); } catch { /* ignore */ }
}

function reload(userDataDir) {
  try {
    compileDnr();
    startServiceWorkerHosts(userDataDir);
    attachAllSessions();
  } catch (err) {
    console.warn("[mv3] reload error:", err && err.message);
  }
}

module.exports = {
  attachSession,
  attachAllSessions,
  bootstrap,
  compileDnr,
  installFromFolder,
  installIpc,
  listExtensions: listForUi,
  listForUi,
  reload,
  resetLoadedRegistry: () => { LOADED_EXTENSIONS.clear(); DNR_RULES.clear(); stopServiceWorkerHosts(); },
  scanAndLoad,
  setEnabled: (userDataDir, id, enabled) => setExtensionEnabled(userDataDir, id, enabled),
  startServiceWorkerHosts,
  stopServiceWorkerHosts,
  uninstallExtension
};
