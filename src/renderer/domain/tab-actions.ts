export {
  closeOtherTabs,
  closeTabsToLeft,
  closeTabsToRight
} from "./tabs/cleanupActions";
export {
  addTab,
  closeActiveTab,
  closeTab,
  duplicateActiveTab,
  duplicateTab,
  restoreClosedTab,
  restoreLastClosedTab
} from "./tabs/lifecycleActions";
export {
  assignTabToGroup,
  groupActiveTab,
  groupTab,
  toggleTabGroupCollapsed,
  ungroupActiveTab,
  ungroupTab,
  updateTabGroup
} from "./tabs/groupActions";
export {
  fillSplitView,
  focusSplitPane,
  moveTabToWorkspace,
  openTabInSplit,
  openUrlInSplit,
  removeTabFromSplit,
  reorderTab,
  toggleSplitMode
} from "./tabs/layoutActions";
export {
  selectAdjacentTab,
  selectTab
} from "./tabs/selectionActions";
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
  toggleTabEssential,
  toggleTabFavorite,
  toggleTabMuted,
  toggleTabPinned,
  updateTab
} from "./tabs/stateActions";
export type { TabDropPlacement } from "./tabs/utils";
