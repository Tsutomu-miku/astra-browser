import { BrowserState } from "../browser-core";
import { getActiveTab, getActiveWorkspace } from "../browser/selectors";
import { updateBrowserState } from "../browser/updateState";
import { pruneEmptyTabGroups } from "./groups";
import { clearSplitView, pruneSplitTabIds } from "./splitView";
import { prependClosedTabs } from "./utils";

export function closeOtherTabs(state: BrowserState, targetTabId?: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const target = getTargetTab(workspace, targetTabId);
    if (!target) return;

    const closed = workspace.tabs.filter((tab) => tab.id !== target.id);
    if (closed.length === 0) return;

    prependClosedTabs(workspace, closed);
    workspace.tabs = [target];
    workspace.activeTabId = target.id;
    pruneEmptyTabGroups(workspace);
    clearSplitView(draft);
  });
}

export function closeTabsToLeft(state: BrowserState, targetTabId?: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const target = getTargetTab(workspace, targetTabId);
    if (!target) return;

    const index = workspace.tabs.findIndex((tab) => tab.id === target.id);
    const closed = workspace.tabs.slice(0, index);
    if (closed.length === 0) return;

    prependClosedTabs(workspace, closed);
    workspace.tabs = workspace.tabs.slice(index);
    workspace.activeTabId = target.id;
    pruneEmptyTabGroups(workspace);
    pruneSplitTabIds(draft, workspace);
  });
}

export function closeTabsToRight(state: BrowserState, targetTabId?: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const target = getTargetTab(workspace, targetTabId);
    if (!target) return;

    const index = workspace.tabs.findIndex((tab) => tab.id === target.id);
    const closed = workspace.tabs.slice(index + 1);
    if (closed.length === 0) return;

    prependClosedTabs(workspace, closed);
    workspace.tabs = workspace.tabs.slice(0, index + 1);
    workspace.activeTabId = target.id;
    pruneEmptyTabGroups(workspace);
    pruneSplitTabIds(draft, workspace);
  });
}

function getTargetTab(workspace: ReturnType<typeof getActiveWorkspace>, targetTabId: string | undefined) {
  return targetTabId
    ? workspace.tabs.find((tab) => tab.id === targetTabId)
    : getActiveTab(workspace);
}
