import { BrowserState } from "./browser-core";
import { getActiveWorkspace } from "./selectors";
import { updateBrowserState } from "./action-core";

export function selectTab(state: BrowserState, tabId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    draft.splitMode && tabId !== workspace.activeTabId
      ? draft.splitTabId = tabId
      : workspace.activeTabId = tabId;
  });
}

export function selectAdjacentTab(state: BrowserState, direction: 1 | -1): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const index = workspace.tabs.findIndex((tab) => tab.id === workspace.activeTabId);
    if (index < 0 || workspace.tabs.length < 2) return;

    const nextIndex = (index + direction + workspace.tabs.length) % workspace.tabs.length;
    workspace.activeTabId = workspace.tabs[nextIndex].id;
    draft.splitMode = false;
    draft.splitTabId = null;
  });
}
