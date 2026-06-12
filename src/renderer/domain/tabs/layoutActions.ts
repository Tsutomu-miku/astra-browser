import { BrowserState, BrowserTab, createTab, getReadableUrlTitle, getWorkspaceHomepageUrl, normalizeAddress, SplitLayout, Workspace } from "../browser";
import { getActiveTab, getActiveWorkspace } from "../browser/selectors";
import { updateBrowserState } from "../browser/updateState";
import {
  activateSplitTab,
  closeSplitPane,
  getSplitTab,
  openTabInAncillaryPane,
  pruneAncillaryTabIds,
  selectAncillaryTab,
  setSplitSideFromState,
  swapSplitPanes as swapSplitPanesCore,
  swapSplitPanesFromState,
  syncLegacyStateSplitFields,
  toggleSplitSideFromState,
  MAX_SPLIT_VIEW_TABS
} from "./splitView";

export { MAX_SPLIT_VIEW_TABS };
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

/** Open an existing tab in the secondary (ancillary) split pane. */
export function openTabInSplit(state: BrowserState, tabId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const tab = workspace.tabs.find((candidate) => candidate.id === tabId);
    if (!tab || tab.id === workspace.activeTabId) return;

    markTabAwake(tab);
    openTabInAncillaryPane(draft, tab.id);
  });
}

/** Activate a specific split entity (shows both panes in content area). */
export function selectSplitTab(state: BrowserState, splitId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    activateSplitTab(workspace, splitId);
  });
}

/** Create a new tab and open it in the ancillary pane. */
export function openUrlInSplit(state: BrowserState, url: string, title?: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const normalizedUrl = normalizeAddress(url, draft.settings.searchEngine);
    const tab = createTab(title || getReadableUrlTitle(normalizedUrl), normalizedUrl);
    workspace.tabs.push(tab);
    openTabInAncillaryPane(draft, tab.id);
  });
}

/** Remove a tab from the ancillary split pane. */
export function removeTabFromSplit(state: BrowserState, tabId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    if (workspace.activeTabId === tabId) {
      closeSplitPane(draft);
      return;
    }

    removeFromAncillary(draft, tabId);
  });
}

function removeFromAncillary(state: BrowserState, tabId: string): void {
  const workspace = getActiveWorkspace(state);
  workspace.ancillaryTabIds = workspace.ancillaryTabIds.filter((id) => id !== tabId);
  if (workspace.activeAncillaryTabId === tabId) {
    workspace.activeAncillaryTabId = workspace.ancillaryTabIds[0] ?? null;
    if (workspace.ancillaryTabIds.length === 0) {
      workspace.splitMode = false;
    }
  }
}

/** Switch focus between primary and ancillary panes. */
export function focusSplitPane(state: BrowserState, tabId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    if (!workspace.splitMode) return;

    // If it's the active tab, no-op
    if (workspace.activeTabId === tabId) return;

    // If the tab is in the ancillary pane → make it active ancillary, then swap
    if (workspace.ancillaryTabIds.includes(tabId)) {
      selectAncillaryTab(draft, tabId);
      swapSplitPanesFromState(draft);
      return;
    }
  });
}

/** Toggle the split pane on/off. When turning on, open the first inactive tab. */
export function toggleSplitMode(state: BrowserState): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const active = getActiveTab(workspace);

    if (workspace.splitMode) {
      closeSplitPane(draft);
      return;
    }

    // Find a suitable tab to show in the split pane
    const inactiveTabs = workspace.tabs.filter((tab) => tab.id !== active.id);
    if (inactiveTabs.length === 0) {
      // No other tabs — create a new one
      const tab = createTab("Reference", getWorkspaceHomepageUrl(draft, workspace));
      workspace.tabs.push(tab);
      openTabInAncillaryPane(draft, tab.id);
    } else {
      markTabAwake(inactiveTabs[0]);
      openTabInAncillaryPane(draft, inactiveTabs[0].id);
    }
  });
}

/** Change the split layout (horizontal / vertical / grid). */
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

/** Fill the split view with many tabs (grid layout mode). */
export function fillSplitView(state: BrowserState): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    workspace.splitLayout = "grid";

    const MAX_GRID_TABS = 4;
    const active = getActiveTab(workspace);
    const ancillary: string[] = [];

    for (const tab of workspace.tabs) {
      if (ancillary.length >= MAX_GRID_TABS - 1) break;
      if (tab.id === active.id) continue;
      markTabAwake(tab);
      ancillary.push(tab.id);
    }

    while (ancillary.length < MAX_GRID_TABS - 1) {
      const tab = createTab("Reference", "https://www.wikipedia.org");
      workspace.tabs.push(tab);
      ancillary.push(tab.id);
    }

    workspace.ancillaryTabIds = ancillary;
    workspace.activeAncillaryTabId = ancillary[0] ?? null;
    workspace.splitMode = ancillary.length > 0;
    syncLegacyStateSplitFields(draft);
  });
}

/** Swap primary and ancillary split panes. */
export function swapSplitPanes(state: BrowserState, splitId?: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const targetId = splitId ?? workspace.activeSplitId;
    if (!targetId) return;
    const split = getSplitTab(workspace, targetId);
    if (!split) return;
    swapSplitPanesCore(workspace, targetId);
  });
}

/** Swap which side the split pane appears on. */
export function toggleSplitPaneSide(state: BrowserState): BrowserState {
  return updateBrowserState(state, (draft) => {
    toggleSplitSideFromState(draft);
  });
}

/** Set which side the split pane appears on. */
export function setSplitPaneSide(state: BrowserState, side: "left" | "right"): BrowserState {
  return updateBrowserState(state, (draft) => {
    setSplitSideFromState(draft, side);
  });
}

export { pruneAncillaryTabIds };
