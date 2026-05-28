import {
  BrowserState,
  BrowserTab,
  createFavorite,
  getReadableUrlTitle
} from "../browser";
import { getActiveTab, getActiveWorkspace } from "../browser/selectors";
import { getSplitTabIds, setSplitTabIds } from "./splitView";
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
      const index = workspace.tabs.findIndex((candidate) => candidate.id === tab.id);
      workspace.activeTabId = workspace.tabs[Math.max(0, index - 1)].id;
    }

    setSplitTabIds(draft, getSplitTabIds(draft).filter((tabId) => tabId !== tab.id));

    tab.isSleeping = true;
    tab.isLoading = false;
    tab.canGoBack = false;
    tab.canGoForward = false;
  });
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
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const visibleTabIds = new Set([workspace.activeTabId, ...getSplitTabIds(draft)].filter(Boolean));

    for (const tab of workspace.tabs) {
      if (!visibleTabIds.has(tab.id) && !tab.isPinned) {
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

export function toggleTabFavorite(state: BrowserState, tabId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = draft.workspaces.find((candidate) => candidate.tabs.some((tab) => tab.id === tabId));
    const tab = workspace?.tabs.find((candidate) => candidate.id === tabId);
    if (!workspace || !tab) return;

    const index = workspace.favorites.findIndex((favorite) => favorite.url === tab.url);
    index >= 0
      ? workspace.favorites.splice(index, 1)
      : workspace.favorites.push(createFavorite(tab.title || getReadableUrlTitle(tab.url), tab.url));
  });
}

export function removeWorkspaceFavorite(state: BrowserState, url: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    workspace.favorites = workspace.favorites.filter((favorite) => favorite.url !== url);
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
