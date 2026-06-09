import { BrowserState, BrowserTab } from "../browser";
import { getActiveWorkspace } from "../browser/selectors";
import { updateBrowserState } from "../browser/updateState";
import { clearSplitView, getSplitTabIds, setSplitTabIds } from "./splitView";
import {
  getGroupSleepableTabs,
  getMemoryReleasableTabs,
  markTabAwake,
  markTabSleeping
} from "./sleepPolicy";

export function sleepTab(state: BrowserState, tabId: string): BrowserState {
  const workspace = state.workspaces.find((candidate) => candidate.tabs.some((tab) => tab.id === tabId));
  const tab = workspace?.tabs.find((candidate) => candidate.id === tabId);
  if (!workspace || !tab || workspace.tabs.length <= 1) return state;

  // Pinned tabs are always protected.
  if (tab.isPinned) return state;

  // Split-view tabs other than the active tab are protected; the active tab
  // can still be put to sleep explicitly when another tab can receive focus.
  const splitTabIds = getSplitTabIds(state);
  if (splitTabIds.includes(tab.id) && workspace.activeTabId !== tab.id) return state;

  if (tab.isSleeping) return state;

  return updateBrowserState(state, (draft) => {
    const workspace = draft.workspaces.find((candidate) => candidate.tabs.some((tab) => tab.id === tabId));
    const tab = workspace?.tabs.find((candidate) => candidate.id === tabId);
    if (!workspace || !tab) return;

    if (workspace.activeTabId === tab.id) {
      const fallback = getSleepTabFocusFallback(workspace.tabs, tab.id);
      if (!fallback) return;

      markTabAwake(fallback);
      workspace.activeTabId = fallback.id;
    }

    setSplitTabIds(draft, getSplitTabIds(draft).filter((id) => id !== tab.id));
    markTabSleeping(tab);
  });
}

function getSleepTabFocusFallback(tabs: BrowserTab[], tabId: string): BrowserTab | null {
  const index = tabs.findIndex((candidate) => candidate.id === tabId);
  if (index < 0) return null;
  return tabs[index - 1] ?? tabs[index + 1] ?? null;
}

export function sleepTabGroup(state: BrowserState, groupId: string): BrowserState {
  const workspace = state.workspaces.find((candidate) => candidate.tabGroups.some((group) => group.id === groupId));
  const sleepableTabIds = workspace
    ? getGroupSleepableTabs(workspace, state, groupId).map((tab) => tab.id)
    : [];

  if (sleepableTabIds.length === 0) return state;

  return updateBrowserState(state, (draft) => {
    const workspace = draft.workspaces.find((candidate) => candidate.tabGroups.some((group) => group.id === groupId));
    if (!workspace) return;

    const sleepableTabs = new Set(sleepableTabIds);
    for (const tab of workspace.tabs) {
      if (sleepableTabs.has(tab.id)) {
        markTabSleeping(tab);
      }
    }
  });
}

export function sleepInactiveTabs(state: BrowserState): BrowserState {
  const workspace = getActiveWorkspace(state);
  const sleepableTabIds = getMemoryReleasableTabs(workspace, state).map((tab) => tab.id);

  if (sleepableTabIds.length === 0) return state;

  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const sleepableTabs = new Set(sleepableTabIds);

    for (const tab of workspace.tabs) {
      if (sleepableTabs.has(tab.id)) {
        markTabSleeping(tab);
      }
    }
  });
}

export function sleepIdleTabs(state: BrowserState, now = Date.now()): BrowserState {
  if (!state.settings.memorySaverEnabled) return state;

  const workspace = getActiveWorkspace(state);
  const cutoff = now - state.settings.memorySaverIdleMinutes * 60_000;
  const sleepableTabIds = getMemoryReleasableTabs(workspace, state)
    .filter((tab) => tab.lastActiveAt <= cutoff)
    .map((tab) => tab.id);

  if (sleepableTabIds.length === 0) return state;

  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const sleepableTabs = new Set(sleepableTabIds);

    for (const tab of workspace.tabs) {
      if (sleepableTabs.has(tab.id)) {
        markTabSleeping(tab);
      }
    }
  });
}

// Keep imports used above referenced so tree-shakers don't drop them in case
// downstream consumers don't re-import these helpers.
void clearSplitView;
