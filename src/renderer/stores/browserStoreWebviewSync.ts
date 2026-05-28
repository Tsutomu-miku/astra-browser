import type { BrowserState } from "../domain/browser";
import type { WebviewElement } from "../types/browser-ui";

export function syncZoom(state: BrowserState, webview?: WebviewElement): BrowserState {
  const workspace = state.workspaces.find((candidate) => candidate.id === state.activeWorkspaceId) ?? state.workspaces[0];
  const tab = workspace.tabs.find((candidate) => candidate.id === workspace.activeTabId) ?? workspace.tabs[0];
  webview?.setZoomFactor?.(tab.zoomFactor);
  return state;
}

export function syncMuted(state: BrowserState, webview?: WebviewElement, tabId?: string): BrowserState {
  const workspace = state.workspaces.find((candidate) => candidate.id === state.activeWorkspaceId) ?? state.workspaces[0];
  const tab = tabId
    ? state.workspaces.flatMap((candidate) => candidate.tabs).find((candidate) => candidate.id === tabId)
    : workspace.tabs.find((candidate) => candidate.id === workspace.activeTabId) ?? workspace.tabs[0];
  if (tab) webview?.setAudioMuted?.(tab.isMuted);
  return state;
}
