export const DEFAULT_URL = "https://www.google.com";

import type {
  BrowserState,
  BrowserTab,
  ClosedTab,
  DownloadEntry,
  Favorite,
  HistoryEntry,
  PartialBrowserState,
  SearchEngine,
  SearchEngineKey,
  SitePermissionRule,
  StartupBehavior,
  TabGroup,
  Workspace
} from "./browser-types";
export { formatBytes } from "./formatting";
import { normalizeSitePermissions } from "./sitePermissions";
import { normalizeTabGroups } from "./tab-groups";
export { getBrowserPartitions, getProfileIdForPartition, getWorkspacePartition } from "./workspaceProfiles";
import { normalizeWorkspaceProfile } from "./workspaceProfiles";
import { DEFAULT_ZOOM_FACTOR, normalizeZoomFactor } from "./zoom";

export type {
  BrowserState,
  BrowserTab,
  ClosedTab,
  DownloadEntry,
  Favorite,
  HistoryEntry,
  PartialBrowserState,
  SearchEngineKey,
  SitePermissionRule,
  StartupBehavior,
  TabGroup,
  Workspace
} from "./browser-types";

export const SEARCH_ENGINES: Record<SearchEngineKey, SearchEngine> = {
  google: {
    name: "Google",
    url: "https://www.google.com/search?q="
  },
  duckduckgo: {
    name: "DuckDuckGo",
    url: "https://duckduckgo.com/?q="
  },
  bing: {
    name: "Bing",
    url: "https://www.bing.com/search?q="
  }
};

const WORKSPACE_ACCENTS = [
  "#7dd3fc",
  "#f0abfc",
  "#86efac",
  "#fda4af",
  "#fde68a",
  "#c4b5fd"
];

export function createId(): string {
  return globalThis.crypto?.randomUUID?.() ?? `${Date.now()}-${Math.random().toString(16).slice(2)}`;
}

export function createTab(title: string, url: string): BrowserTab {
  return {
    id: createId(),
    title,
    url,
    groupId: null,
    canGoBack: false,
    canGoForward: false,
    isMuted: false,
    isPinned: false,
    isLoading: false,
    zoomFactor: DEFAULT_ZOOM_FACTOR
  };
}

export function createFavorite(title: string, url: string): Favorite {
  return {
    id: createId(),
    title,
    url
  };
}

export function createDefaultState(): BrowserState {
  return {
    activeWorkspaceId: "personal",
    splitMode: false,
    splitTabId: null,
    history: [],
    downloads: [],
    sitePermissions: [],
    settings: {
      homepage: DEFAULT_URL,
      searchEngine: "google",
      startupBehavior: "restore"
    },
    workspaces: [
      {
        id: "personal",
        name: "Personal",
        accent: "#7dd3fc",
        profileId: "personal",
        profileName: "Personal",
        closedTabs: [],
        favorites: [
          createFavorite("Chromium", "https://www.chromium.org"),
          createFavorite("MDN", "https://developer.mozilla.org")
        ],
        tabGroups: [],
        tabs: [
          createTab("New Tab", DEFAULT_URL)
        ],
        activeTabId: null
      },
      {
        id: "work",
        name: "Work",
        accent: "#f0abfc",
        profileId: "work",
        profileName: "Work",
        closedTabs: [],
        favorites: [
          createFavorite("GitHub", "https://github.com")
        ],
        tabGroups: [],
        tabs: [
          createTab("Docs", "https://www.chromium.org")
        ],
        activeTabId: null
      }
    ]
  };
}

export function normalizeAddress(value: unknown, searchEngineKey: SearchEngineKey = "google"): string {
  const trimmed = String(value ?? "").trim();
  if (!trimmed) {
    return DEFAULT_URL;
  }

  try {
    const url = new URL(trimmed.includes("://") ? trimmed : `https://${trimmed}`);
    return url.href;
  } catch {
    const engine = SEARCH_ENGINES[searchEngineKey] ?? SEARCH_ENGINES.google;
    return `${engine.url}${encodeURIComponent(trimmed)}`;
  }
}

