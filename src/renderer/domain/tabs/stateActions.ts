import {
  BrowserState,
  BrowserTab,
  createFavorite,
  getReadableUrlTitle
} from "../browser";
import { getActiveTab, getActiveWorkspace } from "../browser/selectors";
import { mergeFavoriteByUrl, moveFavoriteBackingTabToWorkspace } from "../common/favoriteTabs";
import { clearSplitView, getSplitTabIds, setSplitTabIds } from "./splitView";
import { pruneEmptyTabGroups } from "./groups";
import { DEFAULT_ZOOM_FACTOR, stepZoomFactor } from "../browser/zoom";
import { updateBrowserState } from "../browser/updateState";
import type { TabDropPlacement } from "./utils";

export function toggleActiveTabPinned(state: BrowserState): BrowserState {
  return toggleTabPinned(state, getActiveTab(getActiveWorkspace(state)).id);
}

export function toggleTabPinned(state: BrowserState, tabId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = draft.workspaces.find((candidate) => candidate.tabs.some((tab) => tab.id === tabId));
    const tab = workspace?.tabs.find((candidate) => candidate.id === tabId);
    if (!workspace || !tab) return;

    tab.isPinned = !tab.isPinned;
    if (tab.isPinned) {
      tab.groupId = null;
      pruneEmptyTabGroups(workspace);
    }
  });
}

export function toggleActiveTabMuted(state: BrowserState): BrowserState {
  return toggleTabMuted(state, getActiveTab(getActiveWorkspace(state)).id);
}

export function toggleTabMuted(state: BrowserState, tabId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const tab = draft.workspaces.flatMap((workspace) => workspace.tabs).find((candidate) => candidate.id === tabId);
    if (!tab) return;

    tab.isMuted = !tab.isMuted;
  });
}

export function sleepTab(state: BrowserState, tabId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = draft.workspaces.find((candidate) => candidate.tabs.some((tab) => tab.id === tabId));
    const tab = workspace?.tabs.find((candidate) => candidate.id === tabId);
    if (!workspace || !tab || workspace.tabs.length <= 1) return;

    if (workspace.activeTabId === tab.id) {
      const fallback = getSleepTabFocusFallback(workspace.tabs, tab.id);
      if (!fallback) return;

      fallback.isSleeping = false;
      fallback.lastActiveAt = Date.now();
      workspace.activeTabId = fallback.id;
    }

    setSplitTabIds(draft, getSplitTabIds(draft).filter((tabId) => tabId !== tab.id));

    tab.isSleeping = true;
    tab.isLoading = false;
    tab.canGoBack = false;
    tab.canGoForward = false;
  });
}

function getSleepTabFocusFallback(tabs: BrowserTab[], tabId: string): BrowserTab | null {
  const index = tabs.findIndex((candidate) => candidate.id === tabId);
  if (index < 0) return null;

  return tabs[index - 1] ?? tabs[index + 1] ?? null;
}

export function sleepTabGroup(state: BrowserState, groupId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = draft.workspaces.find((candidate) => candidate.tabGroups.some((group) => group.id === groupId));
    if (!workspace) return;

    const protectedTabIds = new Set([workspace.activeTabId, ...getSplitTabIds(draft)].filter(Boolean));

    for (const tab of workspace.tabs) {
      if (tab.groupId !== groupId || tab.isPinned || protectedTabIds.has(tab.id)) {
        continue;
      }

      tab.isSleeping = true;
      tab.isLoading = false;
      tab.canGoBack = false;
      tab.canGoForward = false;
    }
  });
}

