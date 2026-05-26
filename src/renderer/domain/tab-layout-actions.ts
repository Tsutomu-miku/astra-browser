import {
  BrowserState,
  BrowserTab,
  createTab,
  getReadableUrlTitle,
  getWorkspaceHomepageUrl,
  normalizeAddress,
  Workspace
} from "./browser-core";
import { getActiveTab, getActiveWorkspace } from "./selectors";
import { updateBrowserState } from "./action-core";
import { clearSplitView, getSplitTabIds, MAX_SPLIT_VIEW_TABS, setSplitTabIds } from "./split-view";
import { pruneEmptyTabGroups } from "./tab-groups";
import type { TabDropPlacement } from "./tab-utils";

export function reorderTab(
  state: BrowserState,
  tabId: string,
  targetTabId: string,
  placement: TabDropPlacement
): BrowserState {
  if (tabId === targetTabId) return state;

  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const fromIndex = workspace.tabs.findIndex((tab) => tab.id === tabId);
    const targetIndex = workspace.tabs.findIndex((tab) => tab.id === targetTabId);
    if (fromIndex < 0 || targetIndex < 0) return;

    const [tab] = workspace.tabs.splice(fromIndex, 1);
    const droppedOnIndex = workspace.tabs.findIndex((candidate) => candidate.id === targetTabId);
    const insertIndex = placement === "after" ? droppedOnIndex + 1 : droppedOnIndex;
    workspace.tabs.splice(insertIndex, 0, tab);
  });
}

export function moveTabToWorkspace(state: BrowserState, tabId: string, workspaceId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const source = draft.workspaces.find((workspace) => workspace.tabs.some((tab) => tab.id === tabId));
    const target = draft.workspaces.find((workspace) => workspace.id === workspaceId);
    if (!source || !target || source.id === target.id) return;

    const index = source.tabs.findIndex((tab) => tab.id === tabId);
    const [tab] = source.tabs.splice(index, 1);
    tab.groupId = null;
    pruneEmptyTabGroups(source);
    if (source.tabs.length === 0) {
      const replacement = createTab("New Tab", getWorkspaceHomepageUrl(draft, source));
      source.tabs.push(replacement);
      source.activeTabId = replacement.id;
    } else if (source.activeTabId === tabId) {
      source.activeTabId = source.tabs[Math.max(0, index - 1)].id;
    }

    target.tabs.push(tab);
    target.activeTabId = tab.id;
    draft.activeWorkspaceId = target.id;
    clearSplitView(draft);
  });
}

export function openTabInSplit(state: BrowserState, tabId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const tab = workspace.tabs.find((candidate) => candidate.id === tabId);
    if (!tab || tab.id === workspace.activeTabId) return;

    tab.isSleeping = false;
    const splitTabIds = getSplitTabIds(draft).filter((candidateId) => candidateId !== tab.id);
    setSplitTabIds(draft, [...splitTabIds, tab.id]);
  });
}

export function openUrlInSplit(state: BrowserState, url: string, title?: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const normalizedUrl = normalizeAddress(url, draft.settings.searchEngine);
    const tab = createTab(title || getReadableUrlTitle(normalizedUrl), normalizedUrl);
    workspace.tabs.push(tab);
    setSplitTabIds(draft, [...getSplitTabIds(draft), tab.id]);
  });
}

export function removeTabFromSplit(state: BrowserState, tabId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    if (workspace.activeTabId === tabId) {
      clearSplitView(draft);
      return;
    }

    setSplitTabIds(draft, getSplitTabIds(draft).filter((candidateId) => candidateId !== tabId));
  });
}

export function focusSplitPane(state: BrowserState, tabId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const activeTabId = workspace.activeTabId;
    const splitTabIds = getSplitTabIds(draft);
    const focusedTab = workspace.tabs.find((tab) => tab.id === tabId);

    if (!draft.splitMode || !activeTabId || !focusedTab || !splitTabIds.includes(tabId)) return;

    focusedTab.isSleeping = false;
    workspace.activeTabId = focusedTab.id;
    setSplitTabIds(draft, splitTabIds.map((candidateId) => (
      candidateId === focusedTab.id ? activeTabId : candidateId
    )));
  });
}

export function toggleSplitMode(state: BrowserState): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const active = getActiveTab(workspace);
    const inactiveTabs = workspace.tabs.filter((tab) => tab.id !== active.id);
    if (draft.splitMode) {
      clearSplitView(draft);
      return;
    }

    const splitTab = inactiveTabs[0] ?? createSplitTab(workspace);
    splitTab.isSleeping = false;
    setSplitTabIds(draft, [splitTab.id]);
  });
}

export function fillSplitView(state: BrowserState): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const active = getActiveTab(workspace);
    const selectedIds = getSplitTabIds(draft);
    const selected = new Set([active.id, ...selectedIds]);
    const nextIds = [...selectedIds];

    for (const tab of workspace.tabs) {
      if (nextIds.length >= MAX_SPLIT_VIEW_TABS - 1) break;
      if (selected.has(tab.id)) continue;
      tab.isSleeping = false;
      nextIds.push(tab.id);
      selected.add(tab.id);
    }

    while (nextIds.length < MAX_SPLIT_VIEW_TABS - 1) {
      const tab = createSplitTab(workspace);
      nextIds.push(tab.id);
    }

    setSplitTabIds(draft, nextIds);
  });
}

function createSplitTab(workspace: Workspace): BrowserTab {
  const tab = createTab("Reference", "https://www.wikipedia.org");
  workspace.tabs.push(tab);
  return tab;
}
