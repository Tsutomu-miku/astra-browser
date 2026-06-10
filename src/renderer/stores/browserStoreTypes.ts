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
import type { PermissionRequestEvent } from "../types/electron";

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
  sidebarCollapsed: boolean;
  sidebarWidth: number;
  state: BrowserState;
  glance: { title: string; url: string } | null;
  cachePageHtml: (tabId: string, html: string) => void;
  openActiveTabReader: () => void;
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
  ungroupActiveTab: () => void;
  ungroupTab: (tabId: string) => void;
  ungroupTabGroup: (groupId: string) => void;
  zoomIn: (webview?: WebviewElement) => void;
  zoomOut: (webview?: WebviewElement) => void;
  setIncognito: (mode: IncognitoSessionMode) => void;
  setPerOriginZoom: (origin: string, zoom: number) => void;
  toggleActiveDevTools: (webview?: WebviewElement) => void;
  newIncognitoWindow: () => void;
  updateSettings: (patch: Partial<BrowserState["settings"]>) => void;
  updateReaderSettings: (patch: Partial<ReaderSettings>) => void;
  updateTranslationSettings: (patch: Partial<TranslationSettings>) => void;
  upsertPassword: (entry: PasswordEntry) => void;
  removePassword: (id: string) => void;
  touchPasswordUsed: (id: string) => void;
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
  resetSettings: () => void;
  clearAllDownloads: () => void;
  openUserDataFolder: (kind?: "userData" | "profile") => void;
  restartBrowser: () => void;
}
