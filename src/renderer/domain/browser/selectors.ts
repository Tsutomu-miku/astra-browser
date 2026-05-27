import type { BrowserState, BrowserTab, Workspace } from "./types";

export function getActiveWorkspace(state: BrowserState): Workspace {
  return state.workspaces.find((workspace) => workspace.id === state.activeWorkspaceId) ?? state.workspaces[0];
}

export function getActiveTab(workspace: Workspace): BrowserTab {
  return workspace.tabs.find((tab) => tab.id === workspace.activeTabId) ?? workspace.tabs[0];
}
