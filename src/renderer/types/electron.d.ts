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

export interface AstraShellApi {
  clearBrowsingData: (partitions?: string[]) => Promise<void>;
  getProfileStorageUsage: (partitions: string[]) => Promise<ProfileStorageUsage[]>;
  getVersion: () => Promise<string>;
  onDownloadEvent: (listener: (payload: DownloadEvent) => void) => () => void;
  onPermissionRequest: (listener: (payload: PermissionRequestEvent) => void) => () => void;
  openPath: (filePath: string) => Promise<string>;
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
