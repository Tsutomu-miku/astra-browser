/* eslint-disable max-lines */
// Central type declaration file. All domain types live here to avoid
// circular imports and to keep Partial<...> variants next to their base.
export type SearchEngineKey = "google" | "duckduckgo" | "bing";
export type StartupBehavior = "restore" | "homepage";
export type ChromeAccentMode = "neutral" | "space";
export type SplitLayout = "grid" | "horizontal" | "vertical";
export type IncognitoSessionMode = "disabled" | "in-memory";
export type ThemeKey =
  | "arc-dark"
  | "arc-light"
  | "dracula"
  | "everforest"
  | "github-dark"
  | "github-light"
  | "monokai"
  | "nord"
  | "solarized-light";

/* M2: Profile (C-1) + Extension (E-1) 基础实体。 */
export interface ProfileEntry {
  id: string;
  name: string;
  color: string;
  createdAt: number;
}

export interface ExtensionEntry {
  id: string;
  name: string;
  version: string;
  description?: string;
  enabled: boolean;
  installedAt: number;
}

/* M2.4 W-3: PWA install UI state. */
export interface PwaInstallPrompt {
  origin: string;
  platforms: string[];
  title: string;
  url: string;
}

export interface InstalledPwaApp {
  id: string;
  origin: string;
  name: string;
  startUrl: string;
  icon?: string;
}

export interface SearchEngine {
  name: string;
  url: string;
}

// BrowserTab state fields fall into two categories:
//   Persistent  — restored verbatim after a process restart (id, url, title,
//                 faviconUrl, groupId, canGoBack, canGoForward, isMuted,
//                 isPinned, zoomFactor, customTitle, lastActiveAt).
//   Transient   — derived from the live <webview> at runtime and always
//                 reset to defaults on startup. These must never be trusted
//                 from serialized state: isMediaPlaying, isCameraOn,
//                 isMicrophoneOn, hasUnread, isLoading, isSleeping*.
// * isSleeping is serialized so that memory-saver state survives restarts,
//   but the tab still starts without a mounted <webview>.
export interface BrowserTab {
  id: string;
  title: string;
  customTitle?: string;
  url: string;
  faviconUrl?: string;
  groupId: string | null;
  canGoBack: boolean;
  canGoForward: boolean;
  isFavorite: boolean;
  isMuted: boolean;
  isPinned: boolean;
  isLoading: boolean;
  isSleeping: boolean;
  lastActiveAt: number;
  zoomFactor: number;
  isMediaPlaying: boolean;
  isCameraOn: boolean;
  isMicrophoneOn: boolean;
  hasUnread: boolean;
}

// ClosedTab stores a complete snapshot of a BrowserTab at close time,
// including layout (groupId, isPinned) and preferences (isMuted, zoomFactor,
// customTitle). Runtime-only flags (isMediaPlaying, isCameraOn,
// isMicrophoneOn, hasUnread, isLoading) are dropped because they describe
// a live webview's transient state, not the tab itself.
export interface ClosedTab {
  id?: string;
  title: string;
  customTitle?: string;
  url: string;
  faviconUrl?: string;
  groupId: string | null;
  canGoBack: boolean;
  canGoForward: boolean;
  isMuted: boolean;
  isPinned: boolean;
  zoomFactor: number;
  closedAt: number;
}

export interface Favorite {
  id: string;
  title: string;
  url: string;
  tabId?: string;
}

export interface TabGroup {
  id: string;
  name: string;
  color: string;
  isCollapsed: boolean;
}

export interface HistoryEntry {
  id: string;
  title: string;
  url: string;
  workspaceId: string;
  visitedAt: number;
}

export interface DownloadEntry {
  id: string;
  filename: string;
  totalBytes: number;
  receivedBytes: number;
  savePath: string;
  /** progressing | paused | completed | interrupted | cancelled */
  state: string;
  canPause?: boolean;
  url?: string;
  startedAt: number;
  finishedAt?: number;
}

export interface PasswordEntry {
  id: string;
  origin: string;
  username: string;
  encryptedPassword: string;
  notes?: string;
  usedAt?: number;
  createdAt: number;
  updatedAt: number;
}

export interface AddressEntry {
  id: string;
  label: string;
  recipient: string;
  address1: string;
  address2?: string;
  city: string;
  region?: string;
  postalCode?: string;
  country?: string;
  phone?: string;
  email?: string;
  createdAt: number;
}

export interface PaymentMethodEntry {
  id: string;
  label: string;
  cardholderName: string;
  cardLastFour: string;
  encryptedCardDetails: string;
  expiryMonth?: number;
  expiryYear?: number;
  createdAt: number;
  updatedAt: number;
}

export interface AutofillDatabase {
  passwords: PasswordEntry[];
  addresses: AddressEntry[];
  paymentMethods: PaymentMethodEntry[];
}

export type ReaderTheme = "light" | "sepia" | "dark";

export interface ReaderSettings {
  enabled: boolean;
  theme: ReaderTheme;
  fontSize: number;
  fontFamily: string;
  lineHeight: number;
  contentWidth: number;
}

