import { BrowserState, BrowserTab, createTab, getReadableUrlTitle, getWorkspaceHomepageUrl, normalizeAddress, SplitLayout, Workspace } from "../browser";
import { getActiveTab, getActiveWorkspace } from "../browser/selectors";
import { updateBrowserState } from "../browser/updateState";
import { clearSplitView, getSplitTabIds, MAX_SPLIT_VIEW_TABS, setSplitTabIds } from "./splitView";
import { markTabAwake } from "./sleepPolicy";

// Folder and workspace-move actions live in folderActions.ts and are re-exported
// here to preserve the existing public surface of the tabs module.
export {
  moveTabToFavoritePosition,
  moveTabToFolderEnd,
  moveTabToFolderPosition,
  moveTabGroupToWorkspace,
  moveTabToWorkspace,
  reorderTab,
  type TabFolder
} from "./folderActions";

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
    workspace.splitLayout = "grid";
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

export function setWorkspaceSplitLayout(state: BrowserState, layout: SplitLayout): BrowserState {
  if (!isSplitLayoutValue(layout)) return state;

  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    if (workspace.splitLayout === layout) return;
    workspace.splitLayout = layout;
  });
}

function isSplitLayoutValue(value: unknown): value is SplitLayout {
  return value === "horizontal" || value === "vertical" || value === "grid";
}

function createSplitTab(workspace: Workspace): BrowserTab {
  const tab = createTab("Reference", "https://www.wikipedia.org");
  workspace.tabs.push(tab);
  return tab;
}
