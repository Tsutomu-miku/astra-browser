import {
  BrowserState,
  BrowserTab,
  createFavorite,
  getCachedFaviconUrl,
  getReadableUrlTitle
} from "../browser";
import { getActiveTab, getActiveWorkspace } from "../browser/selectors";
import { placeTabInFavoritesFolder, removeTabFromFavoritesFolder, reorderFavoriteTab, moveFavoriteTabToWorkspace } from "../common/favoriteTabs";
import { clearSplitView } from "./splitView";
import { pruneEmptyTabGroups } from "./groups";
import { moveTabToFolder } from "./folderActions";
import { DEFAULT_ZOOM_FACTOR, stepZoomFactor } from "../browser/zoom";
import { updateBrowserState } from "../browser/updateState";
import { normalizeFaviconUrl, setCachedFaviconUrl } from "../browser/favicon";
import type { TabDropPlacement } from "./utils";

// Tab sleep actions are split into sleepActions.ts; re-export here so the
// public import surface of the tabs domain stays unchanged.
export { sleepIdleTabs, sleepInactiveTabs, sleepTab, sleepTabGroup } from "./sleepActions";

export function toggleActiveTabPinned(state: BrowserState): BrowserState {
  return toggleTabPinned(state, getActiveTab(getActiveWorkspace(state)).id);
}

export function toggleTabPinned(state: BrowserState, tabId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = draft.workspaces.find((candidate) => candidate.tabs.some((tab) => tab.id === tabId));
    const tab = workspace?.tabs.find((candidate) => candidate.id === tabId);
    if (!workspace || !tab) return;

    moveTabToFolder(workspace, tab, tab.isPinned ? { type: "tabs" } : { type: "pinned" });
    pruneEmptyTabGroups(workspace);
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
  return toggleTabFavorite(state, getActiveTab(getActiveWorkspace(state)).id);
}

export function addTabToFavorites(state: BrowserState, tabId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = draft.workspaces.find((candidate) => candidate.tabs.some((tab) => tab.id === tabId));
    const tab = workspace?.tabs.find((candidate) => candidate.id === tabId);
    if (!workspace || !tab) return;
    placeTabInFavoritesFolder(workspace, tab);
  });
}

export function toggleTabFavorite(state: BrowserState, tabId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = draft.workspaces.find((candidate) => candidate.tabs.some((tab) => tab.id === tabId));
    const tab = workspace?.tabs.find((candidate) => candidate.id === tabId);
    if (!workspace || !tab) return;

    if (tab.isFavorite) {
      removeTabFromFavoritesFolder(workspace, tab);
    } else {
      placeTabInFavoritesFolder(workspace, tab);
    }
  });
}

export function removeWorkspaceFavorite(state: BrowserState, tabId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = draft.workspaces.find((w) => w.tabs.some((t) => t.id === tabId));
    const tab = workspace?.tabs.find((t) => t.id === tabId);
    if (!workspace || !tab) return;
    removeTabFromFavoritesFolder(workspace, tab);
  });
}

export function moveWorkspaceFavoriteToWorkspace(
  state: BrowserState,
  tabId: string,
  workspaceId: string
): BrowserState {
  const source = getActiveWorkspace(state);
  const target = state.workspaces.find((workspace) => workspace.id === workspaceId);
  if (!target || target.id === source.id || !source.tabs.some((tab) => tab.id === tabId)) {
    return state;
  }

  return updateBrowserState(state, (draft) => {
    const source = getActiveWorkspace(draft);
    const target = draft.workspaces.find((workspace) => workspace.id === workspaceId);
    const tab = source.tabs.find((t) => t.id === tabId);
    if (!target || target.id === source.id || !tab) return;

    moveFavoriteTabToWorkspace(draft, source, target, tab);
    draft.activeWorkspaceId = target.id;
    clearSplitView(draft);
  });
}

function reorderById<T extends { id: string }>(
  list: T[],
  id: string,
  targetId: string,
  placement: TabDropPlacement
) {
  const fromIndex = list.findIndex((item) => item.id === id);
  const targetIndex = list.findIndex((item) => item.id === targetId);
  if (fromIndex < 0 || targetIndex < 0) return;

  const [item] = list.splice(fromIndex, 1);
  const droppedOnIndex = list.findIndex((candidate) => candidate.id === targetId);
  const insertIndex = placement === "after" ? droppedOnIndex + 1 : droppedOnIndex;
  list.splice(insertIndex, 0, item);
}

export function reorderEssential(
  state: BrowserState,
  essentialId: string,
  targetEssentialId: string,
  placement: TabDropPlacement
): BrowserState {
  if (essentialId === targetEssentialId) return state;

  return updateBrowserState(state, (draft) => {
    reorderById(draft.essentials, essentialId, targetEssentialId, placement);
  });
}

export function reorderWorkspaceFavorite(
  state: BrowserState,
  tabId: string,
  targetTabId: string,
  placement: TabDropPlacement
): BrowserState {
  if (tabId === targetTabId) return state;

  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    reorderFavoriteTab(workspace, tabId, targetTabId, placement);
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
      const faviconUrl = normalizeFaviconUrl(patch.faviconUrl);
      if (faviconUrl) {
        tab.faviconUrl = faviconUrl;
        setCachedFaviconUrl(draft.faviconCache, tab.url, faviconUrl);
      } else if (patch.url) {
        const cachedFaviconUrl = getCachedFaviconUrl(draft.faviconCache, patch.url);
        cachedFaviconUrl ? tab.faviconUrl = cachedFaviconUrl : delete tab.faviconUrl;
      }
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
