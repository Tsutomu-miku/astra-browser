import { BrowserState, getReadableUrlTitle } from "./browser-core";
import type { TabGroup } from "./browser-types";
import { getActiveTab, getActiveWorkspace } from "./selectors";
import { updateBrowserState } from "./action-core";
import {
  createTabGroup,
  normalizeTabGroupColor,
  normalizeTabGroupName,
  pruneEmptyTabGroups
} from "./tab-groups";

export function groupActiveTab(state: BrowserState): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const tab = getActiveTab(workspace);
    if (tab.groupId) return;

    const group = createTabGroup(getReadableUrlTitle(tab.url), workspace.tabGroups.length);
    workspace.tabGroups.push(group);
    tab.groupId = group.id;
  });
}

export function ungroupActiveTab(state: BrowserState): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const tab = getActiveTab(workspace);
    tab.groupId = null;
    pruneEmptyTabGroups(workspace);
  });
}

export function assignTabToGroup(state: BrowserState, tabId: string, groupId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const group = workspace.tabGroups.find((candidate) => candidate.id === groupId);
    const tab = workspace.tabs.find((candidate) => candidate.id === tabId);
    if (!group || !tab || tab.isPinned) return;

    tab.groupId = group.id;
    pruneEmptyTabGroups(workspace);
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
