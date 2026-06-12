/* eslint-disable max-lines */
import type { TabDropPlacement, TabFolder, WorkspaceDropPlacement } from "../domain/actions";
import type {
  AddressEntry,
  AutofillDatabase,
  BrowserState,
  BrowserTab,
  DownloadEntry,
  ExtensionEntry,
  IncognitoSessionMode,
  PasswordEntry,
  PaymentMethodEntry,
  ProfileEntry,
  ReaderSettings,
  SplitLayout,
  TranslationSettings,
  Workspace
} from "../domain/browser";
import type { WebviewAction, WebviewElement } from "../types/browser-ui";
import type {
  AutoUpdateState,
  AutofillFieldFocusEvent,
  AutofillPaymentRequestEvent,
  AutofillSaveCreditcardEvent,
  PermissionRequestEvent,
  WindowRegistryWindow
} from "../types/electron";

export type Panel = "history" | "downloads" | "settings" | "site" | null;

export interface FindResultState {
  activeMatchOrdinal: number;
  finalUpdate: boolean;
  matches: number;
}

export interface BrowserStore {
  addressValue: string;
  commandOpen: boolean;
  commandQuery: string;
  compactMode: boolean;
  findOpen: boolean;
  findQuery: string;
  findResult: FindResultState | null;
  floatingSidebarOpen: boolean;
  floatingToolbarOpen: boolean;
  pageHtmlCache: Map<string, string>;
  panel: Panel;
  permissionRequest: PermissionRequestEvent | null;
  passwordSavePrompt: {
    id: string;
    origin: string;
    username: string;
    /** Plaintext (memory only; never persisted until encrypted via upsertPassword). */
    password: string;
  } | null;
  passwordVaultUnlocked: boolean;
  safeBrowsingAlert: {
    url: string;
    reason: string;
    severity?: "low" | "medium" | "high";
    proceedCallback?: () => void;
  } | null;
  sidebarCollapsed: boolean;
  sidebarWidth: number;
  state: BrowserState;
  glance: { title: string; url: string } | null;
  cachePageHtml: (tabId: string, html: string) => void;
  openActiveTabReader: () => void;
  /* ===== M2.3 Safe Browsing ===== */
  checkSafeBrowsingForNavigation: (url: string) => Promise<{
    blocked: boolean;
    alert?: { url: string; reason: string; severity?: "low" | "medium" | "high" };
  }>;
  dismissSafeBrowsingAlert: () => void;
  addTabToFavorites: (tabId: string) => void;
  addWorkspace: () => void;
  assignTabToGroup: (tabId: string, groupId: string) => void;
  clearBrowsingData: () => void;
  clearHistory: () => void;
  clearWorkspaceBrowsingData: (workspaceId: string) => void;
  closeActiveTab: () => void;
  closeOtherTabs: (tabId?: string) => void;
  closeTabGroup: (groupId: string) => void;
  closeTabsToLeft: (tabId?: string) => void;
  closeTabsToRight: (tabId?: string) => void;
  closeTab: (tabId: string) => void;
  clearSitePermission: (profileId: string, origin: string, permission: string) => void;
  clearSitePermissionsForOrigin: (profileId: string, origin: string) => void;
  clearPerOriginZoom: (origin: string) => void;
  clearAllPerOriginZoomSettings: () => void;
  deleteWorkspace: (workspaceId: string) => void;
  duplicateActiveTab: () => void;
  duplicateTab: (tabId: string) => void;
  duplicateTabGroup: (groupId: string) => void;
  fillSplitView: () => void;
  focusSplitPane: (tabId: string) => void;
  groupActiveTab: () => void;
  groupTab: (tabId: string) => void;
  groupTabsTogether: (sourceTabId: string, targetTabId: string) => void;
  ingestDownload: (download: DownloadEntry) => void;
  cancelDownload: (id: string) => void;
  pauseDownload: (id: string) => Promise<boolean>;
  resumeDownload: (id: string) => Promise<boolean>;
  removeDownload: (id: string) => void;
  ingestPermissionRequest: (request: PermissionRequestEvent) => void;
  moveTabToWorkspace: (tabId: string, workspaceId: string) => void;
  moveTabGroupToWorkspace: (groupId: string, workspaceId: string) => void;
  moveWorkspaceFavoriteToWorkspace: (tabId: string, workspaceId: string) => void;
  moveTabToNewWorkspace: (tabId: string) => void;
  moveTabGroupToNewWorkspace: (groupId: string) => void;
  moveWorkspaceFavoriteToNewWorkspace: (tabId: string) => void;
  restoreClosedTabToNewWorkspace: (closedIndex: number) => void;
  closeGlance: () => void;
  openGlance: (url: string, title?: string) => void;
  openGlanceInSplit: () => void;
  openTabInSplit: (tabId: string) => void;
  openUrlInSplit: (url: string, title?: string) => void;
  moveTabToFolderEnd: (tabId: string, folder: TabFolder) => void;
  moveTabToFolderPosition: (tabId: string, targetTabId: string, placement: TabDropPlacement) => void;
  moveTabToFavoritePosition: (tabId: string, targetTabId: string, placement: TabDropPlacement) => void;
  navigateActiveTab: (url: string, webview?: WebviewElement) => void;
  newTab: () => void;
  newTabInGroup: (groupId: string) => void;
  openUrlInActiveWorkspace: (url: string, title?: string) => void;
  recordHistory: (tabId: string, url: string) => void;
  removeHistoryEntry: (historyId: string) => void;
  removeEssential: (url: string) => void;
  removeTabFromSplit: (tabId: string) => void;
  removeWorkspaceFavorite: (tabId: string) => void;
  replaceBrowserState: (state: BrowserState) => void;
  reorderEssential: (essentialId: string, targetEssentialId: string, placement: TabDropPlacement) => void;
  reorderTabGroup: (groupId: string, targetGroupId: string, placement: TabDropPlacement) => void;
  reorderWorkspaceFavorite: (tabId: string, targetTabId: string, placement: TabDropPlacement) => void;
  reorderTab: (tabId: string, targetTabId: string, placement: TabDropPlacement) => void;
  reorderWorkspace: (workspaceId: string, targetWorkspaceId: string, placement: WorkspaceDropPlacement) => void;
  runWebviewAction: (action: WebviewAction, webview?: WebviewElement) => void;
  selectAdjacentTab: (direction: 1 | -1) => void;
  selectTab: (tabId: string) => void;
  selectSplitTab: (splitId: string) => void;
  sleepIdleTabs: () => void;
  sleepInactiveTabs: () => void;
  sleepTabGroup: (groupId: string) => void;
  sleepTab: (tabId: string) => void;
  resetActiveTabZoom: (webview?: WebviewElement) => void;
  resolvePermissionRequest: (decision: "allow" | "block") => void;
  restoreClosedTab: (closedIndex: number) => void;
  restoreClosedTabToWorkspace: (closedIndex: number, workspaceId: string) => void;
  restoreLastClosedTab: () => void;
  setActiveTabZoom: (zoomFactor: number, webview?: WebviewElement) => void;
  setAddressValue: (value: string) => void;
  setCommandOpen: (open: boolean) => void;
  setCommandQuery: (query: string) => void;
  setFindOpen: (open: boolean) => void;
  setFindQuery: (query: string) => void;
  setFindResult: (result: FindResultState | null) => void;
  setPanel: (panel: Panel) => void;
  setSidebarWidth: (width: number) => void;
  setSplitLayout: (layout: SplitLayout) => void;
  setSitePermission: (profileId: string, origin: string, permission: string, decision: "allow" | "block") => void;
  switchWorkspace: (workspaceId: string) => void;
  toggleActiveTabFavorite: () => void;
  toggleActiveTabEssential: () => void;
  toggleActiveTabMuted: (webview?: WebviewElement) => void;
  toggleActiveTabPinned: () => void;
  toggleCompactMode: () => void;
  toggleFloatingSidebar: () => void;
  toggleFloatingToolbar: () => void;
  toggleTabGroupCollapsed: (groupId: string) => void;
  toggleTabEssential: (tabId: string) => void;
  toggleTabFavorite: (tabId: string) => void;
  toggleTabMuted: (tabId: string, webview?: WebviewElement) => void;
  toggleTabPinned: (tabId: string) => void;
  toggleSidebar: () => void;
  toggleSplitMode: () => void;
  swapSplitPanes: (splitId?: string) => void;
  toggleSplitPaneSide: () => void;
  setSplitPaneSide: (side: "left" | "right") => void;
  ungroupActiveTab: () => void;
  ungroupTab: (tabId: string) => void;
  ungroupTabGroup: (groupId: string) => void;
  zoomIn: (webview?: WebviewElement) => void;
  zoomOut: (webview?: WebviewElement) => void;
  setIncognito: (mode: IncognitoSessionMode) => void;
  setPerOriginZoom: (origin: string, zoom: number) => void;
  toggleActiveDevTools: (webview?: WebviewElement) => void;
  newIncognitoWindow: () => void;
  newGuestWindow: () => void;
  syncForceHttps: (enabled: boolean) => void;
  syncSafeBrowsing: (settings: { enabled: boolean; remoteLookupUrl?: string }) => void;
  reportSafeBrowsingDecision: (decision: {
    url: string;
    reason: string;
    severity?: "low" | "medium" | "high";
    action: "block" | "proceed";
  }) => void;
  toggleActivePictureInPicture: (activeWebviewId?: number) => Promise<{ success: boolean; reason?: string; entering?: boolean }>;
  syncMediaSessionOnTabSwitch: (payload: { fromId?: number; toId?: number }) => void;
  updateSettings: (patch: Partial<BrowserState["settings"]>) => void;
  updateReaderSettings: (patch: Partial<ReaderSettings>) => void;
  updateTranslationSettings: (patch: Partial<TranslationSettings>) => void;
  upsertPassword: (entry: PasswordEntry) => void;
  removePassword: (id: string) => void;
  touchPasswordUsed: (id: string) => void;
  /** Add a save-password prompt shown in the password prompt bubble. */
  ingestPasswordSavePrompt: (prompt: { id: string; origin: string; username: string; password: string }) => void;
  /** Dismiss the save-password prompt without saving. */
  rejectPasswordSavePrompt: (id: string) => void;
  /** Save the prompt to the vault (creates PasswordEntry, encrypts if vault unlocked). */
  acceptPasswordSavePrompt: (id: string) => Promise<void>;
  /** Unlock the in-memory vault (MVP passphrase is empty; M2 adds user-chosen password + keytar). */
  unlockPasswordVault: (passphrase?: string) => Promise<void>;
  lockPasswordVault: () => void;
  /** Decrypt a PasswordEntry (returns plaintext password; requires unlocked vault). */
  decryptPassword: (id: string) => Promise<string | null>;
  upsertAddress: (entry: AddressEntry) => void;
  removeAddress: (id: string) => void;
  upsertPaymentMethod: (entry: PaymentMethodEntry) => void;
  removePaymentMethod: (id: string) => void;
  importBookmarks: (html: string, opts: { source: "chrome" | "edge" | "firefox" | "safari" | "html"; maxCount?: number }) => {
    essentialsAdded: number;
    favoritesAdded: number;
  };
  updateTabGroup: (groupId: string, patch: Partial<Pick<Workspace["tabGroups"][number], "name" | "color">>) => void;
  updateTab: (tabId: string, patch: Partial<BrowserTab>) => void;
  updateWorkspaceById: (workspaceId: string, patch: Partial<Pick<Workspace, "name" | "accent" | "homepage" | "profileName">>) => void;
  updateWorkspace: (patch: Partial<Pick<Workspace, "name" | "accent" | "homepage" | "profileName">>) => void;
  /* ===== M2.1 Profiles / Extensions / Reset ===== */
  addProfile: (name: string, color: string) => void;
  removeProfile: (id: string) => void;
  switchProfile: (id: string) => void;
  switchActiveProfile: (profileId: string) => void;
  addExtension: (ext: Omit<ExtensionEntry, "id" | "installedAt">) => void;
  removeExtension: (id: string) => void;
  toggleExtensionEnabled: (id: string, enabled: boolean) => void;
  /** M2.5 E-1/E-2 MV3 PoC：从文件夹安装真实 CRX/manifest 扩展。 */
  installMv3ExtensionFromFolder: (folderPath: string) => Promise<{ ok: boolean; id?: string; reason?: string }>;
  uninstallMv3Extension: (id: string) => Promise<{ ok: boolean; reason?: string }>;
  setMv3ExtensionEnabled: (id: string, enabled: boolean) => Promise<boolean>;
  reloadInstalledExtensions: () => Promise<void>;
  pickFolderAndInstallMv3Extension: () => Promise<{
    canceled?: boolean;
    ok?: boolean;
    folder?: string;
    id?: string;
    reason?: string;
  }>;
  resetSettings: () => void;
  clearAllDownloads: () => void;
  /** Re-open the download URL in a new tab (used for retry). */
  retryDownload: (url: string) => void;
  openUserDataFolder: (kind?: "userData" | "profile") => void;
  restartBrowser: () => void;
  /* ===== M2.4 W-3 PWA install ===== */
  ingestPendingPwaInstallPrompt: (
    prompt: import("../domain/browser").PwaInstallPrompt
  ) => void;
  dismissPendingPwaInstallPrompt: (origin: string) => void;
  confirmPwaInstall: (origin: string) => Promise<import("../types/electron").PwaInstallConfirmResult>;
  ingestInstalledPwaApp: (app: import("../domain/browser").InstalledPwaApp) => void;
  reloadInstalledPwaApps: () => Promise<void>;
  launchInstalledPwa: (origin: string) => Promise<{ ok: boolean; reason?: string }>;
  uninstallPwa: (origin: string) => Promise<{ ok: boolean; reason?: string }>;
  /* ===== M2.5 W-10 auto-update ===== */
  autoUpdateState: AutoUpdateState | null;
  setAutoUpdateState: (state: AutoUpdateState) => void;
  refreshAutoUpdateState: () => Promise<void>;
  checkForUpdates: () => Promise<{ ok: boolean; hasUpdate?: boolean; error?: string; reason?: string }>;
  downloadUpdate: () => Promise<{ ok: boolean; error?: string; reason?: string }>;
  installUpdateAndRestart: () => Promise<boolean>;
  /* ===== ADR-0005 / W-1 multi-window × Space registry ===== */
  ownWindowId: number | null;
  windowRegistry: WindowRegistryWindow[];
  setOwnWindowId: (id: number | null) => void;
  syncWindowRegistry: (snapshot: WindowRegistryWindow[]) => void;
  /** 切换当前窗口的 active Space；底层调用 switchSpace + setActiveSpace IPC */
  switchActiveSpaceForWindow: (spaceId: string) => void;
  /** 当前窗口激活某 Tab（同时通知 WindowRegistry 更新该窗口的该 Space activeTabId，
   *  以便同 Space 在不同窗口有独立的 tab 选择状态） */
  setActiveTabForWindow: (spaceId: string, tabId: string) => void;
  /** 把当前 Tab 打开到新窗口（跨窗口 Tab 拖拽 MVP） */
  openTabInNewWindow: (spaceId: string, tabId: string) => Promise<{
    ok: boolean;
    reason?: string;
    windowId?: number | null;
  }>;
  /* ===== ADR-0005 / P-2 autofill ===== */
  /** 浮动弹窗状态：收到 field-focus 时填充并展示，用户点击/切换/隐藏时清空。 */
  autofillPrompt: (AutofillFieldFocusEvent & {
    /** 匹配到的候选（地址 or 信用卡）。 */
    matches: Array<{
      id: string;
      label: string;
      subtitle?: string;
      values: Record<string, string>;
    }>;
    /** 若为 true，末尾显示 "保存新地址" 按钮。 */
    offerSaveAddress?: boolean;
  }) | null;
  /** autofill bridge preload 文件路径，用于 <webview preload 属性>。 */
  autofillBridgePath: string | null;
  /** 一次性在启动时从 main 拉取 bridge preload 路径（由 useBrowserEffects 调用）。 */
  loadAutofillBridgePath: () => Promise<void>;
  /** main 推送 field-focus → renderer 生成 matches → 打开 popup。 */
  showAutofillPopup: (event: AutofillFieldFocusEvent) => void;
  /** 收到 field-blur / 失焦 / 用户 esc 时关闭。 */
  hideAutofillPopup: (webContentsId?: number | null) => void;
  /** 用户选择某个候选项 → 触发 IPC fillForm。 */
  acceptAutofillMatch: (matchId: string) => Promise<void>;
  /** 把当前 form 的值保存为新地址（由用户在 popup 底部点击触发）。 */
  saveCurrentFormAsAddress: (partial: Partial<AddressEntry>) => void;
  /* ===== ADR-0005 / P-3 PaymentRequest + 保存新卡 prompt ===== */
  /** 激活时，站点触发 window.PaymentRequest.show → 弹出卡片选择器。 */
  paymentRequestPrompt: (AutofillPaymentRequestEvent & {
    candidateCards: Array<{
      id: string;
      label: string;
      subtitle?: string;
      cardholderName: string;
      cardLastFour: string;
      expiryMonth?: number;
      expiryYear?: number;
    }>;
  }) | null;
  /** 当用户提交含有 CC 的表单后激活的保存新卡 prompt。
   *  注意：不保存明文；只在用户点 Save 时走 vault 加密。 */
  saveCreditcardPrompt: AutofillSaveCreditcardEvent | null;
  /** P-3: 订阅 payment-request 事件后 → 渲染侧打开卡片选择器。 */
  showPaymentRequestPrompt: (event: AutofillPaymentRequestEvent) => void;
  /** 用户在 PaymentRequest 选择卡片 → 解密卡 → 合成 response → IPC 回传。 */
  acceptPaymentRequestCard: (matchId: string) => Promise<void>;
  /** 用户在 PaymentRequest 取消 → 走原生 Payment Sheet。 */
  rejectPaymentRequest: () => void;
  /** P-3: 订阅 save-creditcard 事件后 → 打开保存 prompt。 */
  showSaveCreditcardPrompt: (event: AutofillSaveCreditcardEvent) => void;
  /** 用户确认保存 → 校验 (Luhn + expiry) + 加密 + upsert。 */
  acceptSaveCreditcard: (opts: { label?: string }) => Promise<string | null>;
  /** 用户点击不保存。 */
  rejectSaveCreditcard: () => void;
  /** 封装：把 raw card draft 转 PaymentMethodEntry（校验 + 加密）。
   *  供 SettingsPanel + saveCreditcardPrompt 复用。失败返回 string error。 */
  createPaymentMethodFromDraft: (draft: {
    cardholderName: string;
    pan: string;
    csc?: string;
    expiryMonth?: number;
    expiryYear?: number;
    label?: string;
  }) => Promise<{ ok: true; entry: PaymentMethodEntry } | { ok: false; reason: string }>;
}
