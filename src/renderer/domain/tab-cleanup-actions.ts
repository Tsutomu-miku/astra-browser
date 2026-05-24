import { BrowserState } from "./browser-core";
import { getActiveTab, getActiveWorkspace } from "./selectors";
import { updateBrowserState } from "./action-core";
import { pruneEmptyTabGroups } from "./tab-groups";
import { prependClosedTabs } from "./tab-utils";

export function closeOtherTabs(state: BrowserState): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const active = getActiveTab(workspace);
    const closed = workspace.tabs.filter((tab) => tab.id !== active.id);
    if (closed.length === 0) return;

    prependClosedTabs(workspace, closed);
    workspace.tabs = [active];
    workspace.activeTabId = active.id;
    pruneEmptyTabGroups(workspace);
    draft.splitMode = false;
    draft.splitTabId = null;
  });
}

export function closeTabsToLeft(state: BrowserState): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const active = getActiveTab(workspace);
    const index = workspace.tabs.findIndex((tab) => tab.id === active.id);
    const closed = workspace.tabs.slice(0, index);
    if (closed.length === 0) return;

    prependClosedTabs(workspace, closed);
    workspace.tabs = workspace.tabs.slice(index);
    workspace.activeTabId = active.id;
    pruneEmptyTabGroups(workspace);
    if (draft.splitTabId && !workspace.tabs.some((tab) => tab.id === draft.splitTabId)) {
      draft.splitMode = false;
      draft.splitTabId = null;
    }
  });
}

export function closeTabsToRight(state: BrowserState): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const active = getActiveTab(workspace);
    const index = workspace.tabs.findIndex((tab) => tab.id === active.id);
    const closed = workspace.tabs.slice(index + 1);
    if (closed.length === 0) return;

    prependClosedTabs(workspace, closed);
    workspace.tabs = workspace.tabs.slice(0, index + 1);
    workspace.activeTabId = active.id;
    pruneEmptyTabGroups(workspace);
    if (draft.splitTabId && !workspace.tabs.some((tab) => tab.id === draft.splitTabId)) {
      draft.splitMode = false;
      draft.splitTabId = null;
    }
  });
}
