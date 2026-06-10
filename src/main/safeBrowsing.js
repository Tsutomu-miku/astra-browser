/**
 * Safe Browsing MVP (PRD §3.7 K-6 / D-3).
 *
 * 三层检查，按顺序执行，任何一层命中即 block：
 *   1) 内置离线黑名单 hash 表（域名级）—— 无网络环境下也能阻断。
 *   2) SHA256(origin) 远程 lookup（占位，CI 环境变量 SA_BROWSING_API_URL
 *      未配置时跳过）—— 用于同步 Google Safe Browsing / PhishTank / 自建库。
 *   3) 下载内容类型 / 扩展名 危险黑名单（.exe/.bat/.ps1/.scr 等 + 混淆扩展名）。
 *
 * 主进程 IPC：
 *   - "safe-browsing:check-navigation"：will-navigate 同步检查，返回 decision
 *   - "safe-browsing:check-download"：will-download 检查，返回 decision
 *   - "safe-browsing:sync-settings"：从 BrowserState.settings.safeBrowsingEnabled 同步开关
 *
 * 渲染端：拦截命中 → PermissionPrompt 风格的 SafeBrowsingPrompt 展示，
 * 用户可选择 "返回安全页" 或 "继续访问（一次性）"。
 */

const { URL } = require("node:url");
const crypto = require("node:crypto");

/* 离线黑名单：示例恶意域名（仅供 MVP 使用，生产替换为 GSB Lookup API）。 */
const OFFLINE_BLACKLIST = new Set([
  "malware-test.example",
  "phishing-test.example",
  "fake-update.badactor.example",
  "tech-support-scam.example",
  "unwanted-software.example"
]);

/* 危险扩展名：D-3 阻断下载的常见恶意投递类型。 */
const DANGEROUS_EXTENSIONS = new Set([
  ".exe", ".msi", ".bat", ".cmd", ".ps1", ".scr", ".pif", ".com",
  ".cpl", ".jar", ".wsf", ".wsh", ".jse", ".vbs", ".vbe",
  ".hta", ".reg", ".dll", ".xll", ".xlsm", ".docm", ".dotm"
]);

let enabled = true;
let remoteLookupUrl = process.env.SA_BROWSING_API_URL || "";

const remoteCache = new Map();
const CACHE_TTL_MS = 60 * 60 * 1000;

function hashOrigin(origin) {
  return crypto.createHash("sha256").update(origin).digest("hex");
}

function extractHost(url) {
  try {
    return new URL(url).hostname;
  } catch {
    return null;
  }
}

function extractOrigin(url) {
  try {
    return new URL(url).origin;
  } catch {
    return null;
  }
}

function isHttpUrl(url) {
  try {
    const p = new URL(url).protocol;
    return p === "http:" || p === "https:";
  } catch {
    return false;
  }
}

function checkOfflineBlacklist(url) {
  const host = extractHost(url);
  if (!host) return { hit: false };
  const lowered = host.toLowerCase();
  if (OFFLINE_BLACKLIST.has(lowered)) {
    return { hit: true, kind: "blacklisted-domain", severity: "high" };
  }
  // 父域名也查一遍（子域名钓鱼 a.phishing-test.example → 匹配）
  const parts = lowered.split(".");
  for (let i = 0; i < parts.length - 1; i++) {
    const parent = parts.slice(i).join(".");
    if (OFFLINE_BLACKLIST.has(parent)) {
      return { hit: true, kind: "blacklisted-suffix", severity: "high" };
    }
  }
  return { hit: false };
}

async function checkRemote(url, { timeoutMs = 2000 } = {}) {
  if (!remoteLookupUrl) return { hit: false };
  const origin = extractOrigin(url);
  if (!origin) return { hit: false };
  const digest = hashOrigin(origin);
  const cached = remoteCache.get(digest);
  if (cached && Date.now() - cached.ts < CACHE_TTL_MS) {
    return cached.result;
  }
  try {
    const ctrl = new AbortController();
    const timer = setTimeout(() => ctrl.abort(), timeoutMs);
    const res = await fetch(`${remoteLookupUrl}?hash=${digest}`, { signal: ctrl.signal });
    clearTimeout(timer);
    if (!res.ok) return { hit: false };
    const payload = await res.json();
    const result = payload?.match
      ? { hit: true, kind: payload.kind || "remote-lookup", severity: payload.severity || "high" }
      : { hit: false };
    remoteCache.set(digest, { ts: Date.now(), result });
    return result;
  } catch {
    return { hit: false };
  }
}

function checkDangerousDownload(filename) {
  if (!filename) return { hit: false };
  const lowered = filename.toLowerCase();
  // 双扩展名混淆：invoice.pdf.exe
  const ext = lowered.match(/(\.[a-z0-9]{1,8})$/);
  const allExts = [...lowered.matchAll(/(\.[a-z0-9]{1,8})/g)].map((m) => m[1]);
  if (ext && DANGEROUS_EXTENSIONS.has(ext[1])) {
    const obfuscated = allExts.length >= 2;
    return {
      hit: true,
      kind: obfuscated ? "double-extension" : "dangerous-extension",
      severity: obfuscated ? "high" : "medium"
    };
  }
  return { hit: false };
}

async function checkNavigation(url) {
  if (!enabled) return { allowed: true };
  if (!isHttpUrl(url)) return { allowed: true };
  const offline = checkOfflineBlacklist(url);
  if (offline.hit) {
    return { allowed: false, reason: offline.kind, severity: offline.severity, url };
  }
  const remote = await checkRemote(url);
  if (remote.hit) {
    return { allowed: false, reason: remote.kind, severity: remote.severity, url };
  }
  return { allowed: true };
}

async function checkDownload({ url, filename }) {
  if (!enabled) return { allowed: true };
  const nav = await checkNavigation(url);
  if (!nav.allowed) return nav;
  const danger = checkDangerousDownload(filename);
  if (danger.hit) {
    return { allowed: false, reason: danger.kind, severity: danger.severity, url, filename };
  }
  return { allowed: true };
}

function setEnabled(next) {
  enabled = Boolean(next);
}

function setRemoteLookupUrl(url) {
  remoteLookupUrl = typeof url === "string" ? url : "";
}

module.exports = {
  // external interface used by main / ipcHandlers
  checkDownload,
  checkNavigation,
  setEnabled,
  setRemoteLookupUrl,
  // exported for tests
  OFFLINE_BLACKLIST,
  DANGEROUS_EXTENSIONS,
  checkDangerousDownload,
  checkOfflineBlacklist
};
