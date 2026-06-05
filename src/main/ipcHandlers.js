const { app, BrowserWindow, ipcMain, session, shell } = require("electron");
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
  ipcMain.handle("toggle-devtools", (event) => {
    const win = BrowserWindow.fromWebContents(event.sender);
    if (win) {
      toggleDevTools(win);
    }
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
  ipcMain.handle("get-process-memory", async (_event) => {
    const appMetrics = app.getAppMetrics();
    const webviewMemory = appMetrics.reduce((sum, metric) => sum + (metric.memory?.workingSetSize ?? 0), 0);
    const totalMemoryBytes = process.memoryUsage
      ? (process.memoryUsage().rss + webviewMemory * 1024 * 1024)
      : webviewMemory * 1024 * 1024;
    return {
      appHeapBytes: process.memoryUsage ? process.memoryUsage().heapUsed : 0,
      appRssBytes: process.memoryUsage ? process.memoryUsage().rss : 0,
      sampledAt: Date.now(),
      totalBytes: totalMemoryBytes,
      webviewCount: appMetrics.length,
      webviewWorkingSetBytes: webviewMemory * 1024 * 1024
    };
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
  return typeof partition === "string" && partition.startsWith("persist:");
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
