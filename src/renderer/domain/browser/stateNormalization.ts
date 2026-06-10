import { DEFAULT_URL, SEARCH_ENGINES } from "./constants";
import { createDefaultState, createId, createTab, getNextWorkspaceAccent } from "./factory";
import type {
  BrowserState,
  BrowserTab,
  ClosedTab,
  DownloadEntry,
  Favorite,
  HistoryEntry,
  PartialBrowserState,
  SearchEngineKey,
  SplitLayout,
  StartupBehavior,
  TabGroup,
  ThemeKey
} from "./types";
import { isThemeKey } from "../../common/theme/themePalette";
import { getHomepageUrl, getReadableUrlTitle, getWorkspaceHomepageUrl, normalizeAddress } from "./navigation";
import { normalizeSitePermissions } from "../permissions/sitePermissions";
import { getSplitTabIds, pruneSplitTabIds } from "../tabs/splitView";
import { normalizeTabGroups } from "../tabs/groups";
import { normalizeWorkspaceProfile } from "../workspaces/profiles";
import { normalizeZoomFactor, DEFAULT_ZOOM_FACTOR } from "./zoom";
import { normalizePerOriginZoom } from "./perOriginZoom";
import { getCachedFaviconUrl, normalizeFaviconCache, normalizeFaviconUrl, setCachedFaviconUrl } from "./favicon";

