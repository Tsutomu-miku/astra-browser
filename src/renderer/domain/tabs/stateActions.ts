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
