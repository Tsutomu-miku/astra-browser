const { BrowserWindow, session } = require("electron");
const path = require("node:path");

/* M2.2 K-1 Force HTTPS 主实现。
 *
 * 所有已知 session 统一通过 applyForceHttpsToSession 安装/卸载 webRequest 拦截器：
 *   - http:// 非本地/私网请求 307 内部重定向到 https://
 *   - localhost、.local、.internal、192.168.*、10.*、172.16-31.*、169.254.* 回落 http
 *   - HSTS 行为的轻量实现，没有预加载表，用 307 替换 301 避免缓存锁死
 */

const FORCE_HTTPS_LISTENERS = new WeakMap();

function isLocalhostPrivateHost(hostname) {
  if (!hostname) return true;
  const lowered = hostname.toLowerCase();
  if (lowered === "localhost" || lowered.endsWith(".localhost")) return true;
  if (lowered === "0.0.0.0" || lowered === "127.0.0.1") return true;
  if (lowered.endsWith(".local") || lowered.endsWith(".internal")) return true;
  if (/^(192\.168\.|10\.|172\.(1[6-9]|2[0-9]|3[0-1])\.|169\.254\.)/.test(lowered)) return true;
  return lowered === "[::1]" || lowered === "::1";
}

/**
 * 给 targetSession 切换 Force HTTPS 拦截器。
 *   - enabled=true  安装
 *   - enabled=false 尝试清理
 */
function applyForceHttpsToSession(targetSession, enabled) {
  if (!targetSession) return;
  const webRequest = targetSession.webRequest;
  if (!webRequest || typeof webRequest.onBeforeRequest !== "function") return;
  const existing = FORCE_HTTPS_LISTENERS.get(targetSession);
  if (enabled) {
    if (existing) return;
    const listener = (details, callback) => {
      try {
        const parsed = new URL(details.url);
        if (parsed.protocol !== "http:") return callback({ cancel: false });
        if (isLocalhostPrivateHost(parsed.hostname)) return callback({ cancel: false });
        parsed.protocol = "https:";
        return callback({ redirectURL: parsed.toString(), statusCode: 307 });
      } catch {
        return callback({ cancel: false });
      }
    };
    try {
      webRequest.onBeforeRequest({ urls: ["<all_urls>"] }, listener);
    } catch {
      try {
        webRequest.onBeforeRequest(listener);
      } catch {
        return;
      }
    }
    FORCE_HTTPS_LISTENERS.set(targetSession, listener);
  } else if (existing) {
    try {
      webRequest.onBeforeRequest(null);
    } catch {
      /* ignore removal errors on older builds */
    }
    FORCE_HTTPS_LISTENERS.delete(targetSession);
  }
}

/**
 * 打开一个一次性会话的 Guest 窗口（K-12，区别于无痕：
 *   - 无痕：用户主账号的 in-memory session，关闭窗口即清空，下次同无痕继续用同一 in-memory session
 *   - Guest：全新独立 in-memory session，每个窗口 partition 独立，关闭彻底销毁
 */
function openGuestWindow(parentBounds) {
  const partition = `in-memory:astra-guest-${Date.now().toString(36)}`;
  const guest = new BrowserWindow({
    width: parentBounds?.width ?? 1200,
    height: parentBounds?.height ?? 800,
    x: parentBounds ? parentBounds.x + 40 : undefined,
    y: parentBounds ? parentBounds.y + 40 : undefined,
    backgroundColor: "#121212",
    title: "Astra (Guest)",
    webPreferences: {
      preload: path.join(__dirname, "preload.js"),
      partition,
      contextIsolation: true,
      plugins: true,
      sandbox: false,
      webviewTag: true
    }
  });
  const DEV_SERVER_URL = process.env.VITE_DEV_SERVER_URL;
  if (DEV_SERVER_URL) {
    guest.loadURL(DEV_SERVER_URL + "#?mode=guest");
  } else {
    guest.loadFile(path.join(__dirname, "../../dist/renderer/index.html"), {
      hash: "?mode=guest"
    });
  }
  require("./diagnostics").installWindowDiagnostics(guest);
  guest.on("closed", () => {
    try {
      guest.webContents.session.clearCache();
      guest.webContents.session.clearStorageData();
    } catch {
      /* best-effort cleanup */
    }
  });
}

function getInstalledSessions() {
  const sessions = new Set([session.defaultSession]);
  for (const win of BrowserWindow.getAllWindows()) {
    try {
      if (win?.webContents && !win.isDestroyed()) sessions.add(win.webContents.session);
      for (const child of win.webContents?.getAllWebContents?.() ?? []) {
        if (child && !child.isDestroyed()) sessions.add(child.session);
      }
    } catch {
      /* ignore torn-down windows */
    }
  }
  return Array.from(sessions);
}

module.exports = {
  applyForceHttpsToSession,
  getInstalledSessions,
  isLocalhostPrivateHost,
  openGuestWindow
};
