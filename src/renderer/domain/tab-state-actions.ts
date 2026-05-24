import {
  BrowserState,
  BrowserTab,
  createFavorite,
  getReadableUrlTitle
} from "./browser-core";
import { getActiveTab, getActiveWorkspace } from "./selectors";
import { pruneEmptyTabGroups } from "./tab-groups";
import { DEFAULT_ZOOM_FACTOR, stepZoomFactor } from "./zoom";
import { updateBrowserState } from "./action-core";

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

export function toggleActiveTabFavorite(state: BrowserState): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const tab = getActiveTab(workspace);
    const index = workspace.favorites.findIndex((favorite) => favorite.url === tab.url);
    index >= 0
      ? workspace.favorites.splice(index, 1)
      : workspace.favorites.push(createFavorite(tab.title || getReadableUrlTitle(tab.url), tab.url));
  });
}

export function updateTab(state: BrowserState, tabId: string, patch: Partial<BrowserTab>): BrowserState {
  return updateBrowserState(state, (draft) => {
    const tab = draft.workspaces.flatMap((workspace) => workspace.tabs).find((candidate) => candidate.id === tabId);
    if (tab) Object.assign(tab, patch);
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
