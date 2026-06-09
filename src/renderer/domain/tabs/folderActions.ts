import {
  BrowserState,
  BrowserTab,
  createTab,
  getWorkspaceHomepageUrl,
  Workspace
} from "../browser";
import {
  isTabInFavoritesFolder,
  placeTabInFavoritesFolder,
  removeTabFromFavoritesFolder,
  reorderFavoriteTab
} from "../common/favoriteTabs";
import { getActiveWorkspace } from "../browser/selectors";
import { updateBrowserState } from "../browser/updateState";
import { clearSplitView } from "./splitView";
import { pruneEmptyTabGroups } from "./groups";
import type { TabDropPlacement } from "./utils";

/**
 * The folder a tab belongs to.
 *
 * `favorites` and `tabs` denote the top-level sidebar section the tab lives
 * in. `group` means the tab lives inside a tab group inside the `tabs`
 * section (unless every member of that group is marked favorite, in which
 * case the whole group renders inside the `favorites` section). `pinned` is
 * a subset of the `tabs` section reserved for pinned individual tabs.
 */
export type TabFolder =
  | { type: "favorites" }
  | { type: "group"; groupId: string }
  | { type: "pinned" }
  | { type: "tabs" };

export function reorderTab(
  state: BrowserState,
  tabId: string,
  targetTabId: string,
  placement: TabDropPlacement
): BrowserState {
  if (tabId === targetTabId) return state;

  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const fromIndex = workspace.tabs.findIndex((tab) => tab.id === tabId);
    const targetIndex = workspace.tabs.findIndex((tab) => tab.id === targetTabId);
    if (fromIndex < 0 || targetIndex < 0) return;

    const [tab] = workspace.tabs.splice(fromIndex, 1);
    const droppedOnIndex = workspace.tabs.findIndex((candidate) => candidate.id === targetTabId);
    const insertIndex = placement === "after" ? droppedOnIndex + 1 : droppedOnIndex;
    workspace.tabs.splice(insertIndex, 0, tab);
  });
}

export function moveTabToFolderPosition(
  state: BrowserState,
  tabId: string,
  targetTabId: string,
  placement: TabDropPlacement
): BrowserState {
  if (tabId === targetTabId) return state;

  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const fromIndex = workspace.tabs.findIndex((tab) => tab.id === tabId);
    const targetIndex = workspace.tabs.findIndex((tab) => tab.id === targetTabId);
    if (fromIndex < 0 || targetIndex < 0) return;

    const targetTab = workspace.tabs[targetIndex];
    const targetFolder = getTabFolder(workspace, targetTab);

    // before/after 只负责"移动到目标 tab 所在的 section"（favorites/tabs/pinned），
    // 不会把 tab 塞进某个 group——进组是 onto 的语义，走 onGroupTab → groupTabsTogether。
    // 所以如果目标 folder 是具体的 group，降级到该 group 所在的顶层 section。
    const sectionFolder: TabFolder = targetFolder.type === "group"
      ? (targetTab.isFavorite ? { type: "favorites" } : { type: "tabs" })
      : targetFolder;

    const [tab] = workspace.tabs.splice(fromIndex, 1);
    if (!moveTabToFolder(workspace, tab, sectionFolder)) {
      workspace.tabs.splice(fromIndex, 0, tab);
      return;
    }
    pruneEmptyTabGroups(workspace);

    // 如果目标 tab 在某个 group 中，插入位置应放在 group 的边界外侧。
    let anchorId = targetTabId;
    if (targetTab.groupId) {
      const siblings = workspace.tabs.filter((sibling) => sibling.groupId === targetTab.groupId);
      if (siblings.length > 0) {
        anchorId = placement === "after"
          ? siblings[siblings.length - 1].id
          : siblings[0].id;
      }
    }
    const droppedOnIndex = workspace.tabs.findIndex((candidate) => candidate.id === anchorId);
    const insertIndex = droppedOnIndex < 0
      ? workspace.tabs.length
      : (placement === "after" ? droppedOnIndex + 1 : droppedOnIndex);
    workspace.tabs.splice(insertIndex, 0, tab);
    if (tab.isFavorite) {
      reorderFavoriteTab(workspace, tab.id, targetTabId, placement);
    }
  });
}

