import { DEFAULT_URL, SEARCH_ENGINES } from "./browser-constants";
import { createDefaultState, createId, createTab, getNextWorkspaceAccent } from "./browser-factory";
import type {
  BrowserState,
  BrowserTab,
  ClosedTab,
  DownloadEntry,
  Favorite,
  HistoryEntry,
  PartialBrowserState,
  SearchEngineKey,
  StartupBehavior,
  TabGroup
} from "./browser-types";
import { getHomepageUrl, getReadableUrlTitle, normalizeAddress } from "./navigation";
import { normalizeSitePermissions } from "./sitePermissions";
import { normalizeTabGroups } from "./tab-groups";
import { normalizeWorkspaceProfile } from "./workspaceProfiles";
import { normalizeZoomFactor } from "./zoom";

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
