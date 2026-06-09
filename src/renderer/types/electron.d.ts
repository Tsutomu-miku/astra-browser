export type DownloadState = "progressing" | "completed" | "cancelled" | "interrupted" | string;

export interface DownloadEvent {
  id: string;
  filename: string;
  totalBytes: number;
  receivedBytes: number;
  savePath: string;
  state: DownloadState;
  startedAt: number;
  finishedAt?: number;
}

export interface PermissionRequestEvent {
  id: string;
  origin: string;
  partition?: string;
  profileId?: string;
  permission: string;
  requestingUrl: string;
}

export interface PermissionRulePayload {
  partition: string;
  origin: string;
  permission: string;
  decision: "allow" | "block";
}

export interface ProfileStorageUsage {
  partition: string;
  cacheBytes: number;
  storageBytes: number;
  storagePath: string | null;
}

export interface ProcessMemorySnapshot {
  appHeapBytes: number;
  appRssBytes: number;
  sampledAt: number;
  totalBytes: number;
  webviewCount: number;
  webviewWorkingSetBytes: number;
}

export type MainProcessAction =
  | "close-active-tab"
  | "find-match"
  | "focus-address"
  | "navigate-history"
  | "new-tab"
  | "open-command"
  | "open-downloads"
  | "open-find"
  | "open-history"
  | "open-settings-panel"
  | "print-page"
  | "reload-page"
  | "reset-zoom"
  | "restore-closed-tab"
  | "select-adjacent-tab"
  | "toggle-active-tab-favorite"
  | "toggle-active-tab-muted"
  | "toggle-sidebar"
  | "zoom-in"
  | "zoom-out";

export interface AstraShellApi {
  clearBrowsingData: (partitions?: string[]) => Promise<void>;
  getFaviconData: (url: string) => Promise<string | null>;
  getProfileStorageUsage: (partitions: string[]) => Promise<ProfileStorageUsage[]>;
  getProcessMemory: () => Promise<ProcessMemorySnapshot>;
  getVersion: () => Promise<string>;
  onDownloadEvent: (listener: (payload: DownloadEvent) => void) => () => void;
  onMainAction: (listener: (action: MainProcessAction, args: unknown[]) => void) => () => void;
  onOpenUrlInNewTab: (listener: (url: string) => void) => () => void;
  onPermissionRequest: (listener: (payload: PermissionRequestEvent) => void) => () => void;
  openPath: (filePath: string) => Promise<string>;
  printWebview: (webContentsId: number) => Promise<void>;
  resolvePermissionRequest: (id: string, allowed: boolean) => Promise<void>;
  setProfilePartitions: (partitions: string[]) => Promise<void>;
  setPermissionRules: (rules: PermissionRulePayload[]) => Promise<void>;
  showItemInFolder: (filePath: string) => Promise<void>;
  toggleDevTools: () => Promise<void>;
}

declare global {
  interface Window {
    astraShell?: AstraShellApi;
  }
}
