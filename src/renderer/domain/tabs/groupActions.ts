import { BrowserState, createId, getReadableUrlTitle } from "../browser";
import type { TabGroup } from "../browser/types";
import { getActiveTab, getActiveWorkspace } from "../browser/selectors";
import { updateBrowserState } from "../browser/updateState";
import {
  createTabGroup,
  normalizeTabGroupColor,
  normalizeTabGroupName,
  pruneEmptyTabGroups
} from "./groups";
import { clearSplitView } from "./splitView";
import type { TabDropPlacement } from "./utils";

export function groupActiveTab(state: BrowserState): BrowserState {
  return groupTab(state);
}

export function groupTab(state: BrowserState, tabId?: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const tab = getTargetTab(workspace, tabId);
    if (!tab || tab.isPinned || tab.groupId) return;

    const group = createTabGroup(getReadableUrlTitle(tab.url), workspace.tabGroups.length);
    workspace.tabGroups.push(group);
    tab.groupId = group.id;
  });
}

export function ungroupActiveTab(state: BrowserState): BrowserState {
  return ungroupTab(state);
}

export function ungroupTab(state: BrowserState, tabId?: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const tab = getTargetTab(workspace, tabId);
    if (!tab) return;

    tab.groupId = null;
    pruneEmptyTabGroups(workspace);
  });
}

export function ungroupTabGroup(state: BrowserState, groupId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const group = workspace.tabGroups.find((candidate) => candidate.id === groupId);
    if (!group) return;

    workspace.tabs.forEach((tab) => {
      if (tab.groupId === group.id) {
        tab.groupId = null;
      }
    });
    pruneEmptyTabGroups(workspace);
  });
}

export function assignTabToGroup(state: BrowserState, tabId: string, groupId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const group = workspace.tabGroups.find((candidate) => candidate.id === groupId);
    const tab = workspace.tabs.find((candidate) => candidate.id === tabId);
    if (!group || !tab) return;

    tab.isPinned = false;
    tab.groupId = group.id;
    pruneEmptyTabGroups(workspace);
  });
}

export function reorderTabGroup(
  state: BrowserState,
  groupId: string,
  targetGroupId: string,
  placement: TabDropPlacement
): BrowserState {
  if (groupId === targetGroupId) return state;

  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const fromIndex = workspace.tabGroups.findIndex((group) => group.id === groupId);
    const targetIndex = workspace.tabGroups.findIndex((group) => group.id === targetGroupId);
    if (fromIndex < 0 || targetIndex < 0) return;

    const [group] = workspace.tabGroups.splice(fromIndex, 1);
    const droppedOnIndex = workspace.tabGroups.findIndex((candidate) => candidate.id === targetGroupId);
    const insertIndex = placement === "after" ? droppedOnIndex + 1 : droppedOnIndex;
    workspace.tabGroups.splice(insertIndex, 0, group);
  });
}

export function duplicateTabGroup(state: BrowserState, groupId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const groupIndex = workspace.tabGroups.findIndex((candidate) => candidate.id === groupId);
    const group = workspace.tabGroups[groupIndex];
    const tabs = workspace.tabs.filter((tab) => tab.groupId === groupId);
    if (!group || tabs.length === 0) return;

    const nextGroup = {
      ...group,
      id: createId(),
      name: `${group.name} Copy`,
      isCollapsed: false
    };
    const lastTabIndex = Math.max(...tabs.map((tab) => workspace.tabs.findIndex((candidate) => candidate.id === tab.id)));
    const now = Date.now();
    const copiedTabs = tabs.map((tab) => ({
      ...tab,
      id: createId(),
      groupId: nextGroup.id,
      canGoBack: false,
      canGoForward: false,
      isLoading: false,
      isSleeping: false,
      lastActiveAt: now
    }));

    workspace.tabGroups.splice(groupIndex + 1, 0, nextGroup);
    workspace.tabs.splice(lastTabIndex + 1, 0, ...copiedTabs);
    workspace.activeTabId = copiedTabs[0].id;
    clearSplitView(draft);
  });
}

export function updateTabGroup(
  state: BrowserState,
  groupId: string,
  patch: Partial<Pick<TabGroup, "name" | "color">>
): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const index = workspace.tabGroups.findIndex((candidate) => candidate.id === groupId);
    if (index < 0) return;

    if (patch.name !== undefined) {
      workspace.tabGroups[index].name = normalizeTabGroupName(patch.name);
    }
    if (patch.color !== undefined) {
      workspace.tabGroups[index].color = normalizeTabGroupColor(patch.color, index);
    }
  });
}

export function toggleTabGroupCollapsed(state: BrowserState, groupId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const group = getActiveWorkspace(draft).tabGroups.find((candidate) => candidate.id === groupId);
    if (group) group.isCollapsed = !group.isCollapsed;
  });
}

function getTargetTab(workspace: ReturnType<typeof getActiveWorkspace>, tabId: string | undefined) {
  return tabId
    ? workspace.tabs.find((candidate) => candidate.id === tabId)
    : getActiveTab(workspace);
}