export function sleepInactiveTabs(state: BrowserState): BrowserState {
  const workspace = getActiveWorkspace(state);
  const visibleTabIds = new Set([workspace.activeTabId, ...getSplitTabIds(state)].filter(Boolean));
  const sleepableTabIds = workspace.tabs
    .filter((tab) => !tab.isSleeping && !tab.isPinned && !visibleTabIds.has(tab.id))
    .map((tab) => tab.id);

  if (sleepableTabIds.length === 0) return state;

  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const sleepableTabs = new Set(sleepableTabIds);

    for (const tab of workspace.tabs) {
      if (sleepableTabs.has(tab.id)) {
        tab.isSleeping = true;
        tab.isLoading = false;
        tab.canGoBack = false;
        tab.canGoForward = false;
      }
    }
  });
}

export function sleepIdleTabs(state: BrowserState, now = Date.now()): BrowserState {
  if (!state.settings.memorySaverEnabled) return state;

  let sleptTabs = 0;
  const next = updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const visibleTabIds = new Set([workspace.activeTabId, ...getSplitTabIds(draft)].filter(Boolean));
    const cutoff = now - draft.settings.memorySaverIdleMinutes * 60_000;

    for (const tab of workspace.tabs) {
      if (tab.isSleeping || tab.isPinned || visibleTabIds.has(tab.id) || tab.lastActiveAt > cutoff) {
        continue;
      }

      tab.isSleeping = true;
      tab.isLoading = false;
      tab.canGoBack = false;
      tab.canGoForward = false;
      sleptTabs += 1;
    }
  });

  return sleptTabs > 0 ? next : state;
}

export function toggleActiveTabFavorite(state: BrowserState): BrowserState {
  return toggleTabFavorite(state, getActiveTab(getActiveWorkspace(state)).id);
}

export function addTabToFavorites(state: BrowserState, tabId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = draft.workspaces.find((candidate) => candidate.tabs.some((tab) => tab.id === tabId));
    const tab = workspace?.tabs.find((candidate) => candidate.id === tabId);
    if (!workspace || !tab) return;
    if (workspace.favorites.some((favorite) => favorite.tabId === tab.id)) return;

    const legacyFavorite = workspace.favorites.find((favorite) => !favorite.tabId && favorite.url === tab.url);
    if (legacyFavorite) {
      legacyFavorite.tabId = tab.id;
      legacyFavorite.title = tab.title || legacyFavorite.title || getReadableUrlTitle(tab.url);
      return;
    }

    workspace.favorites.push(createFavorite(tab.title || getReadableUrlTitle(tab.url), tab.url, tab.id));
  });
}

export function toggleTabFavorite(state: BrowserState, tabId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = draft.workspaces.find((candidate) => candidate.tabs.some((tab) => tab.id === tabId));
    const tab = workspace?.tabs.find((candidate) => candidate.id === tabId);
    if (!workspace || !tab) return;

    const tabBackedIndex = workspace.favorites.findIndex((favorite) => favorite.tabId === tab.id);
    const index = tabBackedIndex >= 0
      ? tabBackedIndex
      : workspace.favorites.findIndex((favorite) => !favorite.tabId && favorite.url === tab.url);
    index >= 0
      ? workspace.favorites.splice(index, 1)
      : workspace.favorites.push(createFavorite(tab.title || getReadableUrlTitle(tab.url), tab.url, tab.id));
  });
}

export function removeWorkspaceFavorite(state: BrowserState, favoriteIdOrUrl: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const removedById = workspace.favorites.some((favorite) => favorite.id === favoriteIdOrUrl);
    workspace.favorites = workspace.favorites.filter((favorite) => (
      removedById ? favorite.id !== favoriteIdOrUrl : favorite.url !== favoriteIdOrUrl
    ));
  });
}

export function moveWorkspaceFavoriteToWorkspace(
  state: BrowserState,
  favoriteId: string,
  workspaceId: string
): BrowserState {
  const source = getActiveWorkspace(state);
  const target = state.workspaces.find((workspace) => workspace.id === workspaceId);
  if (!target || target.id === source.id || !source.favorites.some((favorite) => favorite.id === favoriteId)) {
    return state;
  }

  return updateBrowserState(state, (draft) => {
    const source = getActiveWorkspace(draft);
    const target = draft.workspaces.find((workspace) => workspace.id === workspaceId);
    if (!target || target.id === source.id) return;

    const index = source.favorites.findIndex((favorite) => favorite.id === favoriteId);
    if (index < 0) return;

    const [favorite] = source.favorites.splice(index, 1);
    moveFavoriteBackingTabToWorkspace(draft, source, target, favorite);
    mergeFavoriteByUrl(target.favorites, favorite);
    draft.activeWorkspaceId = target.id;
    clearSplitView(draft);
  });
}

