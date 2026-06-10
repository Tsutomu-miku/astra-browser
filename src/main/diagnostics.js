const { BrowserWindow, webContents } = require("electron");

/**
 * Electron 窗口诊断：统一走 Renderer → IPC → Main 链路，以便 DevTools 能开到
 * 聚焦的 webview（Tab/Split/Glance）而不是主窗口 renderer。
 *
 * F12 / Ctrl+Shift+I / Cmd+Alt+I 仍然在这里拦截，但是：
 *   1. 若聚焦的是主窗口 webContents → 转发给 renderer 处理（它知道当前 tab）；
 *   2. 若聚焦的已经是 webview webContents（通常来自 DevTools 窗口或 DevTools 内嵌）
 *      → 直接 toggle 它自己的 DevTools。
 */
function installWindowDiagnostics(win) {
  win.webContents.on("before-input-event", (event, input) => {
    if (!isDevToolsShortcut(input)) return;

    const focused = webContents.getFocusedWebContents();
    // 用户正在 webview 内部 DevTools / 内嵌面板打字，开到那个 webContents。
    if (focused && !focused.isDestroyed() && focused.getType() === "webview" && focused.hostWebContents === win.webContents) {
      event.preventDefault();
      toggleWebContentsDevTools(focused);
      return;
    }

    // 主窗口（渲染进程）：转发给它，由 browserStore 决定开 webview 还是主窗口。
    event.preventDefault();
    win.webContents.send("main-action", "toggle-devtools");
  });

  win.webContents.on("did-fail-load", (_event, errorCode, errorDescription, validatedUrl) => {
    console.error("[Astra] renderer failed to load", {
      errorCode,
      errorDescription,
      url: validatedUrl
    });
    openDevTools(win);
  });

  win.webContents.on("render-process-gone", (_event, details) => {
    console.error("[Astra] renderer process gone", details);
    openDevTools(win);
  });

  win.webContents.on("unresponsive", () => {
    console.error("[Astra] renderer became unresponsive");
    openDevTools(win);
  });

  win.webContents.on("console-message", (_event, ...args) => {
    console.log("[Astra renderer]", ...formatConsoleMessage(args));
  });
}

function isDevToolsShortcut(input) {
  if (input.type !== "keyDown") {
    return false;
  }

  if (input.key === "F12") {
    return true;
  }

  if (input.shift && (input.control || input.meta) && input.key.toLowerCase() === "i") {
    return true;
  }

  // macOS 原生菜单里用的是 Cmd+Alt+I；before-input-event 也捕获它。
  if (process.platform === "darwin" && input.alt && input.meta && input.key.toLowerCase() === "i") {
    return true;
  }

  return false;
}

function toggleDevTools(win) {
  if (win && !win.isDestroyed() && !win.webContents.isDestroyed()) {
    toggleWebContentsDevTools(win.webContents);
  }
}

/**
 * 对任意 webContents（主窗口 / webview / BrowserView）统一开 DevTools。
 * 由 ipcHandlers 的 toggle-devtools 调起，传入 webContentsId 支持定位到 tab 层。
 */
function toggleWebContentsDevTools(target) {
  if (!target || target.isDestroyed()) return;
  if (target.isDevToolsOpened()) {
    target.closeDevTools();
    return;
  }
  target.openDevTools({ mode: "detach" });
}

function openDevTools(win) {
  if (win.isDestroyed() || win.webContents.isDestroyed()) {
    return;
  }

  win.webContents.openDevTools({ mode: "detach" });
}

function formatConsoleMessage(args) {
  if (args.length === 1 && typeof args[0] === "object") {
    return [args[0]];
  }

  const [level, message, line, sourceId] = args;
  return [{ level, message, line, sourceId }];
}

module.exports = {
  installWindowDiagnostics,
  toggleDevTools,
  toggleWebContentsDevTools
};
