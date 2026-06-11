/* eslint-disable max-lines */
// main.js is the Electron entry point: window lifecycle + menu + diagnostics
// + IPC + protocol installation. Splitting it further would scatter the
// cross-cutting concerns (window/session) without real clarity.
const { app, BrowserWindow, session } = require("electron");
const fs = require("node:fs");
const path = require("node:path");
const { installWindowDiagnostics, toggleDevTools } = require("./diagnostics");
const { installIpcHandlers } = require("./ipcHandlers");
const { installApplicationMenu } = require("./appMenu");
const { installAstraProtocol } = require("./astraProtocol");
const { installPwaListeners } = require("./pwaInstall");
const { loadFlags } = require("./astraProtocol");
const mv3Extensions = require("./mv3Extensions");
const { initUpdater } = require("./autoUpdaterService");
const WindowRegistry = require("./windowRegistry");

/* M2.5 E-10: register astra:// protocol (newtab + flags).
 *   - astra://app -> renderer SPA
 *   - astra://newtab -> newtab
 *   - astra://flags -> experimental features
 * Must be installed before app.whenReady fires.
 */
installAstraProtocol();

const APP_ORIGIN = "astra://app";
const DEV_SERVER_URL = process.env.VITE_DEV_SERVER_URL;
const PERMISSION_TIMEOUT_MS = 30_000;
const windows = new Set();
const bridgedSessions = new WeakSet();
const sessionPartitions = new WeakMap();
const permissionCallbacks = new Map();
const permissionRules = new Map();
const downloadItems = new Map();

function windowStatePath() {
  return path.join(app.getPath("userData"), "window-state.json");
}

function loadWindowStates() {
  try {
    const raw = fs.readFileSync(windowStatePath(), "utf-8");
    const data = JSON.parse(raw);
    return Array.isArray(data.windows) ? data.windows : [];
  } catch {
    return [];
  }
}

function saveWindowStates() {
  const snapshot = WindowRegistry.serialize();
  try {
    fs.writeFileSync(windowStatePath(), JSON.stringify({ windows: snapshot }));
  } catch {
    /* userData may not be writable — non-fatal */
  }
}

function createWindow(saved, opts) {
  const win = new BrowserWindow({
    x: saved?.isMaximized ? undefined : saved?.x,
    y: saved?.isMaximized ? undefined : saved?.y,
    width: saved?.width ?? 1440,
    height: saved?.height ?? 960,
    minWidth: 960,
    minHeight: 640,
    title: "Astra Browser",
    titleBarStyle: "hiddenInset",
    backgroundColor: "#111318",
    webPreferences: {
      preload: path.join(__dirname, "preload.js"),
      contextIsolation: true,
      nodeIntegration: false,
      plugins: true,
      webviewTag: true
    }
  });

  if (saved?.isMaximized) win.maximize();
  if (saved?.isFullScreen) win.setFullScreen(true);
  windows.add(win);
  installWindowDiagnostics(win);
  /* ADR-0005 方案 A：把新窗口注册到 WindowRegistry。
   * 默认 Space ID = 最后一个 saved 的 activeSpaceId 或传入的 defaultSpaceId，
   * 找不到时回退到 workspace[0]（renderer 启动时会再次 setActiveSpace 修正）。
   */
  const defaultSpaceId = opts?.defaultSpaceId ?? saved?.activeSpaceId ?? "workspace-0";
  WindowRegistry.registerWindow(win, {
    defaultSpaceId,
    restoredActiveSpaceId: saved?.activeSpaceId,
    restoredSpaceFocus: saved?.spaceFocus
  });

  win.on("closed", () => {
    windows.delete(win);
    saveWindowStates();
  });
  // saveWindowStates 只负责把 registry.serialize() 写盘（调用 WindowRegistry.serialize）
  // bounds 事件由 WindowRegistry 内部 throttled persist 触发写盘。这里保留老的 closed 触发。
  // 旧的 win.on("resize/move/maximize/...") 已被 WindowRegistry.registerWindow 接管
  // schedulePersist()，避免双重节流。

  installSessionBridge(session.defaultSession, "default");
  if (DEV_SERVER_URL) {
    win.loadURL(DEV_SERVER_URL + (opts?.defaultActiveTabId ? `#tab=${opts.defaultActiveTabId}` : ""));
  } else {
    win.loadFile(path.join(__dirname, "../../dist/renderer/index.html"),
      opts?.defaultActiveTabId ? { hash: `tab=${opts.defaultActiveTabId}` } : undefined);
  }
  return win;
}

