import {
  createFavorite,
  createTab,
  getReadableUrlTitle,
  getWorkspaceHomepageUrl,
  resolveFavoriteTab,
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

export function placeTabInFavoritesFolder(workspace: Workspace, tab: BrowserTab) {
  tab.isPinned = false;
  tab.groupId = null;
  pruneEmptyTabGroups(workspace);
  upsertTabFavorite(workspace, tab);
}

export function takeTabFavorite(workspace: Workspace, tab: BrowserTab): Favorite | null {
  const favoriteIndex = workspace.favorites.findIndex((favorite) => (
    favorite.tabId === tab.id || (!favorite.tabId && favorite.url === tab.url)
  ));
  if (favoriteIndex < 0) return null;

  const [favorite] = workspace.favorites.splice(favoriteIndex, 1);
  favorite.tabId = tab.id;
  favorite.url = tab.url;
  favorite.title = tab.title || favorite.title || getReadableUrlTitle(tab.url);
  return favorite;
}

export function moveTabFavoriteToWorkspace(source: Workspace, target: Workspace, tab: BrowserTab) {
  const favorite = takeTabFavorite(source, tab);
  if (!favorite) return;

  mergeFavoriteByUrl(target.favorites, favorite);
}

export function removeTabFromFavoritesFolder(workspace: Workspace, tab: BrowserTab) {
  workspace.favorites = workspace.favorites.filter((favorite) => (
    favorite.tabId !== tab.id && (favorite.tabId || favorite.url !== tab.url)
  ));
}

export function isTabInFavoritesFolder(workspace: Workspace, tab: BrowserTab) {
  return workspace.favorites.some((favorite) => (
    favorite.tabId === tab.id || (!favorite.tabId && favorite.url === tab.url)
  ));
}

export function reorderFavoriteBackingTab(
  workspace: Workspace,
  tabId: string,
  targetTabId: string,
  placement: "before" | "after"
) {
  const favoriteIndex = workspace.favorites.findIndex((favorite) => favorite.tabId === tabId);
  const targetIndex = workspace.favorites.findIndex((favorite) => favorite.tabId === targetTabId);
  if (favoriteIndex < 0 || targetIndex < 0) return;

  const [favorite] = workspace.favorites.splice(favoriteIndex, 1);
  const droppedOnIndex = workspace.favorites.findIndex((candidate) => candidate.tabId === targetTabId);
  const insertIndex = placement === "after" ? droppedOnIndex + 1 : droppedOnIndex;
  workspace.favorites.splice(insertIndex, 0, favorite);
}

function upsertTabFavorite(workspace: Workspace, tab: BrowserTab) {
  const tabBackedFavorite = workspace.favorites.find((favorite) => favorite.tabId === tab.id);
  if (tabBackedFavorite) return;

  const legacyFavorite = workspace.favorites.find((favorite) => !favorite.tabId && favorite.url === tab.url);
  if (legacyFavorite) {
    legacyFavorite.tabId = tab.id;
    legacyFavorite.title = tab.title || legacyFavorite.title || getReadableUrlTitle(tab.url);
    return;
  }

  workspace.favorites.push(createFavorite(tab.title || getReadableUrlTitle(tab.url), tab.url, tab.id));
}

function detachFavoriteBackingTab(
  state: BrowserState,
  workspace: Workspace,
  favorite: Favorite
): BrowserTab | null {
  const tab = resolveFavoriteTab(workspace, favorite);

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