export function moveTabToFolderEnd(
  state: BrowserState,
  tabId: string,
  folder: TabFolder
): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const fromIndex = workspace.tabs.findIndex((tab) => tab.id === tabId);
    if (fromIndex < 0) return;

    const [tab] = workspace.tabs.splice(fromIndex, 1);
    if (!moveTabToFolder(workspace, tab, folder)) {
      workspace.tabs.splice(fromIndex, 0, tab);
      return;
    }
    pruneEmptyTabGroups(workspace);
    workspace.tabs.push(tab);
  });
}

export function moveTabToFavoritePosition(
  state: BrowserState,
  tabId: string,
  targetTabId: string,
  placement: TabDropPlacement
): BrowserState {
  if (!tabId || !targetTabId) return state;

  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const tabIndex = workspace.tabs.findIndex((tab) => tab.id === tabId);
    if (tabIndex < 0) return;
    if (!workspace.favoriteOrder.includes(targetTabId)) return;

    const [tab] = workspace.tabs.splice(tabIndex, 1);
    // before/after 永远只是排序+进入 favorites folder，不会把 tab 塞进某个现有 group
    // （进组走的是 onGroupTab → groupTabsTogether 路径）
    moveTabToFolder(workspace, tab, { type: "favorites" });

    // 如果 target 在一个 favorite group 内，用该 group 在 tabs 数组中的边界计算插入位置
    const targetTab = workspace.tabs.find((candidate) => candidate.id === targetTabId);
    let anchorId = targetTabId;
    if (targetTab?.groupId) {
      const siblings = workspace.tabs.filter((sibling) => sibling.groupId === targetTab.groupId);
      if (siblings.length > 0) {
        anchorId = placement === "after"
          ? siblings[siblings.length - 1].id
          : siblings[0].id;
      }
    }

    const anchorIndex = workspace.tabs.findIndex((candidate) => candidate.id === anchorId);
    if (anchorIndex < 0) {
      workspace.tabs.push(tab);
    } else {
      workspace.tabs.splice(placement === "after" ? anchorIndex + 1 : anchorIndex, 0, tab);
    }
    reorderFavoriteTab(workspace, tab.id, targetTabId, placement);
  });
}

export function moveTabToWorkspace(state: BrowserState, tabId: string, workspaceId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const source = draft.workspaces.find((workspace) => workspace.tabs.some((tab) => tab.id === tabId));
    const target = draft.workspaces.find((workspace) => workspace.id === workspaceId);
    if (!source || !target || source.id === target.id) return;

    const index = source.tabs.findIndex((tab) => tab.id === tabId);
    const [tab] = source.tabs.splice(index, 1);
    const wasFavorite = tab.isFavorite;
    const wasGrouped = Boolean(tab.groupId);
    const originalGroupId = tab.groupId;
    tab.groupId = null;
    if (wasGrouped) pruneEmptyTabGroups(source);
    source.favoriteOrder = source.favoriteOrder.filter((id) => id !== tabId);
    if (source.tabs.length === 0) {
      const replacement = createTab("New Tab", getWorkspaceHomepageUrl(draft, source));
      source.tabs.push(replacement);
      source.activeTabId = replacement.id;
    } else if (source.activeTabId === tabId || !source.tabs.some((t) => t.id === source.activeTabId)) {
      source.activeTabId = source.tabs[Math.max(0, index - 1)].id;
    }

    target.tabs.push(tab);
    // Restore group membership if the destination already has the same group.
    if (wasGrouped && originalGroupId && target.tabGroups.some((g) => g.id === originalGroupId)) {
      tab.groupId = originalGroupId;
    }
    // Re-apply favorite status now that the tab lives inside the target workspace.
    if (wasFavorite) {
      placeTabInFavoritesFolder(target, tab);
    } else {
      removeTabFromFavoritesFolder(target, tab);
    }
    target.activeTabId = tab.id;
    draft.activeWorkspaceId = target.id;
    clearSplitView(draft);
  });
}