function installSessionBridge(targetSession, partition = "default") {
  if (!targetSession || bridgedSessions.has(targetSession)) {
    if (targetSession && (partition !== "default" || !sessionPartitions.has(targetSession))) {
      sessionPartitions.set(targetSession, partition);
    }
    return;
  }

  if (partition !== "default" || !sessionPartitions.has(targetSession)) {
    sessionPartitions.set(targetSession, partition);
  }
  bridgedSessions.add(targetSession);
  applyForceHttpsToSession(targetSession, false);
  targetSession.setPermissionRequestHandler((webContents, permission, callback, details) => {
    const partition = sessionPartitions.get(targetSession) ?? "default";
    const requestingUrl = details.requestingUrl || webContents.getURL();
    const origin = getRequestOrigin(requestingUrl);
    if (!origin) {
      callback(false);
      return;
    }

    const rule = permissionRules.get(getPermissionKey(partition, origin, permission));
    if (rule) {
      callback(rule === "allow");
      return;
    }

    const id = `${Date.now()}-${Math.random().toString(16).slice(2)}`;
    const timeout = setTimeout(() => resolvePermissionRequest(id, false), PERMISSION_TIMEOUT_MS);
    permissionCallbacks.set(id, { callback, timeout });
    broadcastPermissionRequest({ id, origin, partition, permission, requestingUrl });
  });

  targetSession.on("will-download", (_event, item) => {
    const id = `${Date.now()}-${Math.random().toString(16).slice(2)}`;
    const startedAt = Date.now();
    let hasEmitted = false;
    downloadItems.set(id, item);
    const buildPayload = (stateOverride) => {
      const baseState = stateOverride ?? "progressing";
      const isPaused = baseState === "progressing" && item.isPaused?.();
      return {
        id,
        filename: item.getFilename(),
        totalBytes: item.getTotalBytes(),
        receivedBytes: item.getReceivedBytes(),
        savePath: item.getSavePath() || "",
        state: isPaused ? "paused" : baseState,
        canPause: baseState === "progressing" ? Boolean(item.canPause?.()) : false,
        url: item.getURL?.() || "",
        startedAt
      };
    };
    const tryEmit = (stateOverride) => {
      const savePath = item.getSavePath();
      if (!savePath && !stateOverride) return;
      const payload = buildPayload(stateOverride);
      if (stateOverride === "completed" || stateOverride === "cancelled" || stateOverride === "interrupted") {
        broadcastDownload({ ...payload, finishedAt: Date.now() });
      } else {
        broadcastDownload(payload);
      }
      hasEmitted = true;
      if (stateOverride === "completed" || stateOverride === "cancelled" || stateOverride === "interrupted") {
        downloadItems.delete(id);
      }
    };

    // Kick off an emit as soon as the save path is available; if the user
    // dismisses the dialog without picking a location, updated/done will
    // still fire later.
    const waitForPathInterval = setInterval(() => {
      if (item.getSavePath()) {
        clearInterval(waitForPathInterval);
        if (!hasEmitted) tryEmit();
      }
    }, 50);

    item.on("updated", (_updatedEvent, state) => {
      tryEmit(state);
    });

    item.once("done", (_doneEvent, state) => {
      clearInterval(waitForPathInterval);
      tryEmit(state);
    });
  });
  /* M2.5 E-1/E-2：如果 mv3 开关启用，给新 session 注入 content scripts + DNR。 */
  mv3Extensions.attachSession(targetSession);
}

function broadcastPermissionRequest(payload) {
  for (const win of windows) {
    if (!win.isDestroyed()) {
      win.webContents.send("permission-request", payload);
    }
  }
}

