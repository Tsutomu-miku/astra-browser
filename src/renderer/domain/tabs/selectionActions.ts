import { BrowserState } from "../browser";
import { getActiveWorkspace } from "../browser/selectors";
import { updateBrowserState } from "../browser/updateState";
import { clearSplitView, getSplitTabIds, setSplitTabIds } from "./splitView";
import { markTabAwake } from "./sleepPolicy";

export function selectTab(state: BrowserState, tabId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const tab = workspace.tabs.find((candidate) => candidate.id === tabId);
    if (!tab) return;

    markTabAwake(tab);
    if (draft.splitMode && tabId !== workspace.activeTabId) {
      const currentIds = getSplitTabIds(draft).filter((candidateId) => candidateId !== tab.id);
      const nextIds = currentIds.length >= 3
        ? [...currentIds.slice(0, 2), tab.id]
        : [...currentIds, tab.id];
      setSplitTabIds(draft, nextIds);
      return;
    }

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
    clearSplitView(draft);
  });
}
