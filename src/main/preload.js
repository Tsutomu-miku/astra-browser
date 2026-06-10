const { contextBridge, ipcRenderer } = require("electron");

contextBridge.exposeInMainWorld("astraShell", {
  clearBrowsingData: (partitions) => ipcRenderer.invoke("clear-browsing-data", partitions),
  getProfileStorageUsage: (partitions) => ipcRenderer.invoke("get-profile-storage-usage", partitions),
  getVersion: () => ipcRenderer.invoke("app-version"),
  // 可选 webContentsId：如果传入，开到那个 tab 的 webview；否则开主窗口 DevTools
  // （PRD E-4 统一入口）
  toggleDevTools: (webContentsId) => ipcRenderer.invoke("toggle-devtools", webContentsId),
  openIncognitoWindow: () => ipcRenderer.invoke("open-incognito-window"),
  onDownloadEvent: (listener) => {
    const wrapped = (_event, payload) => listener(payload);
    ipcRenderer.on("download-event", wrapped);
    return () => ipcRenderer.removeListener("download-event", wrapped);
  },
  onOpenUrlInNewTab: (listener) => {
    const wrapped = (_event, url) => listener(url);
    ipcRenderer.on("open-url-in-new-tab", wrapped);
    return () => ipcRenderer.removeListener("open-url-in-new-tab", wrapped);
  },
  // Main-process menu-driven actions. The action name mirrors the ShortcutIntent
  // `type` field where possible so the renderer can dispatch them uniformly.
  onMainAction: (listener) => {
    const wrapped = (_event, action, ...args) => listener(action, args);
    ipcRenderer.on("main-action", wrapped);
    return () => ipcRenderer.removeListener("main-action", wrapped);
  },
  onPermissionRequest: (listener) => {
    const wrapped = (_event, payload) => listener(payload);
    ipcRenderer.on("permission-request", wrapped);
    return () => ipcRenderer.removeListener("permission-request", wrapped);
  },
  resolvePermissionRequest: (id, allowed) => ipcRenderer.invoke("resolve-permission-request", id, allowed),
  cancelDownload: (id) => ipcRenderer.invoke("cancel-download", id),
  setProfilePartitions: (partitions) => ipcRenderer.invoke("set-profile-partitions", partitions),
  setPermissionRules: (rules) => ipcRenderer.invoke("set-permission-rules", rules),
  openPath: (filePath) => ipcRenderer.invoke("open-path", filePath),
  printWebview: (webContentsId) => ipcRenderer.invoke("print-webview", webContentsId),
  showItemInFolder: (filePath) => ipcRenderer.invoke("show-item-in-folder", filePath),
  getProcessMemory: () => ipcRenderer.invoke("get-process-memory"),
  getFaviconData: (url) => ipcRenderer.invoke("get-favicon-data", url),
  relaunch: () => ipcRenderer.invoke("app-relaunch"),
  getUserDataPaths: () => ipcRenderer.invoke("get-user-data-paths"),
  syncForceHttps: (enabled) => ipcRenderer.invoke("sync-force-https", enabled),
  openGuestWindow: () => ipcRenderer.invoke("open-guest-window")
});