function broadcastDownload(payload) {
  for (const win of windows) {
    if (!win.isDestroyed()) {
      win.webContents.send("download-event", payload);
    }
  }
}

function resolvePermissionRequest(id, allowed) {
  const pending = permissionCallbacks.get(id);
  if (!pending) {
    return;
  }

  clearTimeout(pending.timeout);
  permissionCallbacks.delete(id);
  pending.callback(Boolean(allowed));
}

function getRequestOrigin(value) {
  try {
    const url = new URL(value);
    return ["http:", "https:"].includes(url.protocol) ? url.origin : null;
  } catch {
    return null;
  }
}

function getPermissionKey(partition, origin, permission) {
  return `${partition}:${origin}:${permission}`;
}

installIpcHandlers({
  downloadItems,
  getPermissionKey,
  installSessionBridge,
  permissionRules,
  resolvePermissionRequest,
  toggleDevTools
});

/* M2.4 W-3 PWA install: beforeinstallprompt capture + standalone launcher.
 * Broadcast helper fans events out to all renderer windows so whichever
 * topbar has the matching active tab can surface the install affordance.
 */
function sendToAll(channel, payload) {
  for (const win of windows) {
    try {
      if (!win.isDestroyed()) win.webContents.send(channel, payload);
    } catch { /* ignore */ }
  }
}
installPwaListeners({ sendToAll });

/* M2.5 W-10 自动更新：electron-updater 初始化（仅 isPackaged 环境启用）。 */
initUpdater({ sendToAll, allowPrerelease: false });

/* M2.5 E-1/E-2 MV3 扩展兼容层（PoC，受 astra://flags 控制）。
 * 启用后在每个 session 上挂 content_scripts + DNR，并为有
 * background.service_worker 的扩展启动隐藏窗口的 SW host。
 */
mv3Extensions.bootstrap(app.getPath("userData"), loadFlags());

/* M2.2 K-1 force HTTPS：在 session 创建时立即注入拦截器。
 *   因为 forceHttpsEnabled 默认 false，首次 attach 是 no-op；后续
 *   renderer 通过 "sync-force-https" IPC 切换状态时 ipcHandlers 会再
 *   次对所有安装的 session 统一重新 apply。
 */
const { applyForceHttpsToSession } = require("./ipcHandlers");

app.whenReady().then(() => {
  installApplicationMenu();
  /* ADR-0005 / W-1: init window registry + persistence + IPC + createWindow factory */
  WindowRegistry.setCreateWindowFn((opts) => createWindow(null, opts));
  WindowRegistry.installIpc();
  WindowRegistry.installPersistence((snapshot) => {
    try {
      fs.writeFileSync(windowStatePath(), JSON.stringify({ windows: snapshot }));
    } catch { /* non-fatal */ }
  });
  const saved = loadWindowStates();
  if (saved.length > 0) {
    saved.forEach((s) => createWindow(s));
  } else {
    createWindow(null);
  }

  app.on("activate", () => {
    if (BrowserWindow.getAllWindows().length === 0) {
      createWindow(null);
    }
  });
});

app.on("new-window-requested", () => {
  createWindow(null);
});

app.on("before-quit", saveWindowStates);

app.on("window-all-closed", () => {
  if (process.platform !== "darwin") {
    app.quit();
  }
});

app.on("web-contents-created", (_event, contents) => {
  installSessionBridge(contents.session);

  contents.setWindowOpenHandler(({ url }) => {
    // Internal astra:// URLs are opened directly by Electron; everything
    // else gets routed to the renderer as a "open URL in a new tab" request
    // so that Ctrl+click / <a target="_blank"> inside any webview opens a
    // new Astra tab instead of jumping to the system default browser.
    if (url.startsWith(APP_ORIGIN)) {
      return { action: "allow" };
    }

    for (const win of windows) {
      if (!win.isDestroyed()) {
        win.webContents.send("open-url-in-new-tab", url);
      }
    }

    return { action: "deny" };
  });

  contents.on("will-navigate", (event, url) => {
    if (contents.getType() === "webview" || url.startsWith(APP_ORIGIN)) {
      return;
    }

    event.preventDefault();
  });
});