export type TranslationProvider = "google" | "libretranslate" | "disabled";

export interface TranslationSettings {
  provider: TranslationProvider;
  autoTranslate: boolean;
  preferredTarget: string;
  /** origin 级例外：跳过翻译 */
  skipOrigins: string[];
  lastUsed?: string;
}

export interface BookmarksImportBatch {
  id: string;
  source: "chrome" | "edge" | "firefox" | "safari" | "html";
  importedAt: number;
  importedEssentials: string[];
  importedFavorites: string[];
}

export type FaviconCache = Record<string, string>;

export type SitePermissionDecision = "allow" | "block";

export interface SitePermissionRule {
  profileId: string;
  origin: string;
  permission: string;
  decision: SitePermissionDecision;
  updatedAt: number;
}

export interface PerOriginZoomRule {
  origin: string;
  zoomFactor: number;
  updatedAt: number;
}

export interface BrowserSettings {
  chromeAccentMode: ChromeAccentMode;
  defaultZoomFactor: number;
  homepage: string;
  incognito: IncognitoSessionMode;
  memorySaverEnabled: boolean;
  memorySaverIdleMinutes: number;
  perOriginZoom: PerOriginZoomRule[];
  searchEngine: SearchEngineKey;
  startupBehavior: StartupBehavior;
  theme: ThemeKey;
  autofill: AutofillDatabase;
  reader: ReaderSettings;
  translation: TranslationSettings;

  /* M2: Print (W-7) */
  printHeaders?: boolean;
  printBackgrounds?: boolean;
  printPaperSize?: "A4" | "Letter" | "Legal" | "Tabloid" | string;
  printScale?: number;
  printMargins?: "default" | "none" | "minimal" | "custom";
  printLandscape?: boolean;
  printCollate?: boolean;
  printCopies?: number;
  printColorMode?: "color" | "grayscale";
  /** When set, the next print should produce a PDF at this path (then reset to empty). */
  printOutputPath?: string;

  /* M2: System / Safe Browsing (W-14 / K-6) */
  backgroundAppMode?: boolean;
  hardwareAcceleration?: boolean;
  lowPowerMode?: boolean;
  forceHttps?: boolean;
  safeBrowsingEnabled?: boolean;

  /* M2: Profile (C-1) */
  activeProfileId?: string;
}

export interface Workspace {
  id: string;
  name: string;
  accent: string;
  homepage: string;
  profileId: string;
  profileName: string;
  splitLayout: SplitLayout;
  closedTabs: ClosedTab[];
  favoriteOrder: string[];
  tabGroups: TabGroup[];
  tabs: BrowserTab[];
  activeTabId: string | null;
}

export interface BrowserState {
  activeWorkspaceId: string;
  faviconCache: FaviconCache;
  splitMode: boolean;
  splitTabId: string | null;
  splitTabIds: string[];
  essentials: Favorite[];
  history: HistoryEntry[];
  downloads: DownloadEntry[];
  sitePermissions: SitePermissionRule[];
  settings: BrowserSettings;
  profiles: ProfileEntry[];
  extensions: ExtensionEntry[];
  workspaces: Workspace[];
  /* M2.4 W-3 PWA install: origin-keyed pending prompts + installed registry. */
  pendingPwaInstallPrompts: PwaInstallPrompt[];
  installedPwaApps: InstalledPwaApp[];
}

export type PartialWorkspace = Partial<Omit<Workspace, "closedTabs" | "favoriteOrder" | "tabGroups" | "tabs">> & {
  closedTabs?: Array<Partial<ClosedTab> | null>;
  favoriteOrder?: Array<string | null>;
  tabGroups?: Array<Partial<TabGroup> | null>;
  tabs?: Array<Partial<BrowserTab> | null>;
};

export type PartialAutofillDatabase = {
  passwords?: Array<Partial<PasswordEntry> | null>;
  addresses?: Array<Partial<AddressEntry> | null>;
  paymentMethods?: Array<Partial<PaymentMethodEntry> | null>;
};

export type PartialReaderSettings = Partial<ReaderSettings>;
export type PartialTranslationSettings = Partial<TranslationSettings>;

export type PartialBrowserState = Partial<Omit<BrowserState, "settings" | "workspaces" | "profiles" | "extensions">> & {
  essentials?: Array<Partial<Favorite> | null>;
  faviconCache?: Record<string, unknown>;
  perOriginZoom?: Array<Partial<PerOriginZoomRule> | null>;
  settings?: Partial<Omit<BrowserSettings, "autofill" | "reader" | "translation">> & {
    autofill?: PartialAutofillDatabase;
    reader?: PartialReaderSettings;
    translation?: PartialTranslationSettings;
  };
  sitePermissions?: Array<Partial<SitePermissionRule> | null>;
  profiles?: Array<Partial<ProfileEntry> | null>;
  extensions?: Array<Partial<ExtensionEntry> | null>;
  pendingPwaInstallPrompts?: Array<Partial<PwaInstallPrompt> | null>;
  installedPwaApps?: Array<Partial<InstalledPwaApp> | null>;
  workspaces?: PartialWorkspace[];
};
