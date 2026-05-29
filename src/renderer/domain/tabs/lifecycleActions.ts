import {
  BrowserState,
  createId,
  createTab,
  getWorkspaceHomepageUrl
} from "../browser";
import { getActiveTab, getActiveWorkspace } from "../browser/selectors";
import { updateBrowserState } from "../browser/updateState";
import { pruneEmptyTabGroups } from "./groups";
import { clearSplitView, pruneSplitTabIds } from "./splitView";
import { prependClosedTabs } from "./utils";

export function addTab(state: BrowserState): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const tab = createTab("New Tab", getWorkspaceHomepageUrl(draft, workspace));
    workspace.tabs.push(tab);
    workspace.activeTabId = tab.id;
    clearSplitView(draft);
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
    removeClosedTabFavorites(workspace, tabId);

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

    pruneSplitTabIds(draft, workspace);
  });
}

function removeClosedTabFavorites(workspace: ReturnType<typeof getActiveWorkspace>, tabId: string) {
  workspace.favorites = workspace.favorites.filter((favorite) => favorite.tabId !== tabId);
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

    const tab = {
      ...createTab(closed.title, closed.url),
      ...(closed.faviconUrl ? { faviconUrl: closed.faviconUrl } : {})
    };
    workspace.tabs.push(tab);
    workspace.activeTabId = tab.id;
    clearSplitView(draft);
  });
}

export function restoreClosedTabToWorkspace(
  state: BrowserState,
  closedIndex: number,
  workspaceId: string
): BrowserState {
  const source = getActiveWorkspace(state);
  const target = state.workspaces.find((workspace) => workspace.id === workspaceId);
  if (!Number.isInteger(closedIndex) || closedIndex < 0 || !source.closedTabs[closedIndex] || !target) {
    return state;
  }

  return updateBrowserState(state, (draft) => {
    const source = getActiveWorkspace(draft);
    const target = draft.workspaces.find((workspace) => workspace.id === workspaceId);
    if (!target) return;

    const [closed] = source.closedTabs.splice(closedIndex, 1);
    if (!closed) return;

    const tab = {
      ...createTab(closed.title, closed.url),
      ...(closed.faviconUrl ? { faviconUrl: closed.faviconUrl } : {})
    };
    target.tabs.push(tab);
    target.activeTabId = tab.id;
    draft.activeWorkspaceId = target.id;
    clearSplitView(draft);
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
      isLoading: false,
      lastActiveAt: Date.now()
    };
    workspace.tabs.splice(index + 1, 0, tab);
    draft.activeWorkspaceId = workspace.id;
    workspace.activeTabId = tab.id;
    clearSplitView(draft);
  });
}