export function moveTabGroupToWorkspace(state: BrowserState, groupId: string, workspaceId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const source = draft.workspaces.find((workspace) => workspace.tabGroups.some((group) => group.id === groupId));
    const target = draft.workspaces.find((workspace) => workspace.id === workspaceId);
    if (!source || !target || source.id === target.id) return;

    const group = source.tabGroups.find((candidate) => candidate.id === groupId);
    const movingTabs = source.tabs.filter((tab) => tab.groupId === groupId);
    if (!group || movingTabs.length === 0) return;
    const allFavorite = movingTabs.every((tab) => tab.isFavorite);

    const firstMovedIndex = source.tabs.findIndex((tab) => tab.groupId === groupId);
    const movingIds = new Set(movingTabs.map((tab) => tab.id));
    const activeMovedTab = movingTabs.find((tab) => tab.id === source.activeTabId);

    source.tabs = source.tabs.filter((tab) => !movingIds.has(tab.id));
    source.favoriteOrder = source.favoriteOrder.filter((id) => !movingIds.has(id));
    pruneEmptyTabGroups(source);

    if (source.tabs.length === 0) {
      const replacement = createTab("New Tab", getWorkspaceHomepageUrl(draft, source));
      source.tabs.push(replacement);
      source.activeTabId = replacement.id;
    } else if (!source.tabs.some((tab) => tab.id === source.activeTabId)) {
      source.activeTabId = source.tabs[Math.min(firstMovedIndex, source.tabs.length - 1)].id;
    }

    if (!target.tabGroups.some((candidate) => candidate.id === group.id)) {
      target.tabGroups.push({ ...group });
    }
    target.tabs.push(...movingTabs);
    for (const movedTab of movingTabs) {
      movedTab.groupId = group.id;
      if (allFavorite) {
        placeTabInFavoritesFolder(target, movedTab);
      } else {
        removeTabFromFavoritesFolder(target, movedTab);
      }
    }
    target.activeTabId = activeMovedTab?.id ?? movingTabs[0].id;
    draft.activeWorkspaceId = target.id;
    clearSplitView(draft);
  });
}

export function getTabFolder(workspace: Workspace, tab: BrowserTab): TabFolder {
  if (tab.isPinned) return { type: "pinned" };
  if (tab.groupId) return { type: "group", groupId: tab.groupId };
  if (isTabInFavoritesFolder(workspace, tab)) return { type: "favorites" };
  return { type: "tabs" };
}

export function moveTabToFolder(workspace: Workspace, tab: BrowserTab, folder: TabFolder): boolean {
  if (folder.type === "group" && !workspace.tabGroups.some((group) => group.id === folder.groupId)) {
    return false;
  }

  tab.isPinned = folder.type === "pinned";

  if (folder.type === "group") {
    const targetGroup = workspace.tabGroups.find((g) => g.id === folder.groupId);
    if (!targetGroup) return false;
    tab.groupId = targetGroup.id;
    // Inherit favorite status from the target group. An empty group inherits
    // from the tab being moved in so newly-created favorite-groups keep their
    // members favorited; non-empty groups derive their status from siblings
    // and normalization will later coerce any mixed membership.
    const siblings = workspace.tabs.filter((t) => t.groupId === targetGroup.id && t.id !== tab.id);
    const groupIsFavorite = siblings.length === 0
      ? tab.isFavorite
      : siblings.every((s) => s.isFavorite);
    if (groupIsFavorite) {
      placeTabInFavoritesFolder(workspace, tab);
    } else {
      removeTabFromFavoritesFolder(workspace, tab);
    }
    return true;
  }

  tab.groupId = null;
  if (folder.type === "favorites") {
    placeTabInFavoritesFolder(workspace, tab);
  } else {
    removeTabFromFavoritesFolder(workspace, tab);
  }
  return true;
}
