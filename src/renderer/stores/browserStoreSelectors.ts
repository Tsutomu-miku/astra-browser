import type { BrowserState } from "../domain/browser-core";

export function getActiveUrl(state: BrowserState): string {
  const workspace = state.workspaces.find((candidate) => candidate.id === state.activeWorkspaceId) ?? state.workspaces[0];
  return workspace.tabs.find((tab) => tab.id === workspace.activeTabId)?.url ?? workspace.tabs[0].url;
}

export function getActiveProfileId(state: BrowserState): string {
  const workspace = state.workspaces.find((candidate) => candidate.id === state.activeWorkspaceId) ?? state.workspaces[0];
  return workspace?.profileId ?? "default";
}
