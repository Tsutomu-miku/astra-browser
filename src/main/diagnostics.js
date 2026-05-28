const { BrowserWindow } = require("electron");

function installWindowDiagnostics(win) {
  win.webContents.on("before-input-event", (event, input) => {
    if (!isDevToolsShortcut(input)) {
      return;
    }

    event.preventDefault();
    toggleDevTools(win);
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

  return input.shift && (input.control || input.meta) && input.key.toLowerCase() === "i";
}

function toggleDevTools(win) {
  if (win.webContents.isDevToolsOpened()) {
    win.webContents.closeDevTools();
    return;
  }

  openDevTools(win);
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
  toggleDevTools
};
