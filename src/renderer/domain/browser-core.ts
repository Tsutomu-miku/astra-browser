export { DEFAULT_URL, INTERNAL_NEW_TAB_URL, SEARCH_ENGINES } from "./browser/constants";
export { createDefaultState, createFavorite, createId, createTab, getNextWorkspaceAccent } from "./browser/factory";
export { formatBytes } from "./browser/formatting";
export { isInternalNewTabUrl, isInternalPageUrl } from "./browser/internalPages";
export {
  getHomepageUrl,
  getHostInitial,
  isEssential,
  getReadableUrlTitle,
  getSearchUrl,
  getWorkspaceHomepageUrl,
  isFavorite,
  normalizeAddress
} from "./browser/navigation";
export {
  applyStartupBehavior,
  normalizeClosedTabs,
  normalizeFavorites,
  normalizeState
} from "./browser/stateNormalization";
export { getBrowserPartitions, getProfileIdForPartition, getWorkspacePartition } from "./workspaces/profiles";
export { getSplitTabIds, MAX_SPLIT_VIEW_TABS } from "./tabs/splitView";

export type {
  BrowserState,
  BrowserTab,
  ClosedTab,
  DownloadEntry,
  Favorite,
  HistoryEntry,
  PartialBrowserState,
  SearchEngineKey,
  SitePermissionRule,
  StartupBehavior,
  TabGroup,
  Workspace
} from "./browser/types";
