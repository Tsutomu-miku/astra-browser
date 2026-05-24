import {
  BrowserState,
  BrowserTab,
  createTab,
  getHomepageUrl,
  Workspace
} from "./browser-core";
import { getActiveTab, getActiveWorkspace } from "./selectors";
import { updateBrowserState } from "./action-core";
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
      const replacement = createTab("New Tab", getHomepageUrl(draft));
      source.tabs.push(replacement);
      source.activeTabId = replacement.id;
    } else if (source.activeTabId === tabId) {
      source.activeTabId = source.tabs[Math.max(0, index - 1)].id;
    }

    target.tabs.push(tab);
    target.activeTabId = tab.id;
    draft.activeWorkspaceId = target.id;
    draft.splitMode = false;
    draft.splitTabId = null;
  });
}

export function openTabInSplit(state: BrowserState, tabId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const tab = workspace.tabs.find((candidate) => candidate.id === tabId);
    if (!tab || tab.id === workspace.activeTabId) return;

    draft.splitMode = true;
    draft.splitTabId = tab.id;
  });
}

export function toggleSplitMode(state: BrowserState): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const active = getActiveTab(workspace);
    const inactiveTabs = workspace.tabs.filter((tab) => tab.id !== active.id);
    draft.splitMode = !draft.splitMode;
    draft.splitTabId = draft.splitMode ? inactiveTabs[0]?.id ?? createSplitTab(workspace).id : null;
  });
}

function createSplitTab(workspace: Workspace): BrowserTab {
  const tab = createTab("Reference", "https://www.wikipedia.org");
  workspace.tabs.push(tab);
  return tab;
}
