/* eslint-disable max-lines */
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

export interface SavePasswordRequestEvent {
  id: string;
  origin: string;
  username: string;
  password: string;
}

export type MainProcessAction =
  | "close-active-tab"
  | "find-match"
  | "focus-address"
  | "navigate-history"
  | "new-incognito-window"
  | "new-guest-window"
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
  | "toggle-devtools"
  | "toggle-picture-in-picture"
  | "toggle-sidebar"
  | "zoom-in"
  | "zoom-out";

export interface AstraShellApi {
  clearBrowsingData: (partitions?: string[]) => Promise<void>;
  getFaviconData: (url: string) => Promise<string | null>;
  getProfileStorageUsage: (partitions: string[]) => Promise<ProfileStorageUsage[]>;
  getProcessMemory: () => Promise<ProcessMemorySnapshot>;
  getVersion: () => Promise<string>;
  getUserDataPaths: () => Promise<{ userData: string; profile: string }>;
  onDownloadEvent: (listener: (payload: DownloadEvent) => void) => () => void;
  onMainAction: (listener: (action: MainProcessAction, args: unknown[]) => void) => () => void;
  onOpenUrlInNewTab: (listener: (url: string) => void) => () => void;
  onPermissionRequest: (listener: (payload: PermissionRequestEvent) => void) => () => void;
  onSavePasswordRequest: (listener: (payload: SavePasswordRequestEvent) => void) => () => void;
  openIncognitoWindow: () => Promise<void>;
  openGuestWindow: () => Promise<void>;
  openPath: (filePath: string) => Promise<string>;
  printWebview: (webContentsId: number, options?: {
    printBackground?: boolean;
    printHeadersAndFooters?: boolean;
    color?: "color" | "grayscale";
    landscape?: boolean;
    scale?: number;
    margins?: "default" | "none" | "minimal" | "custom";
    collate?: boolean;
    copies?: number;
    pageSize?: "A4" | "Letter" | "Legal" | "Tabloid" | string;
    pdfPath?: string;
  }) => Promise<void | { ok: boolean; path?: string; error?: string }>;
  relaunch: () => Promise<void>;
  resolvePermissionRequest: (id: string, allowed: boolean) => Promise<void>;
  resolvePasswordSave?: (id: string, accepted: boolean) => Promise<void>;
  syncForceHttps: (enabled: boolean) => Promise<void>;
  cancelDownload: (id: string) => Promise<void>;
  pauseDownload: (id: string) => Promise<boolean>;
  resumeDownload: (id: string) => Promise<boolean>;
  setProfilePartitions: (partitions: string[]) => Promise<void>;
  setPermissionRules: (rules: PermissionRulePayload[]) => Promise<void>;
  showItemInFolder: (filePath: string) => Promise<void>;
  toggleDevTools: (webContentsId?: number) => Promise<void>;
  togglePictureInPicture?: (webContentsId?: number) => Promise<PiPToggleResult>;
  syncMediaSession?: (payload: { fromId?: number; toId?: number }) => Promise<void>;
  safeBrowsing?: {
    syncSettings: (settings: { enabled: boolean; remoteLookupUrl?: string }) => Promise<void>;
    checkNavigation: (url: string) => Promise<SafeBrowsingCheckResult>;
    checkDownload: (payload: { url: string; filename?: string }) => Promise<SafeBrowsingCheckResult>;
  };
  /* ===== M2.4 W-3 PWA install ===== */
  onPwaInstallPromptAvailable: (
    listener: (payload: PwaInstallPromptPayload) => void
  ) => () => void;
  onPwaAppInstalled: (
    listener: (payload: PwaInstalledAppRecord) => void
  ) => () => void;
  pwaConfirmInstall: (origin: string) => Promise<PwaInstallConfirmResult>;
  pwaListInstalled: () => Promise<PwaInstalledAppRecord[]>;
  pwaLaunch: (origin: string) => Promise<{ ok: boolean; reason?: string }>;
  pwaUninstall: (origin: string) => Promise<{ ok: boolean; reason?: string }>;

  /* ===== M2.5 E-1/E-2 MV3 extension compatibility (PoC) ===== */
  mv3Extensions: {
    list: () => Promise<Array<{
      id: string;
      name: string;
      version: string;
      description: string;
      enabled: boolean;
    }>>;
    setEnabled: (id: string, enabled: boolean) => Promise<boolean>;
    uninstall: (id: string) => Promise<{ ok: boolean; reason?: string }>;
    installFromFolder: (folderPath: string) => Promise<{ ok: boolean; id?: string; reason?: string }>;
    pickFolderAndInstall: () => Promise<
      | { canceled: true }
      | { canceled: false; folder: string; ok: boolean; id?: string; reason?: string }
    >;
  };
  autoUpdate: AutoUpdateApi;
  windowRegistry: WindowRegistryApi;
  autofill: AutofillApi;
}

/* ===== ADR-0005 / P-2 Autofill ===== */
export type AutofillBucket = "address" | "creditcard";

export interface AutofillFieldMeta {
  type: string;
  bucket: AutofillBucket;
  label: string;
  selector: string;
}

export interface AutofillFieldFocusEvent {
  webContentsId: number | null;
  detail: {
    focusedType: string;
    focusedBucket: AutofillBucket;
    focusedLabel: string;
    host: string;
    fields: AutofillFieldMeta[];
  };
}

