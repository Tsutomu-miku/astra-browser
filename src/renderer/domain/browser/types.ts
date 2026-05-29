export type SearchEngineKey = "google" | "duckduckgo" | "bing";
export type StartupBehavior = "restore" | "homepage";

export interface SearchEngine {
  name: string;
  url: string;
}

export interface BrowserTab {
  id: string;
  title: string;
  url: string;
  groupId: string | null;
  canGoBack: boolean;
  canGoForward: boolean;
  isMuted: boolean;
  isPinned: boolean;
  isLoading: boolean;
  isSleeping: boolean;
  lastActiveAt: number;
  zoomFactor: number;
}

export interface ClosedTab {
  title: string;
  url: string;
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

export type SitePermissionDecision = "allow" | "block";

export interface SitePermissionRule {
  profileId: string;
  origin: string;
  permission: string;
  decision: SitePermissionDecision;
  updatedAt: number;
}

export interface BrowserSettings {
  homepage: string;
  memorySaverEnabled: boolean;
  memorySaverIdleMinutes: number;
  searchEngine: SearchEngineKey;
  startupBehavior: StartupBehavior;
}

export interface Workspace {
  id: string;
  name: string;
  accent: string;
  homepage: string;
  profileId: string;
  profileName: string;
  closedTabs: ClosedTab[];
  favorites: Favorite[];
  tabGroups: TabGroup[];
  tabs: BrowserTab[];
  activeTabId: string | null;
}

export interface BrowserState {
  activeWorkspaceId: string;
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

export type PartialWorkspace = Partial<Omit<Workspace, "closedTabs" | "favorites" | "tabGroups" | "tabs">> & {
  closedTabs?: Array<Partial<ClosedTab> | null>;
  favorites?: Array<Partial<Favorite> | null>;
  tabGroups?: Array<Partial<TabGroup> | null>;
  tabs?: Array<Partial<BrowserTab> | null>;
};

export type PartialBrowserState = Partial<Omit<BrowserState, "settings" | "workspaces">> & {
  essentials?: Array<Partial<Favorite> | null>;
  settings?: Partial<BrowserSettings>;
  sitePermissions?: Array<Partial<SitePermissionRule> | null>;
  workspaces?: PartialWorkspace[];
};
