export {
  clearAllPerOriginZoom,
  clearPerOriginZoom,
  getZoomForOrigin,
  getZoomForUrl,
  normalizePerOriginZoom,
  upsertPerOriginZoom
} from "./perOriginZoom";
export {
  DEFAULT_ZOOM_FACTOR,
  formatZoomPercent,
  MAX_ZOOM_FACTOR,
  MIN_ZOOM_FACTOR,
  normalizeZoomFactor,
  stepZoomFactor,
  ZOOM_STEP
} from "./zoom";
export { DEFAULT_URL, INTERNAL_NEW_TAB_URL, SEARCH_ENGINES } from "./constants";
export { createClosedTab, createDefaultState, createFavorite, createId, createTab, getNextWorkspaceAccent } from "./factory";
export {
  getCachedFaviconUrl,
  getFaviconCacheKey,
  normalizeFaviconCache,
  normalizeFaviconUrl,
  setCachedFaviconUrl
} from "./favicon";
export { formatBytes } from "./formatting";
export { isInternalNewTabUrl, isInternalPageUrl } from "./internalPages";
export {
  getHomepageUrl,
  getHostInitial,
  isEssential,
  getReadableUrlTitle,
  getSearchUrl,
  getWorkspaceHomepageUrl,
  isFavorite,
  isTabFavorite,
  normalizeAddress
} from "./navigation";
export {
  applyStartupBehavior,
  normalizeClosedTabs,
  normalizeFavorites,
  normalizeState
} from "./stateNormalization";
export { getBrowserPartitions, getProfileIdForPartition, getWorkspacePartition } from "../workspaces/profiles";
export { getSplitTabIds, MAX_SPLIT_VIEW_TABS } from "../tabs/splitView";
export {
  createPasswordEntry,
  decryptSecret,
  encryptSecret,
  getVaultMetadata,
  isVaultUnlocked,
  lockVault,
  normalizeOrigin as normalizePasswordOrigin,
  passwordMatchesOrigin,
  unlockVault,
  type AddressEntry,
  type PasswordEntry,
  type PasswordDraft,
  type PaymentMethodEntry,
  type VaultMetadata
} from "./passwordVault";
export {
  importBookmarksFromHtml,
  type BookmarksImportBatch,
  type ImportedBookmarkFolder,
  type ImportedBookmarkNode
} from "./bookmarksImport";
export {
  applyReaderStyles,
  extractReaderContent,
  type ReaderContent,
  type ReaderTheme
} from "./readerMode";
export {
  buildTranslateUrl,
  detectLanguage,
  getTranslationPanelStatus,
  type TranslationProvider,
  type TranslationSettings
} from "./translation";

export type {
  AddressEntry as AutofillAddressEntry,
  AutofillDatabase,
  BookmarksImportBatch as BookmarksImportRecord,
  BrowserState,
  BrowserSettings,
  BrowserTab,
  ChromeAccentMode,
  ClosedTab,
  DownloadEntry,
  ExtensionEntry,
  FaviconCache,
  Favorite,
  HistoryEntry,
  IncognitoSessionMode,
  PartialAutofillDatabase,
  PartialBrowserState,
  PasswordEntry as AutofillPasswordEntry,
  PaymentMethodEntry as AutofillPaymentMethodEntry,
  PerOriginZoomRule,
  ProfileEntry,
  ReaderSettings,
  SearchEngineKey,
  SitePermissionRule,
  SplitLayout,
  StartupBehavior,
  TabGroup,
  ThemeKey,
  TranslationSettings as BrowserTranslationSettings,
  Workspace
} from "./types";
