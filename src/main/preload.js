const { contextBridge, ipcRenderer } = require("electron");

contextBridge.exposeInMainWorld("astraShell", {
  clearBrowsingData: (partitions) => ipcRenderer.invoke("clear-browsing-data", partitions),
  getProfileStorageUsage: (partitions) => ipcRenderer.invoke("get-profile-storage-usage", partitions),
  getVersion: () => ipcRenderer.invoke("app-version"),
  toggleDevTools: () => ipcRenderer.invoke("toggle-devtools"),
  onDownloadEvent: (listener) => {
    const wrapped = (_event, payload) => listener(payload);
    ipcRenderer.on("download-event", wrapped);
    return () => ipcRenderer.removeListener("download-event", wrapped);
  },
  onPermissionRequest: (listener) => {
    const wrapped = (_event, payload) => listener(payload);
    ipcRenderer.on("permission-request", wrapped);
    return () => ipcRenderer.removeListener("permission-request", wrapped);
  },
  resolvePermissionRequest: (id, allowed) => ipcRenderer.invoke("resolve-permission-request", id, allowed),
  setProfilePartitions: (partitions) => ipcRenderer.invoke("set-profile-partitions", partitions),
  setPermissionRules: (rules) => ipcRenderer.invoke("set-permission-rules", rules),
  openPath: (filePath) => ipcRenderer.invoke("open-path", filePath),
  showItemInFolder: (filePath) => ipcRenderer.invoke("show-item-in-folder", filePath),
  getProcessMemory: () => ipcRenderer.invoke("get-process-memory")
});