export function normalizeState(candidateState: PartialBrowserState | null | undefined): BrowserState {
  const fallback = createDefaultState();
  const state = Array.isArray(candidateState?.workspaces) && candidateState.workspaces.length > 0
    ? candidateState as BrowserState
    : fallback;
  state.history = Array.isArray(state.history) ? state.history as HistoryEntry[] : [];
  state.downloads = Array.isArray(state.downloads) ? state.downloads as DownloadEntry[] : [];
  state.essentials = normalizeFavorites(state.essentials, state.settings?.searchEngine);
  state.faviconCache = normalizeFaviconCache(state.faviconCache);
  state.sitePermissions = normalizeSitePermissions(state.sitePermissions);
  state.settings = { ...fallback.settings, ...(state.settings ?? {}) };
  if (!isSearchEngineKey(state.settings.searchEngine)) state.settings.searchEngine = fallback.settings.searchEngine;
  if (!isStartupBehavior(state.settings.startupBehavior)) state.settings.startupBehavior = fallback.settings.startupBehavior;
  if (!isChromeAccentMode(state.settings.chromeAccentMode)) state.settings.chromeAccentMode = fallback.settings.chromeAccentMode;
  if (!isThemeKey(state.settings.theme)) state.settings.theme = fallback.settings.theme;
  state.settings.memorySaverEnabled = state.settings.memorySaverEnabled !== false;
  state.settings.memorySaverIdleMinutes = normalizeMemorySaverIdleMinutes(state.settings.memorySaverIdleMinutes);
  state.settings.homepage = normalizeAddress(state.settings.homepage || DEFAULT_URL, state.settings.searchEngine);
  state.settings.defaultZoomFactor = normalizeDefaultZoomFactor(state.settings.defaultZoomFactor);
  state.settings.incognito = state.settings.incognito === "in-memory" ? "in-memory" : "disabled";
  state.settings.perOriginZoom = normalizePerOriginZoom(state.settings.perOriginZoom);
  for (const workspace of state.workspaces) {
    workspace.id = workspace.id ?? createId();
    workspace.name = workspace.name || "Space";
    workspace.accent = isHexColor(workspace.accent) ? workspace.accent : getNextWorkspaceAccent(0);
    workspace.homepage = getWorkspaceHomepageUrl(state, workspace);
    workspace.splitLayout = isSplitLayout(workspace.splitLayout) ? workspace.splitLayout : "horizontal";
    Object.assign(workspace, normalizeWorkspaceProfile(workspace));
    workspace.closedTabs = normalizeClosedTabs(workspace.closedTabs);
    workspace.tabGroups = normalizeTabGroups(workspace.tabGroups);
    workspace.tabs = Array.isArray(workspace.tabs) ? workspace.tabs.filter(Boolean) as BrowserTab[] : [];
    if (workspace.tabs.length === 0) workspace.tabs.push(createTab("New Tab", workspace.homepage));
    for (const tab of workspace.tabs) {
      tab.id = tab.id ?? createId();
      tab.url = normalizeAddress(tab.url || getHomepageUrl(state), state.settings.searchEngine);
      tab.title = tab.title || getReadableUrlTitle(tab.url);
      const faviconUrl = normalizeFaviconUrl(tab.faviconUrl);
      if (faviconUrl) {
        tab.faviconUrl = faviconUrl;
        setCachedFaviconUrl(state.faviconCache, tab.url, faviconUrl);
      } else if (getCachedFaviconUrl(state.faviconCache, tab.url)) {
        tab.faviconUrl = getCachedFaviconUrl(state.faviconCache, tab.url);
      } else {
        delete tab.faviconUrl;
      }
      tab.groupId = isKnownTabGroup(workspace.tabGroups, tab.groupId) ? tab.groupId : null;
      tab.canGoBack = Boolean(tab.canGoBack);
      tab.canGoForward = Boolean(tab.canGoForward);
      tab.isFavorite = Boolean(tab.isFavorite);
      tab.isMuted = Boolean(tab.isMuted);
      tab.isPinned = Boolean(tab.isPinned);
      // Transient runtime flags always start false on startup — they describe
      // a live <webview>'s state, not something that survives a restart.
      tab.isLoading = false;
      tab.isMediaPlaying = false;
      tab.isCameraOn = false;
      tab.isMicrophoneOn = false;
      tab.hasUnread = false;
      tab.isSleeping = Boolean(tab.isSleeping);
      if (typeof tab.customTitle !== "string" || tab.customTitle.trim() === "") delete tab.customTitle;
      tab.lastActiveAt = normalizeTimestamp(tab.lastActiveAt);
      tab.zoomFactor = normalizeZoomFactor(tab.zoomFactor);
    }
    // Migration: old workspace.favorites[] → tab.isFavorite + workspace.favoriteOrder[].
    // Legacy URL-only favorites (no tabId) are dropped — they are redundant with
    // Essentials. Favorites whose tabId no longer exists are also dropped.
    const validTabIds = new Set(workspace.tabs.map((tab) => tab.id));
    const seenIds = new Set<string>();
    const migratedOrder: string[] = [];
    // 1. Migrate from old favorites[] array.
    const oldFavorites = (workspace as Partial<{ favorites: Array<{ tabId?: string } | null> | undefined }>).favorites;
    if (Array.isArray(oldFavorites)) {
      for (const fav of oldFavorites) {
        if (!fav?.tabId || !validTabIds.has(fav.tabId) || seenIds.has(fav.tabId)) continue;
        seenIds.add(fav.tabId);
        migratedOrder.push(fav.tabId);
        const tab = workspace.tabs.find((t) => t.id === fav.tabId);
        if (tab) tab.isFavorite = true;
      }
    }
    // 2. Merge with existing favoriteOrder (from new-format persisted state).
    const existingOrder = (workspace as { favoriteOrder?: unknown[] }).favoriteOrder;
    if (Array.isArray(existingOrder)) {
      for (const id of existingOrder) {
        if (typeof id !== "string" || !validTabIds.has(id) || seenIds.has(id)) continue;
        seenIds.add(id);
        migratedOrder.push(id);
      }
    }
    // 3. Catch any tabs that have isFavorite=true but are missing from the order.
    for (const tab of workspace.tabs) {
      if (tab.isFavorite && !seenIds.has(tab.id)) {
        migratedOrder.push(tab.id);
        seenIds.add(tab.id);
      }
    }
    // 4. Enforce invariants.
    //    - Pinned tabs cannot be favorite or grouped.
    //    - All tabs in one group share the same `isFavorite` value. Mixed
    //      groups are coerced to non-favorite so they land in the Tabs section
    //      rather than being split across sections.
    for (const tab of workspace.tabs) {
      if (tab.isPinned) {
        tab.isFavorite = false;
        tab.groupId = null;
      }
    }
    for (const group of workspace.tabGroups) {
      const members = workspace.tabs.filter((tab) => tab.groupId === group.id);
      if (members.length === 0) continue;
      const allFavorite = members.every((tab) => tab.isFavorite);
      if (!allFavorite) for (const member of members) member.isFavorite = false;
    }
    workspace.favoriteOrder = migratedOrder.filter((id) => Boolean(workspace.tabs.find((t) => t.id === id)?.isFavorite));
    // Clean up the legacy key so it doesn't pollute re-serialized state.
    delete (workspace as { favorites?: unknown }).favorites;
    if (!workspace.activeTabId || !workspace.tabs.some((tab) => tab.id === workspace.activeTabId)) {
      workspace.activeTabId = workspace.tabs[0].id;
    }
  }
  if (!state.workspaces.some((workspace) => workspace.id === state.activeWorkspaceId)) {
    state.activeWorkspaceId = state.workspaces[0].id;
  }
  state.splitMode = Boolean(state.splitMode);
  state.splitTabIds = getSplitTabIds({
    splitMode: state.splitMode,
    splitTabId: state.splitTabId,
    splitTabIds: Array.isArray(state.splitTabIds) ? state.splitTabIds : []
  });
  state.splitTabId = state.splitTabIds[0] ?? null;
  pruneSplitTabIds(state, state.workspaces.find((workspace) => workspace.id === state.activeWorkspaceId) ?? state.workspaces[0]);
  return state;
}

