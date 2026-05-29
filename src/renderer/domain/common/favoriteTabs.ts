import {
  createTab,
  getReadableUrlTitle,
  getWorkspaceHomepageUrl,
  type BrowserState,
  type BrowserTab,
  type Favorite,
  type Workspace
} from "../browser";
import { pruneEmptyTabGroups } from "../tabs/groups";

export function moveFavoriteBackingTabToWorkspace(
  state: BrowserState,
  source: Workspace,
  target: Workspace,
  favorite: Favorite
): BrowserTab {
  const tab = takeFavoriteBackingTab(state, source, favorite);
  target.tabs.push(tab);
  target.activeTabId = tab.id;

  return tab;
}

export function takeFavoriteBackingTab(state: BrowserState, source: Workspace, favorite: Favorite): BrowserTab {
  const tab = detachFavoriteBackingTab(state, source, favorite) ?? createTabForFavorite(favorite);
  favorite.tabId = tab.id;
  return tab;
}

export function createTabForFavorite(favorite: Favorite): BrowserTab {
  return createTab(favorite.title || getReadableUrlTitle(favorite.url), favorite.url);
}

export function mergeFavoriteByUrl(favorites: Favorite[], favorite: Favorite): Favorite {
  const existing = favorites.find((candidate) => candidate.url === favorite.url);
  if (!existing) {
    favorites.push(favorite);
    return favorite;
  }

  existing.title = favorite.title || existing.title;
  existing.tabId = favorite.tabId;
  return existing;
}

function detachFavoriteBackingTab(
  state: BrowserState,
  workspace: Workspace,
  favorite: Favorite
): BrowserTab | null {
  const tab =
    (favorite.tabId ? workspace.tabs.find((candidate) => candidate.id === favorite.tabId) : null) ??
    workspace.tabs.find((candidate) => candidate.url === favorite.url);

  return tab ? detachTabFromWorkspace(state, workspace, tab.id) : null;
}

function detachTabFromWorkspace(state: BrowserState, workspace: Workspace, tabId: string): BrowserTab | null {
  const index = workspace.tabs.findIndex((candidate) => candidate.id === tabId);
  if (index < 0) return null;

  const [tab] = workspace.tabs.splice(index, 1);
  tab.groupId = null;
  pruneEmptyTabGroups(workspace);
  replaceEmptyOrMovedActiveTab(state, workspace, tab.id, index);

  return tab;
}

function replaceEmptyOrMovedActiveTab(
  state: BrowserState,
  workspace: Workspace,
  movedTabId: string,
  movedIndex: number
) {
  if (workspace.tabs.length === 0) {
    const replacement = createTab("New Tab", getWorkspaceHomepageUrl(state, workspace));
    workspace.tabs.push(replacement);
    workspace.activeTabId = replacement.id;
  } else if (workspace.activeTabId === movedTabId || !workspace.tabs.some((tab) => tab.id === workspace.activeTabId)) {
    workspace.activeTabId = workspace.tabs[Math.min(movedIndex, workspace.tabs.length - 1)].id;
  }
}