export interface AutofillFillRequest {
  webContentsId: number;
  values: Record<string, string>;
}

export interface AutofillPaymentRequestEvent {
  webContentsId: number | null;
  detail: {
    correlationId: string;
    host: string;
    methodData: Array<{ supportedMethods: string; data?: unknown }>;
    total?: { label: string; amount: { currency: string; value: string } };
    displayItems?: Array<{ label: string; amount: { currency: string; value: string } }>;
  };
}

export interface AutofillPaymentResponse {
  correlationId: string;
  canceled: boolean;
  paymentResponse?: {
    cardholderName: string;
    cardNumber: string;
    expiryMonth: string;
    expiryYear: string;
    cardSecurityCode: string;
    billingAddress?: unknown;
  };
}

export interface AutofillPaymentResponseRequest {
  webContentsId: number;
  response: AutofillPaymentResponse;
}

export interface AutofillSaveCreditcardEvent {
  webContentsId: number | null;
  detail: {
    host: string;
    cardholderName: string;
    number: string;
    expiryRaw: string;
    expiryMonth: string;
    expiryYear: string;
    cvv: string;
  };
}

export interface AutofillApi {
  getBridgePath: () => Promise<string | null>;
  fillForm: (payload: AutofillFillRequest) => Promise<{ ok: boolean; reason?: string }>;
  sendPaymentResponse: (
    payload: AutofillPaymentResponseRequest
  ) => Promise<{ ok: boolean; reason?: string }>;
  onFieldFocus: (
    listener: (event: AutofillFieldFocusEvent) => void
  ) => () => void;
  onFieldBlur: (
    listener: (event: { webContentsId: number | null }) => void
  ) => () => void;
  onPaymentRequest: (
    listener: (event: AutofillPaymentRequestEvent) => void
  ) => () => void;
  onSaveCreditcard: (
    listener: (event: AutofillSaveCreditcardEvent) => void
  ) => () => void;
}

/**
 * M2.5 W-10: electron-updater 状态 + IPC。
 */
export type AutoUpdateStatus =
  | "idle"
  | "checking"
  | "available"
  | "not-available"
  | "downloading"
  | "ready"
  | "error";

export interface AutoUpdateInfoFile {
  url: string;
  size: number;
}

export interface AutoUpdateInfo {
  version: string;
  releaseName: string | null;
  releaseNotes: string | null;
  releaseDate: string | null;
  files: AutoUpdateInfoFile[];
}

export interface AutoUpdateProgress {
  percent: number;
  bytesPerSecond: number;
  transferred: number;
  total: number;
}

export interface AutoUpdateState {
  status: AutoUpdateStatus;
  currentVersion: string;
  info: AutoUpdateInfo | null;
  error: string | null;
  progress: AutoUpdateProgress | null;
  isEnabled: boolean;
}

export interface AutoUpdateApi {
  getState: () => Promise<AutoUpdateState>;
  check: () => Promise<{ ok: boolean; hasUpdate?: boolean; error?: string; reason?: string }>;
  download: () => Promise<{ ok: boolean; error?: string; reason?: string }>;
  installAndRestart: () => Promise<boolean>;
  onStateChange: (listener: (state: AutoUpdateState) => void) => () => void;
}

export interface PiPToggleResult {
  success: boolean;
  entering?: boolean;
  reason?: string;
}

export interface SafeBrowsingCheckResult {
  allowed: boolean;
  reason?: string;
  severity?: "low" | "medium" | "high";
  url?: string;
  filename?: string;
}

/* ===== M2.4 W-3 PWA install ===== */
export interface PwaInstallPromptPayload {
  origin: string;
  platforms: string[];
  title: string;
  url: string;
}

export interface PwaInstalledAppRecord {
  id: string;
  origin: string;
  name: string;
  startUrl: string;
  icon?: string;
}

export interface PwaInstallConfirmResult {
  accepted: boolean;
  outcome?: string;
  reason?: string;
}

declare global {
  interface Window {
    astraShell?: AstraShellApi;
  }
}

/**
 * ADR-0005 Accepted / W-1: 多窗口 × Space 同步类型。
 */
export interface WindowRegistrySpaceFocus {
  activeTabId: string;
  splitFocusTabId?: string | null;
  glance?: { title: string; url: string } | null;
  panel?: "history" | "downloads" | "settings" | "site" | null;
  findOpen?: boolean;
}

export interface WindowRegistryWindow {
  windowId: number;
  bounds: { x: number; y: number; width: number; height: number };
  isMaximized: boolean;
  isFullScreen: boolean;
  activeSpaceId: string;
  spaceFocus: Record<string, WindowRegistrySpaceFocus>;
}

export interface WindowRegistryApi {
  get: () => Promise<{
    ownWindowId: number | null;
    registry: WindowRegistryWindow[];
  }>;
  setActiveSpace: (payload: {
    spaceId: string;
    defaultActiveTabId?: string;
  }) => Promise<{ ok: boolean; reason?: string }>;
  setFocus: (payload: {
    spaceId: string;
    patch: Partial<WindowRegistrySpaceFocus>;
  }) => Promise<{ ok: boolean; reason?: string }>;
  openNewWindow: (payload: {
    spaceId: string;
    defaultActiveTabId?: string;
  }) => Promise<{ ok: boolean; reason?: string; windowId?: number | null }>;
  onSync: (
    listener: (registry: WindowRegistryWindow[]) => void
  ) => () => void;
}