import {
  type BrowserState,
  type BrowserTab,
  type TabGroup,
  type Workspace
} from "../browser";
import { pruneEmptyTabGroups } from "../tabs/groups";

/**
 * Insert a tab into the favorites folder. If the tab belongs to a group,
 * every sibling in that group is marked favorite as well so the entire
 * group moves into the favorites section together. Pinned tabs are unpinned
 * because pinned and favorites are mutually exclusive.
 */
export function placeTabInFavoritesFolder(workspace: Workspace, tab: BrowserTab) {
  tab.isPinned = false;
  tab.isFavorite = true;
  if (!workspace.favoriteOrder.includes(tab.id)) {
    workspace.favoriteOrder.push(tab.id);
  }
  const groupId = tab.groupId;
  if (groupId) {
    for (const sibling of workspace.tabs) {
      if (sibling.groupId === groupId && sibling.id !== tab.id) {
        sibling.isFavorite = true;
        if (!workspace.favoriteOrder.includes(sibling.id)) {
          workspace.favoriteOrder.push(sibling.id);
        }
      }
    }
  }
}

/**
 * Remove a tab from the favorites folder. If the tab belongs to a group,
 * every sibling in that group is also removed from favorites so the
 * group stays coherent inside the Tabs section.
 */
export function removeTabFromFavoritesFolder(workspace: Workspace, tab: BrowserTab) {
  const groupId = tab.groupId;
  tab.isFavorite = false;
  if (groupId) {
    const groupTabIds = new Set<string>();
    for (const sibling of workspace.tabs) {
      if (sibling.groupId === groupId) {
        sibling.isFavorite = false;
        groupTabIds.add(sibling.id);
      }
    }
    groupTabIds.add(tab.id);
    workspace.favoriteOrder = workspace.favoriteOrder.filter((id) => !groupTabIds.has(id));
  } else {
    workspace.favoriteOrder = workspace.favoriteOrder.filter((id) => id !== tab.id);
  }
}

/**
 * True when the tab is currently a member of the favorites folder.
 */
export function isTabInFavoritesFolder(_workspace: Workspace, tab: BrowserTab) {
  return tab.isFavorite;
}

/**
 * Returns true when every tab in the group is marked as a favorite.
 * Groups with no tabs are considered non-favorite.
 */
export function isFavoriteGroup(workspace: Workspace, group: TabGroup): boolean {
  const members = workspace.tabs.filter((tab) => tab.groupId === group.id);
  return members.length > 0 && members.every((tab) => tab.isFavorite);
}

/**
 * Reorder a favorite tab relative to another favorite tab by id within
 * workspace.favoriteOrder.
 */
export function reorderFavoriteTab(
  workspace: Workspace,
  tabId: string,
  targetTabId: string,
  placement: "before" | "after"
) {
  const fromIndex = workspace.favoriteOrder.indexOf(tabId);
  const targetIndex = workspace.favoriteOrder.indexOf(targetTabId);
  if (fromIndex < 0 || targetIndex < 0) return;

  const [id] = workspace.favoriteOrder.splice(fromIndex, 1);
  const droppedOnIndex = workspace.favoriteOrder.indexOf(targetTabId);
  const insertIndex = placement === "after" ? droppedOnIndex + 1 : droppedOnIndex;
  workspace.favoriteOrder.splice(insertIndex, 0, id);
}

/**
 * Remove invalid entries from favoriteOrder: ids that refer to tabs which no
 * longer exist or which are no longer marked as favorites.
 */
export function pruneFavoriteOrder(workspace: Workspace) {
  const validIds = new Set(
    workspace.tabs.filter((tab) => tab.isFavorite).map((tab) => tab.id)
  );
  workspace.favoriteOrder = workspace.favoriteOrder.filter((id) => validIds.has(id));
}

/**
 * Move a favorite tab (and its whole group, if any) from one workspace to
 * another. Used by cross-workspace favorite drag/move flows.
 */
export function moveFavoriteTabToWorkspace(
  state: BrowserState,
  source: Workspace,
  target: Workspace,
  tab: BrowserTab
) {
  const groupId = tab.groupId;
  const movingTabs: BrowserTab[] = groupId
    ? source.tabs.filter((t) => t.groupId === groupId)
    : [tab];
  const movingIds = new Set(movingTabs.map((t) => t.id));

  for (const movingTab of movingTabs) {
    const tabIndex = source.tabs.findIndex((t) => t.id === movingTab.id);
    if (tabIndex >= 0) source.tabs.splice(tabIndex, 1);
  }
  source.favoriteOrder = source.favoriteOrder.filter((id) => !movingIds.has(id));
  pruneEmptyTabGroups(source);

  if (source.tabs.length === 0) {
    const { getWorkspaceHomepageUrl, createTab } = require("../browser") as typeof import("../browser");
    const replacement = createTab("New Tab", getWorkspaceHomepageUrl(state, source));
    source.tabs.push(replacement);
    source.activeTabId = replacement.id;
  } else if (source.activeTabId === null || !source.tabs.some((t) => t.id === source.activeTabId)) {
    source.activeTabId = source.tabs[0].id;
  } else if (movingIds.has(source.activeTabId)) {
    source.activeTabId = source.tabs[0].id;
  }

  for (const movingTab of movingTabs) {
    movingTab.isPinned = false;
    movingTab.isFavorite = true;
    target.tabs.push(movingTab);
    if (!target.favoriteOrder.includes(movingTab.id)) {
      target.favoriteOrder.push(movingTab.id);
    }
  }
  target.activeTabId = tab.id;
}