export function normalizeState(candidateState: PartialBrowserState | null | undefined): BrowserState {
  const fallback = createDefaultState();
  const state = Array.isArray(candidateState?.workspaces) && candidateState.workspaces.length > 0
    ? candidateState as BrowserState
    : fallback;

  state.history = Array.isArray(state.history) ? state.history as HistoryEntry[] : [];
  state.downloads = Array.isArray(state.downloads) ? state.downloads as DownloadEntry[] : [];
  state.sitePermissions = normalizeSitePermissions(state.sitePermissions);
  state.settings = {
    ...fallback.settings,
    ...(state.settings ?? {})
  };

  if (!isSearchEngineKey(state.settings.searchEngine)) {
    state.settings.searchEngine = fallback.settings.searchEngine;
  }

  if (!isStartupBehavior(state.settings.startupBehavior)) {
    state.settings.startupBehavior = fallback.settings.startupBehavior;
  }

  state.settings.homepage = normalizeAddress(state.settings.homepage || DEFAULT_URL, state.settings.searchEngine);

  for (const workspace of state.workspaces) {
    workspace.id = workspace.id ?? createId();
    workspace.name = workspace.name || "Space";
    workspace.accent = isHexColor(workspace.accent) ? workspace.accent : getNextWorkspaceAccent(0);
    Object.assign(workspace, normalizeWorkspaceProfile(workspace));
    workspace.closedTabs = normalizeClosedTabs(workspace.closedTabs);
    workspace.favorites = normalizeFavorites(workspace.favorites, state.settings.searchEngine);
    workspace.tabGroups = normalizeTabGroups(workspace.tabGroups);
    workspace.tabs = Array.isArray(workspace.tabs) ? workspace.tabs.filter(Boolean) as BrowserTab[] : [];

    if (workspace.tabs.length === 0) {
      workspace.tabs.push(createTab("New Tab", getHomepageUrl(state)));
    }

    for (const tab of workspace.tabs) {
      tab.id = tab.id ?? createId();
      tab.url = normalizeAddress(tab.url || getHomepageUrl(state), state.settings.searchEngine);
      tab.title = tab.title || getReadableUrlTitle(tab.url);
      tab.groupId = isKnownTabGroup(workspace.tabGroups, tab.groupId) ? tab.groupId : null;
      tab.canGoBack = Boolean(tab.canGoBack);
      tab.canGoForward = Boolean(tab.canGoForward);
      tab.isMuted = Boolean(tab.isMuted);
      tab.isPinned = Boolean(tab.isPinned);
      tab.isLoading = Boolean(tab.isLoading);
      tab.zoomFactor = normalizeZoomFactor(tab.zoomFactor);
    }

    if (!workspace.activeTabId || !workspace.tabs.some((tab) => tab.id === workspace.activeTabId)) {
      workspace.activeTabId = workspace.tabs[0].id;
    }
  }

  if (!state.workspaces.some((workspace) => workspace.id === state.activeWorkspaceId)) {
    state.activeWorkspaceId = state.workspaces[0].id;
  }

  return state;
}

export function applyStartupBehavior(state: BrowserState): BrowserState {
  if (state.settings.startupBehavior === "restore") {
    return state;
  }

  return {
    ...state,
    splitMode: false,
    splitTabId: null,
    workspaces: state.workspaces.map((workspace) => {
      const tab = createTab("New Tab", getHomepageUrl(state));
      return {
        ...workspace,
        tabs: [tab],
        activeTabId: tab.id,
        tabGroups: []
      };
    })
  };
}

export function normalizeFavorites(
  favorites: Array<Partial<Favorite> | null> | undefined,
  searchEngineKey: SearchEngineKey = "google"
): Favorite[] {
  if (!Array.isArray(favorites)) {
    return [];
  }

  return favorites
    .filter((favorite): favorite is Partial<Favorite> & { url: string } => Boolean(favorite?.url))
    .map((favorite) => {
      const url = normalizeAddress(favorite.url, searchEngineKey);
      return {
        id: favorite.id ?? createId(),
        title: favorite.title || getReadableUrlTitle(url),
        url
      };
    });
}

export function normalizeClosedTabs(closedTabs: Array<Partial<ClosedTab> | null> | undefined): ClosedTab[] {
  if (!Array.isArray(closedTabs)) {
    return [];
  }

  return closedTabs
    .filter((tab): tab is Partial<ClosedTab> & { url: string } => Boolean(tab?.url))
    .map((tab) => ({
      title: tab.title || getReadableUrlTitle(tab.url),
      url: normalizeAddress(tab.url),
      closedAt: Number(tab.closedAt) || Date.now()
    }))
    .slice(0, 25);
}

export function getHomepageUrl(state?: Pick<BrowserState, "settings">): string {
  return normalizeAddress(state?.settings?.homepage || DEFAULT_URL, state?.settings?.searchEngine);
}

export function getSearchUrl(query: string, state?: Pick<BrowserState, "settings">): string {
  const key = state?.settings?.searchEngine;
  const engine = key ? SEARCH_ENGINES[key] : SEARCH_ENGINES.google;
  return `${engine.url}${encodeURIComponent(query)}`;
}

export function getReadableUrlTitle(url: string | undefined): string {
  try {
    return new URL(url ?? "").hostname;
  } catch {
    return url || "Untitled";
  }
}

export function getHostInitial(url: string): string {
  try {
    return new URL(url).hostname.replace(/^www\./, "").slice(0, 1).toUpperCase();
  } catch {
    return "?";
  }
}

export function isFavorite(workspace: Pick<Workspace, "favorites"> | undefined, url: string): boolean {
  return Boolean(workspace?.favorites?.some((favorite) => favorite.url === url));
}

export function getNextWorkspaceAccent(index: number): string {
  return WORKSPACE_ACCENTS[index % WORKSPACE_ACCENTS.length];
}

function isHexColor(value: unknown): value is string {
  return typeof value === "string" && /^#[0-9a-f]{6}$/i.test(value);
}

function isSearchEngineKey(value: unknown): value is SearchEngineKey {
  return typeof value === "string" && value in SEARCH_ENGINES;
}

function isStartupBehavior(value: unknown): value is StartupBehavior {
  return value === "restore" || value === "homepage";
}

function isKnownTabGroup(groups: TabGroup[], groupId: unknown): groupId is string {
  return typeof groupId === "string" && groups.some((group) => group.id === groupId);
}