export function applyStartupBehavior(state: BrowserState): BrowserState {
  if (state.settings.startupBehavior === "restore") return state;
  return {
    ...state,
    splitMode: false,
    splitTabId: null,
    splitTabIds: [],
    workspaces: state.workspaces.map((workspace) => {
      const tab = createTab("New Tab", getWorkspaceHomepageUrl(state, workspace));
      return { ...workspace, tabs: [tab], activeTabId: tab.id, tabGroups: [], favoriteOrder: [] };
    })
  };
}

export function normalizeFavorites(
  favorites: Array<Partial<Favorite> | null> | undefined,
  searchEngineKey: SearchEngineKey = "google"
): Favorite[] {
  if (!Array.isArray(favorites)) return [];
  return favorites
    .filter((favorite): favorite is Partial<Favorite> & { url: string } => Boolean(favorite?.url))
    .map((favorite) => {
      const url = normalizeAddress(favorite.url, searchEngineKey);
      return {
        id: favorite.id ?? createId(),
        title: favorite.title || getReadableUrlTitle(url),
        ...(typeof favorite.tabId === "string" ? { tabId: favorite.tabId } : {}),
        url
      };
    });
}

export function normalizeClosedTabs(closedTabs: Array<Partial<ClosedTab> | null> | undefined): ClosedTab[] {
  if (!Array.isArray(closedTabs)) return [];
  return closedTabs
    .filter((tab): tab is Partial<ClosedTab> & { url: string } => Boolean(tab?.url))
    .map((tab) => {
      const url = normalizeAddress(tab.url);
      return {
        title: tab.title || getReadableUrlTitle(url),
        ...(typeof tab.customTitle === "string" && tab.customTitle.trim() !== "" ? { customTitle: tab.customTitle } : {}),
        url,
        ...(normalizeFaviconUrl(tab.faviconUrl) ? { faviconUrl: normalizeFaviconUrl(tab.faviconUrl)! } : {}),
        groupId: typeof tab.groupId === "string" ? tab.groupId : null,
        canGoBack: Boolean(tab.canGoBack),
        canGoForward: Boolean(tab.canGoForward),
        isMuted: Boolean(tab.isMuted),
        isPinned: Boolean(tab.isPinned),
        zoomFactor: normalizeZoomFactor(tab.zoomFactor),
        closedAt: Number(tab.closedAt) || Date.now()
      };
    })
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
function isChromeAccentMode(value: unknown): value is BrowserState["settings"]["chromeAccentMode"] {
  return value === "neutral" || value === "space";
}
function normalizeTimestamp(value: unknown): number {
  const timestamp = Number(value);
  return Number.isFinite(timestamp) && timestamp > 0 ? timestamp : Date.now();
}
function normalizeMemorySaverIdleMinutes(value: unknown): number {
  const minutes = Number(value);
  if (!Number.isFinite(minutes)) return 30;
  return Math.min(240, Math.max(1, Math.round(minutes)));
}
function isKnownTabGroup(groups: TabGroup[], groupId: unknown): groupId is string {
  return typeof groupId === "string" && groups.some((group) => group.id === groupId);
}
function isSplitLayout(value: unknown): value is SplitLayout {
  return value === "horizontal" || value === "vertical" || value === "grid";
}
function normalizeDefaultZoomFactor(value: unknown): number {
  const factor = Number(value);
  if (!Number.isFinite(factor)) return DEFAULT_ZOOM_FACTOR;
  return normalizeZoomFactor(factor);
}