export function reorderEssential(
  state: BrowserState,
  essentialId: string,
  targetEssentialId: string,
  placement: TabDropPlacement
): BrowserState {
  if (essentialId === targetEssentialId) return state;

  return updateBrowserState(state, (draft) => {
    reorderFavoriteList(draft.essentials, essentialId, targetEssentialId, placement);
  });
}

export function reorderWorkspaceFavorite(
  state: BrowserState,
  favoriteId: string,
  targetFavoriteId: string,
  placement: TabDropPlacement
): BrowserState {
  if (favoriteId === targetFavoriteId) return state;

  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    reorderFavoriteList(workspace.favorites, favoriteId, targetFavoriteId, placement);
  });
}

function reorderFavoriteList(
  favorites: Array<{ id: string }>,
  favoriteId: string,
  targetFavoriteId: string,
  placement: TabDropPlacement
) {
  const fromIndex = favorites.findIndex((favorite) => favorite.id === favoriteId);
  const targetIndex = favorites.findIndex((favorite) => favorite.id === targetFavoriteId);
  if (fromIndex < 0 || targetIndex < 0) return;

  const [favorite] = favorites.splice(fromIndex, 1);
  const droppedOnIndex = favorites.findIndex((candidate) => candidate.id === targetFavoriteId);
  const insertIndex = placement === "after" ? droppedOnIndex + 1 : droppedOnIndex;
  favorites.splice(insertIndex, 0, favorite);
}

export function toggleActiveTabEssential(state: BrowserState): BrowserState {
  return toggleTabEssential(state, getActiveTab(getActiveWorkspace(state)).id);
}

export function toggleTabEssential(state: BrowserState, tabId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const tab = draft.workspaces.flatMap((workspace) => workspace.tabs).find((candidate) => candidate.id === tabId);
    if (!tab) return;

    const index = draft.essentials.findIndex((essential) => essential.url === tab.url);
    index >= 0
      ? draft.essentials.splice(index, 1)
      : draft.essentials.push(createFavorite(tab.title || getReadableUrlTitle(tab.url), tab.url));
  });
}

export function removeEssential(state: BrowserState, url: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    draft.essentials = draft.essentials.filter((essential) => essential.url !== url);
  });
}

export function updateTab(state: BrowserState, tabId: string, patch: Partial<BrowserTab>): BrowserState {
  return updateBrowserState(state, (draft) => {
    const tab = draft.workspaces.flatMap((workspace) => workspace.tabs).find((candidate) => candidate.id === tabId);
    if (tab) {
      Object.assign(tab, patch);
      if (patch.url || patch.isLoading) {
        tab.isSleeping = false;
        tab.lastActiveAt = Date.now();
      }
    }
  });
}

export function setActiveTabZoom(state: BrowserState, zoomFactor: number): BrowserState {
  return updateBrowserState(state, (draft) => {
    getActiveTab(getActiveWorkspace(draft)).zoomFactor = zoomFactor;
  });
}

export function stepActiveTabZoom(state: BrowserState, direction: 1 | -1): BrowserState {
  return updateBrowserState(state, (draft) => {
    const tab = getActiveTab(getActiveWorkspace(draft));
    tab.zoomFactor = stepZoomFactor(tab.zoomFactor, direction);
  });
}

export function resetActiveTabZoom(state: BrowserState): BrowserState {
  return setActiveTabZoom(state, DEFAULT_ZOOM_FACTOR);
}
