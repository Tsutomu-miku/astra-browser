import type { BrowserTab, TabGroup, Workspace } from "../../../domain/browser-core";

export interface MoveWorkspaceTarget {
  id: string;
  name: string;
}

export interface MoveGroupTarget {
  id: string;
  name: string;
}

export interface TabCleanupState {
  canCloseOtherTabs: boolean;
  canCloseTabsToLeft: boolean;
  canCloseTabsToRight: boolean;
}

export interface TabGroupMenuState {
  canCreateGroup: boolean;
  canUngroup: boolean;
  moveGroupTargets: MoveGroupTarget[];
}

export function getMoveWorkspaceTargets(
  workspaces: Pick<Workspace, "id" | "name">[],
  activeWorkspaceId: string
): MoveWorkspaceTarget[] {
  return workspaces
    .filter((workspace) => workspace.id !== activeWorkspaceId)
    .map((workspace) => ({
      id: workspace.id,
      name: workspace.name.trim() || "Space"
    }));
}

export function getTabCleanupState(
  tabs: Pick<BrowserTab, "id">[],
  tabId: string
): TabCleanupState {
  const index = tabs.findIndex((tab) => tab.id === tabId);
  if (index < 0) {
    return {
      canCloseOtherTabs: false,
      canCloseTabsToLeft: false,
      canCloseTabsToRight: false
    };
  }

  return {
    canCloseOtherTabs: tabs.length > 1,
    canCloseTabsToLeft: index > 0,
    canCloseTabsToRight: index < tabs.length - 1
  };
}

export function getTabGroupMenuState(
  groups: Pick<TabGroup, "id" | "name">[],
  tab: Pick<BrowserTab, "groupId" | "isPinned">
): TabGroupMenuState {
  if (tab.isPinned) {
    return {
      canCreateGroup: false,
      canUngroup: false,
      moveGroupTargets: []
    };
  }

  return {
    canCreateGroup: !tab.groupId,
    canUngroup: Boolean(tab.groupId),
    moveGroupTargets: groups
      .filter((group) => group.id !== tab.groupId)
      .map((group) => ({
        id: group.id,
        name: group.name.trim() || "Group"
      }))
  };
}
