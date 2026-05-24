import {
  BrowserState,
  createId,
  createTab,
  getWorkspaceHomepageUrl
} from "./browser-core";
import { getActiveTab, getActiveWorkspace } from "./selectors";
import { updateBrowserState } from "./action-core";
import { pruneEmptyTabGroups } from "./tab-groups";
import { prependClosedTabs } from "./tab-utils";

export function addTab(state: BrowserState): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const tab = createTab("New Tab", getWorkspaceHomepageUrl(draft, workspace));
    workspace.tabs.push(tab);
    workspace.activeTabId = tab.id;
    draft.splitTabId = null;
  });
}

export function closeActiveTab(state: BrowserState): BrowserState {
  return closeTab(state, getActiveTab(getActiveWorkspace(state)).id);
}

export function closeTab(state: BrowserState, tabId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const tab = workspace.tabs.find((candidate) => candidate.id === tabId);
    if (!tab) return;

    prependClosedTabs(workspace, [tab]);

    if (workspace.tabs.length === 1) {
      workspace.tabs[0] = createTab("New Tab", getWorkspaceHomepageUrl(draft, workspace));
      workspace.activeTabId = workspace.tabs[0].id;
      pruneEmptyTabGroups(workspace);
      return;
    }

    const index = workspace.tabs.findIndex((candidate) => candidate.id === tabId);
    workspace.tabs = workspace.tabs.filter((candidate) => candidate.id !== tabId);
    pruneEmptyTabGroups(workspace);
    if (workspace.activeTabId === tabId) {
      workspace.activeTabId = workspace.tabs[Math.max(0, index - 1)].id;
    }

    if (draft.splitTabId && !workspace.tabs.some((tab) => tab.id === draft.splitTabId)) {
      draft.splitTabId = null;
      draft.splitMode = false;
    }
  });
}

export function restoreLastClosedTab(state: BrowserState): BrowserState {
  return restoreClosedTab(state, 0);
}

export function restoreClosedTab(state: BrowserState, closedIndex: number): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    if (!Number.isInteger(closedIndex) || closedIndex < 0) return;

    const [closed] = workspace.closedTabs.splice(closedIndex, 1);
    if (!closed) return;

    const tab = createTab(closed.title, closed.url);
    workspace.tabs.push(tab);
    workspace.activeTabId = tab.id;
    draft.splitTabId = null;
  });
}

export function duplicateActiveTab(state: BrowserState): BrowserState {
  return duplicateTab(state, getActiveTab(getActiveWorkspace(state)).id);
}

export function duplicateTab(state: BrowserState, tabId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = draft.workspaces.find((candidate) => candidate.tabs.some((tab) => tab.id === tabId));
    if (!workspace) return;

    const source = workspace.tabs.find((tab) => tab.id === tabId);
    if (!source) return;

    const index = workspace.tabs.findIndex((tab) => tab.id === source.id);
    const tab = {
      ...source,
      id: createId(),
      canGoBack: false,
      canGoForward: false,
      isLoading: false
    };
    workspace.tabs.splice(index + 1, 0, tab);
    draft.activeWorkspaceId = workspace.id;
    workspace.activeTabId = tab.id;
    draft.splitTabId = null;
  });
}
