import type { SplitLayout } from "../stores/browserStore";

export interface Command {
  title: string;
  subtitle: string;
  run: () => void;
  runInSplit?: () => void;
  runPreview?: () => void;
}

export interface CommandActions {
  addWorkspace: () => void;
  clearBrowsingData: () => void;
  clearHistory: () => void;
  clearWorkspaceBrowsingData: (workspaceId: string) => void;
  assignTabToGroup: (tabId: string, groupId: string) => void;
  closeActiveTab: () => void;
  closeOtherTabs: () => void;
  closeTabsToLeft: () => void;
  closeTabsToRight: () => void;
  deleteWorkspace: (workspaceId: string) => void;
  duplicateActiveTab: () => void;
  fillSplitView: () => void;
  focusAddressBar: () => void;
  groupActiveTab: () => void;
  moveTabToWorkspace: (tabId: string, workspaceId: string) => void;
  openGlance: (url: string, title?: string) => void;
  openTabInSplit: (tabId: string) => void;
  openUrlInSplit: (url: string, title?: string) => void;
  newTab: () => void;
  openUrlInActiveWorkspace: (url: string, title?: string) => void;
  restoreClosedTab: (closedIndex: number) => void;
  restoreLastClosedTab: () => void;
  selectAdjacentTab: (direction: 1 | -1) => void;
  selectTab: (tabId: string) => void;
  setSplitLayout: (layout: SplitLayout) => void;
  resetActiveTabZoom: () => void;
  sleepInactiveTabs: () => void;
  switchWorkspace: (workspaceId: string) => void;
  toggleActiveTabFavorite: () => void;
  toggleActiveTabEssential: () => void;
  toggleActiveTabMuted: () => void;
  toggleActiveTabPinned: () => void;
  toggleCompactMode: () => void;
  toggleFloatingSidebar: () => void;
  toggleFloatingToolbar: () => void;
  toggleTabGroupCollapsed: (groupId: string) => void;
  toggleSidebar: () => void;
  toggleSplitMode: () => void;
  ungroupActiveTab: () => void;
  zoomIn: () => void;
  zoomOut: () => void;
}
