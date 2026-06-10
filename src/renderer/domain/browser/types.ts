export type SearchEngineKey = "google" | "duckduckgo" | "bing";
export type StartupBehavior = "restore" | "homepage";
export type ChromeAccentMode = "neutral" | "space";
export type SplitLayout = "grid" | "horizontal" | "vertical";
export type IncognitoSessionMode = "disabled" | "in-memory";
export type ThemeKey =
  | "arc-dark"
  | "arc-light"
  | "dracula"
  | "everforest"
  | "github-dark"
  | "github-light"
  | "monokai"
  | "nord"
  | "solarized-light";

export interface SearchEngine {
  name: string;
  url: string;
}

// BrowserTab state fields fall into two categories:
//   Persistent  — restored verbatim after a process restart (id, url, title,
//                 faviconUrl, groupId, canGoBack, canGoForward, isMuted,
//                 isPinned, zoomFactor, customTitle, lastActiveAt).
//   Transient   — derived from the live <webview> at runtime and always
//                 reset to defaults on startup. These must never be trusted
//                 from serialized state: isMediaPlaying, isCameraOn,
//                 isMicrophoneOn, hasUnread, isLoading, isSleeping*.
// * isSleeping is serialized so that memory-saver state survives restarts,
//   but the tab still starts without a mounted <webview>.
export interface BrowserTab {
  id: string;
  title: string;
  customTitle?: string;
  url: string;
  faviconUrl?: string;
  groupId: string | null;
  canGoBack: boolean;
  canGoForward: boolean;
  isFavorite: boolean;
  isMuted: boolean;
  isPinned: boolean;
  isLoading: boolean;
  isSleeping: boolean;
  lastActiveAt: number;
  zoomFactor: number;
  isMediaPlaying: boolean;
  isCameraOn: boolean;
  isMicrophoneOn: boolean;
  hasUnread: boolean;
}

// ClosedTab stores a complete snapshot of a BrowserTab at close time,
// including layout (groupId, isPinned) and preferences (isMuted, zoomFactor,
// customTitle). Runtime-only flags (isMediaPlaying, isCameraOn,
// isMicrophoneOn, hasUnread, isLoading) are dropped because they describe
// a live webview's transient state, not the tab itself.
export interface ClosedTab {
  id?: string;
  title: string;
  customTitle?: string;
  url: string;
  faviconUrl?: string;
  groupId: string | null;
  canGoBack: boolean;
  canGoForward: boolean;
  isMuted: boolean;
  isPinned: boolean;
  zoomFactor: number;
  closedAt: number;
}

export interface Favorite {
  id: string;
  title: string;
  url: string;
  tabId?: string;
}

export interface TabGroup {
  id: string;
  name: string;
  color: string;
  isCollapsed: boolean;
}

export interface HistoryEntry {
  id: string;
  title: string;
  url: string;
  workspaceId: string;
  visitedAt: number;
}

export interface DownloadEntry {
  id: string;
  filename: string;
  totalBytes: number;
  receivedBytes: number;
  savePath: string;
  state: string;
  startedAt: number;
  finishedAt?: number;
}

export type FaviconCache = Record<string, string>;

export type SitePermissionDecision = "allow" | "block";

export interface SitePermissionRule {
  profileId: string;
  origin: string;
  permission: string;
  decision: SitePermissionDecision;
  updatedAt: number;
}

export interface PerOriginZoomRule {
  origin: string;
  zoomFactor: number;
  updatedAt: number;
}

export interface BrowserSettings {
  chromeAccentMode: ChromeAccentMode;
  defaultZoomFactor: number;
  homepage: string;
  incognito: IncognitoSessionMode;
  memorySaverEnabled: boolean;
  memorySaverIdleMinutes: number;
  perOriginZoom: PerOriginZoomRule[];
  searchEngine: SearchEngineKey;
  startupBehavior: StartupBehavior;
  theme: ThemeKey;
}

export interface Workspace {
  id: string;
  name: string;
  accent: string;
  homepage: string;
  profileId: string;
  profileName: string;
  splitLayout: SplitLayout;
  closedTabs: ClosedTab[];
  favoriteOrder: string[];
  tabGroups: TabGroup[];
  tabs: BrowserTab[];
  activeTabId: string | null;
}

export interface BrowserState {
  activeWorkspaceId: string;
  faviconCache: FaviconCache;
  splitMode: boolean;
  splitTabId: string | null;
  splitTabIds: string[];
  essentials: Favorite[];
  history: HistoryEntry[];
  downloads: DownloadEntry[];
  sitePermissions: SitePermissionRule[];
  settings: BrowserSettings;
  workspaces: Workspace[];
}

export type PartialWorkspace = Partial<Omit<Workspace, "closedTabs" | "favoriteOrder" | "tabGroups" | "tabs">> & {
  closedTabs?: Array<Partial<ClosedTab> | null>;
  favoriteOrder?: Array<string | null>;
  tabGroups?: Array<Partial<TabGroup> | null>;
  tabs?: Array<Partial<BrowserTab> | null>;
};

export type PartialBrowserState = Partial<Omit<BrowserState, "settings" | "workspaces">> & {
  essentials?: Array<Partial<Favorite> | null>;
  faviconCache?: Record<string, unknown>;
  perOriginZoom?: Array<Partial<PerOriginZoomRule> | null>;
  settings?: Partial<BrowserSettings>;
  sitePermissions?: Array<Partial<SitePermissionRule> | null>;
  workspaces?: PartialWorkspace[];
};
