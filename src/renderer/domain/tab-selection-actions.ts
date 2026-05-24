import { BrowserState } from "./browser-core";
import { getActiveWorkspace } from "./selectors";
import { updateBrowserState } from "./action-core";

export function selectTab(state: BrowserState, tabId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const tab = workspace.tabs.find((candidate) => candidate.id === tabId);
    if (!tab) return;

    tab.isSleeping = false;
    draft.splitMode && tabId !== workspace.activeTabId
      ? draft.splitTabId = tab.id
      : workspace.activeTabId = tab.id;
  });
}

export function selectAdjacentTab(state: BrowserState, direction: 1 | -1): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const index = workspace.tabs.findIndex((tab) => tab.id === workspace.activeTabId);
    if (index < 0 || workspace.tabs.length < 2) return;

    const nextIndex = (index + direction + workspace.tabs.length) % workspace.tabs.length;
    workspace.activeTabId = workspace.tabs[nextIndex].id;
    workspace.tabs[nextIndex].isSleeping = false;
    draft.splitMode = false;
    draft.splitTabId = null;
  });
}
