export { updateBrowserState } from "./browser/updateState";
export {
  clearHistory,
  clearBrowsingData,
  clearWorkspaceBrowsingData,
  navigateActiveTab,
  openUrlInActiveWorkspace,
  recordHistory,
  removeHistoryEntry,
  upsertDownload
} from "./browsing/actions";
export {
  clearSitePermissionRule,
  setSitePermission,
  updateSettings
} from "./permissions/settingsActions";
export {
  addTab,
  assignTabToGroup,
  closeActiveTab,
  closeOtherTabs,
  closeTabsToLeft,
  closeTabsToRight,
  closeTab,
  duplicateActiveTab,
  duplicateTab,
  fillSplitView,
  focusSplitPane,
  groupActiveTab,
  groupTab,
  moveTabToWorkspace,
  openTabInSplit,
  openUrlInSplit,
  removeTabFromSplit,
  reorderTab,
  resetActiveTabZoom,
  restoreClosedTab,
  restoreLastClosedTab,
  selectAdjacentTab,
  selectTab,
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
  toggleTabGroupCollapsed,
  toggleTabMuted,
  toggleTabPinned,
  toggleSplitMode,
  ungroupActiveTab,
  ungroupTab,
  updateTabGroup,
  updateTab,
  type TabDropPlacement
} from "./tab-actions";
export {
  addWorkspace,
  deleteWorkspace,
  reorderWorkspace,
  switchWorkspace,
  updateWorkspaceById,
  updateWorkspace,
  type WorkspaceDropPlacement
} from "./workspaces/actions";
