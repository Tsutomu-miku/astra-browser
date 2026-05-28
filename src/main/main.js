const { app, BrowserWindow, session, shell } = require("electron");
const path = require("node:path");
const { installWindowDiagnostics, toggleDevTools } = require("./diagnostics");
const { installIpcHandlers } = require("./ipcHandlers");

const APP_ORIGIN = "astra://app";
const DEV_SERVER_URL = process.env.VITE_DEV_SERVER_URL;
const PERMISSION_TIMEOUT_MS = 30_000;
const windows = new Set();
const bridgedSessions = new WeakSet();
const sessionPartitions = new WeakMap();
const permissionCallbacks = new Map();
const permissionRules = new Map();

function createWindow() {
  const win = new BrowserWindow({
    width: 1440,
    height: 960,
    minWidth: 960,
    minHeight: 640,
    title: "Astra Browser",
    titleBarStyle: "hiddenInset",
    backgroundColor: "#111318",
    webPreferences: {
      preload: path.join(__dirname, "preload.js"),
      contextIsolation: true,
      nodeIntegration: false,
      webviewTag: true
    }
  });

  windows.add(win);
  installWindowDiagnostics(win);

  win.on("closed", () => {
    windows.delete(win);
  });

  installSessionBridge(session.defaultSession, "default");
  if (DEV_SERVER_URL) {
    win.loadURL(DEV_SERVER_URL);
  } else {
    win.loadFile(path.join(__dirname, "../../dist/renderer/index.html"));
  }
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
    const basePayload = {
      id,
      filename: item.getFilename(),
      totalBytes: item.getTotalBytes(),
      receivedBytes: 0,
      savePath: item.getSavePath(),
      state: "progressing",
      startedAt
    };

    broadcastDownload(basePayload);

    item.on("updated", (_updatedEvent, state) => {
      broadcastDownload({
        ...basePayload,
        receivedBytes: item.getReceivedBytes(),
        totalBytes: item.getTotalBytes(),
        savePath: item.getSavePath(),
        state
      });
    });

    item.once("done", (_doneEvent, state) => {
      broadcastDownload({
        ...basePayload,
        receivedBytes: item.getReceivedBytes(),
        totalBytes: item.getTotalBytes(),
        savePath: item.getSavePath(),
        state,
        finishedAt: Date.now()
      });
    });
  });
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
  getPermissionKey,
  installSessionBridge,
  permissionRules,
  resolvePermissionRequest,
  toggleDevTools
});

app.whenReady().then(() => {
  createWindow();

  app.on("activate", () => {
    if (BrowserWindow.getAllWindows().length === 0) {
      createWindow();
    }
  });
});

app.on("window-all-closed", () => {
  if (process.platform !== "darwin") {
    app.quit();
  }
});

app.on("web-contents-created", (_event, contents) => {
  installSessionBridge(contents.session);

  contents.setWindowOpenHandler(({ url }) => {
    if (!url.startsWith(APP_ORIGIN)) {
      shell.openExternal(url);
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
