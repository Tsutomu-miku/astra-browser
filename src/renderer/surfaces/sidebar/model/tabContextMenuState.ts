import type { BrowserTab, Workspace } from "../../../domain/browser-core";

export interface MoveWorkspaceTarget {
  id: string;
  name: string;
}

export interface TabCleanupState {
  canCloseOtherTabs: boolean;
  canCloseTabsToLeft: boolean;
  canCloseTabsToRight: boolean;
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
