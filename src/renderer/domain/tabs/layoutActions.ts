import {
  BrowserState,
  BrowserTab,
  createTab,
  getReadableUrlTitle,
  getWorkspaceHomepageUrl,
  normalizeAddress,
  Workspace
} from "../browser";
import {
  isTabInFavoritesFolder,
  moveTabFavoriteToWorkspace,
  placeTabInFavoritesFolder,
  removeTabFromFavoritesFolder,
  reorderFavoriteBackingTab
} from "../common/favoriteTabs";
import { getActiveTab, getActiveWorkspace } from "../browser/selectors";
import { updateBrowserState } from "../browser/updateState";
import { clearSplitView, getSplitTabIds, MAX_SPLIT_VIEW_TABS, setSplitTabIds } from "./splitView";
import { pruneEmptyTabGroups } from "./groups";
import { markTabAwake } from "./sleepPolicy";
import type { TabDropPlacement } from "./utils";

export type TabFolder =
  | { type: "favorites" }
  | { type: "group"; groupId: string }
  | { type: "pinned" }
  | { type: "tabs" };

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

export function moveTabToFolderPosition(
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

    const targetTab = workspace.tabs[targetIndex];
    const targetFolder = getTabFolder(workspace, targetTab);

    const [tab] = workspace.tabs.splice(fromIndex, 1);
    if (!moveTabToFolder(workspace, tab, targetFolder)) {
      workspace.tabs.splice(fromIndex, 0, tab);
      return;
    }
    pruneEmptyTabGroups(workspace);
    const droppedOnIndex = workspace.tabs.findIndex((candidate) => candidate.id === targetTabId);
    const insertIndex = placement === "after" ? droppedOnIndex + 1 : droppedOnIndex;
    workspace.tabs.splice(insertIndex, 0, tab);
    if (targetFolder.type === "favorites") {
      reorderFavoriteBackingTab(workspace, tab.id, targetTabId, placement);
    }
  });
}

export function moveTabToFolderEnd(
  state: BrowserState,
  tabId: string,
  folder: TabFolder
): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const fromIndex = workspace.tabs.findIndex((tab) => tab.id === tabId);
    if (fromIndex < 0) return;

    const [tab] = workspace.tabs.splice(fromIndex, 1);
    if (!moveTabToFolder(workspace, tab, folder)) {
      workspace.tabs.splice(fromIndex, 0, tab);
      return;
    }
    pruneEmptyTabGroups(workspace);
    workspace.tabs.push(tab);
  });
}

function getTabFolder(workspace: Workspace, tab: BrowserTab): TabFolder {
  if (isTabInFavoritesFolder(workspace, tab)) return { type: "favorites" };
  if (tab.isPinned) return { type: "pinned" };
  if (tab.groupId) return { type: "group", groupId: tab.groupId };

  return { type: "tabs" };
}

function moveTabToFolder(workspace: Workspace, tab: BrowserTab, folder: TabFolder): boolean {
  if (folder.type === "group" && !workspace.tabGroups.some((group) => group.id === folder.groupId)) {
    return false;
  }

  tab.isPinned = folder.type === "pinned";
  tab.groupId = folder.type === "group" ? folder.groupId : null;
  if (folder.type === "favorites") {
    placeTabInFavoritesFolder(workspace, tab);
  } else {
    removeTabFromFavoritesFolder(workspace, tab);
  }
  return true;
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
    moveTabFavoriteToWorkspace(source, target, tab);
    target.activeTabId = tab.id;
    draft.activeWorkspaceId = target.id;
    clearSplitView(draft);
  });
}

export function moveTabGroupToWorkspace(state: BrowserState, groupId: string, workspaceId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const source = draft.workspaces.find((workspace) => workspace.tabGroups.some((group) => group.id === groupId));
    const target = draft.workspaces.find((workspace) => workspace.id === workspaceId);
    if (!source || !target || source.id === target.id) return;

    const group = source.tabGroups.find((candidate) => candidate.id === groupId);
    const movingTabs = source.tabs.filter((tab) => tab.groupId === groupId);
    if (!group || movingTabs.length === 0) return;

    const firstMovedIndex = source.tabs.findIndex((tab) => tab.groupId === groupId);
    const movingIds = new Set(movingTabs.map((tab) => tab.id));
    const activeMovedTab = movingTabs.find((tab) => tab.id === source.activeTabId);

    source.tabs = source.tabs.filter((tab) => !movingIds.has(tab.id));
    pruneEmptyTabGroups(source);

    if (source.tabs.length === 0) {
      const replacement = createTab("New Tab", getWorkspaceHomepageUrl(draft, source));
      source.tabs.push(replacement);
      source.activeTabId = replacement.id;
    } else if (!source.tabs.some((tab) => tab.id === source.activeTabId)) {
      source.activeTabId = source.tabs[Math.min(firstMovedIndex, source.tabs.length - 1)].id;
    }

    if (!target.tabGroups.some((candidate) => candidate.id === group.id)) {
      target.tabGroups.push({ ...group });
    }
    target.tabs.push(...movingTabs);
    target.activeTabId = activeMovedTab?.id ?? movingTabs[0].id;
    draft.activeWorkspaceId = target.id;
    clearSplitView(draft);
  });
}

export function openTabInSplit(state: BrowserState, tabId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const tab = workspace.tabs.find((candidate) => candidate.id === tabId);
    if (!tab || tab.id === workspace.activeTabId) return;

    markTabAwake(tab);
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

    markTabAwake(focusedTab);
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
    markTabAwake(splitTab);
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
      markTabAwake(tab);
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
