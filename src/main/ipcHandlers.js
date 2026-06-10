const { app, BrowserWindow, ipcMain, session, shell, webContents } = require("electron");
const fs = require("node:fs/promises");
const path = require("node:path");

function installIpcHandlers({
  getPermissionKey,
  installSessionBridge,
  permissionRules,
  resolvePermissionRequest,
  toggleDevTools
}) {
  ipcMain.handle("app-version", () => app.getVersion());
  ipcMain.handle("toggle-devtools", (event, webContentsId) => {
    // PRD §5 M0 交付 E-4：统一 DevTools 入口。
    // 优先开到指定 webContentsId（通常是当前活跃 tab 的 webview）；fallback 到主窗口。
    if (typeof webContentsId === "number") {
      const target = webContents.fromId(webContentsId);
      if (target && !target.isDestroyed()) {
        toggleDevTools({ webContents: target });
        return;
      }
    }
    const win = BrowserWindow.fromWebContents(event.sender);
    if (win) {
      toggleDevTools(win);
    }
  });
  ipcMain.handle("open-incognito-window", (event) => {
    // PRD §5 M0 交付 K-12 MVP：in-memory 无痕窗口。
    // 无痕窗口使用默认 session 的 in-memory 副本（`partition` 前缀不带 persist:），
    // 所有状态、cookie、缓存都不写磁盘。
    const parentWin = BrowserWindow.fromWebContents(event.sender);
    const bounds = parentWin && !parentWin.isDestroyed() ? parentWin.getBounds() : null;
    const incognito = new BrowserWindow({
      width: bounds?.width ?? 1200,
      height: bounds?.height ?? 800,
      x: bounds ? bounds.x + 40 : undefined,
      y: bounds ? bounds.y + 40 : undefined,
      backgroundColor: "#121212",
      title: "Astra (Incognito)",
      webPreferences: {
        preload: path.join(__dirname, "preload.js"),
        partition: "in-memory:astra-incognito-" + Date.now().toString(36),
        contextIsolation: true,
        sandbox: false,
        webviewTag: true
      }
    });

    const DEV_SERVER_URL = process.env.VITE_DEV_SERVER_URL;
    if (DEV_SERVER_URL) {
      incognito.loadURL(DEV_SERVER_URL + "#?mode=incognito");
    } else {
      incognito.loadFile(path.join(__dirname, "../../dist/renderer/index.html"), {
        hash: "?mode=incognito"
      });
    }

    require("./diagnostics").installWindowDiagnostics(incognito);
    incognito.on("closed", () => {
      try {
        incognito.webContents.session.clearCache();
        incognito.webContents.session.clearStorageData();
      } catch {
        /* ignore — session was already torn down */
      }
    });
  });
  ipcMain.handle("clear-browsing-data", async (_event, partitions) => {
    const targets = getSessionsForClearing(partitions);
    await Promise.all(targets.map(async (targetSession) => {
      await targetSession.clearCache();
      await targetSession.clearStorageData();
    }));
  });
  ipcMain.handle("get-profile-storage-usage", async (_event, partitions) => {
    const validPartitions = getValidPartitions(partitions);
    return Promise.all(validPartitions.map(async (partition) => {
      const targetSession = session.fromPartition(partition);
      installSessionBridge(targetSession, partition);
      const storagePath = targetSession.getStoragePath();
      const [cacheBytes, storageBytes] = await Promise.all([
        targetSession.getCacheSize(),
        getDirectorySize(storagePath)
      ]);

      return {
        partition,
        cacheBytes,
        storageBytes,
        storagePath
      };
    }));
  });
  ipcMain.handle("set-profile-partitions", (_event, partitions) => {
    for (const partition of getValidPartitions(partitions)) {
      installSessionBridge(session.fromPartition(partition), partition);
    }
  });
  ipcMain.handle("set-permission-rules", (_event, rules) => {
    permissionRules.clear();
    for (const rule of Array.isArray(rules) ? rules : []) {
      if (
        !isValidPartition(rule?.partition) ||
        !rule?.origin ||
        !rule?.permission ||
        !["allow", "block"].includes(rule?.decision)
      ) {
        continue;
      }

      permissionRules.set(getPermissionKey(rule.partition, rule.origin, rule.permission), rule.decision);
    }
  });
  ipcMain.handle("resolve-permission-request", (_event, id, allowed) => {
    resolvePermissionRequest(id, allowed);
  });
  ipcMain.handle("show-item-in-folder", (_event, filePath) => {
    if (filePath) {
      shell.showItemInFolder(filePath);
    }
  });
  ipcMain.handle("open-path", (_event, filePath) => {
    if (!filePath) return "";
    return shell.openPath(filePath);
  });
  ipcMain.handle("print-webview", (_event, webContentsId) => {
    if (typeof webContentsId !== "number") return;
    const target = require("electron").webContents.fromId(webContentsId);
    if (target && !target.isDestroyed() && target.getType() === "webview") {
      target.print({ silent: false, printBackground: true });
    }
  });
  ipcMain.handle("get-process-memory", async (_event) => {
    const appMetrics = app.getAppMetrics();
    // ProcessMetric.memory.workingSetSize is reported in bytes by Electron on
    // all platforms (one entry per child process: GPU, utility, each <webview>,
    // etc.). The total RSS is the main process' RSS plus the working-set size
    // of every child process — no extra multiplication needed.
    const webviewWorkingSetBytes = appMetrics.reduce(
      (sum, metric) => sum + (Number.isFinite(metric.memory?.workingSetSize) ? metric.memory.workingSetSize : 0),
      0
    );
    const mainRss = Number(process.memoryUsage?.().rss) || 0;
    const mainHeap = Number(process.memoryUsage?.().heapUsed) || 0;
    return {
      appHeapBytes: mainHeap,
      appRssBytes: mainRss,
      sampledAt: Date.now(),
      totalBytes: mainRss + webviewWorkingSetBytes,
      webviewCount: appMetrics.length,
      webviewWorkingSetBytes: webviewWorkingSetBytes
    };
  });
  // Proxy favicon fetches through the user's profile sessions. The renderer's
  // <img> tags load via the default session and can't access cookies from a
  // profile's <webview>, so favicons on login-gated sites 403. This handler
  // retries the fetch through every session we know about (default + every
  // live webContents' session) and returns a data URL on the first hit.
  ipcMain.handle("get-favicon-data", async (_event, faviconUrl) => {
    if (typeof faviconUrl !== "string" || !faviconUrl) return null;
    try {
      const parsed = new URL(faviconUrl);
      if (parsed.protocol !== "http:" && parsed.protocol !== "https:") return null;
    } catch {
      return null;
    }

    const sessions = new Set([session.defaultSession]);
    for (const win of BrowserWindow.getAllWindows()) {
      try {
        if (win.webContents && !win.isDestroyed()) sessions.add(win.webContents.session);
        for (const child of win.webContents?.getAllWebContents?.() ?? []) {
          if (child && !child.isDestroyed()) sessions.add(child.session);
        }
      } catch {
        // ignore destroyed windows
      }
    }

    for (const targetSession of sessions) {
      try {
        const response = await targetSession.fetch(faviconUrl, {
          credentials: "include",
          redirect: "follow",
          signal: AbortSignal.timeout(2000)
        });
        if (!response || !response.ok) continue;
        const buffer = Buffer.from(await response.arrayBuffer());
        if (buffer.length === 0) continue;
        const contentType = response.headers.get("content-type") || "image/x-icon";
        return `data:${contentType};base64,${buffer.toString("base64")}`;
      } catch {
        // try next session
      }
    }
    return null;
  });
}

function getSessionsForClearing(partitions) {
  const targets = [session.defaultSession];
  for (const partition of getValidPartitions(partitions)) {
    targets.push(session.fromPartition(partition));
  }

  return Array.from(new Set(targets));
}

function getValidPartitions(partitions) {
  return Array.isArray(partitions) ? partitions.filter(isValidPartition) : [];
}

function isValidPartition(partition) {
  return typeof partition === "string" &&
    (partition.startsWith("persist:") || partition.startsWith("in-memory:"));
}

async function getDirectorySize(directoryPath) {
  if (!directoryPath) {
    return 0;
  }

  try {
    const entries = await fs.readdir(directoryPath, { withFileTypes: true });
    const sizes = await Promise.all(entries.map(async (entry) => {
      const entryPath = path.join(directoryPath, entry.name);
      if (entry.isSymbolicLink()) {
        return 0;
      }

      if (entry.isDirectory()) {
        return getDirectorySize(entryPath);
      }

      if (!entry.isFile()) {
        return 0;
      }

      const stat = await fs.stat(entryPath);
      return stat.size;
    }));

    return sizes.reduce((sum, size) => sum + size, 0);
  } catch {
    return 0;
  }
}

module.exports = {
  getValidPartitions,
  installIpcHandlers
};
