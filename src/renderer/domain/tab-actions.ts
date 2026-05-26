export {
  closeOtherTabs,
  closeTabsToLeft,
  closeTabsToRight
} from "./tab-cleanup-actions";
export {
  addTab,
  closeActiveTab,
  closeTab,
  duplicateActiveTab,
  duplicateTab,
  restoreClosedTab,
  restoreLastClosedTab
} from "./tab-lifecycle-actions";
export {
  assignTabToGroup,
  groupActiveTab,
  toggleTabGroupCollapsed,
  ungroupActiveTab,
  updateTabGroup
} from "./tab-group-actions";
export {
  fillSplitView,
  moveTabToWorkspace,
  openTabInSplit,
  openUrlInSplit,
  removeTabFromSplit,
  reorderTab,
  toggleSplitMode
} from "./tab-layout-actions";
export {
  selectAdjacentTab,
  selectTab
} from "./tab-selection-actions";
export {
  resetActiveTabZoom,
  setActiveTabZoom,
  sleepInactiveTabs,
  sleepTab,
  stepActiveTabZoom,
  toggleActiveTabFavorite,
  toggleActiveTabEssential,
  toggleActiveTabMuted,
  toggleActiveTabPinned,
  toggleTabMuted,
  toggleTabPinned,
  updateTab
} from "./tab-state-actions";
export type { TabDropPlacement } from "./tab-utils";
