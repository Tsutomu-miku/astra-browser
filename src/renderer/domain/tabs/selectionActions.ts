import { BrowserState } from "../browser";
import { getActiveWorkspace } from "../browser/selectors";
import { closeSplitPane, getActiveAncillaryTabId, selectAncillaryTab } from "./splitView";
import { markTabAwake } from "./sleepPolicy";

export function selectTab(state: BrowserState, tabId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const tab = workspace.tabs.find((candidate) => candidate.id === tabId);
    if (!tab) return;

    markTabAwake(tab);
    tab.hasUnread = false;
    workspace.activeTabId = tab.id;
  });
}

export function selectAdjacentTab(state: BrowserState, direction: 1 | -1): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const index = workspace.tabs.findIndex((tab) => tab.id === workspace.activeTabId);
    if (index < 0 || workspace.tabs.length < 2) return;

    const nextIndex = (index + direction + workspace.tabs.length) % workspace.tabs.length;
    workspace.activeTabId = workspace.tabs[nextIndex].id;
    markTabAwake(workspace.tabs[nextIndex]);
    workspace.tabs[nextIndex].hasUnread = false;
    // Adjacent tab navigation keeps the split pane open (Arc-style)
  });
}

export function selectTabInSplitPane(state: BrowserState, tabId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const tab = workspace.tabs.find((candidate) => candidate.id === tabId);
    if (!tab) return;

    markTabAwake(tab);
    tab.hasUnread = false;
    selectAncillaryTab(draft, tabId);
  });
}

export function closeSplitPaneFromSelection(state: BrowserState): BrowserState {
  return updateBrowserState(state, (draft) => {
    closeSplitPane(draft);
  });
}

// Re-export updateState for convenience
import { updateBrowserState } from "../browser/updateState";
