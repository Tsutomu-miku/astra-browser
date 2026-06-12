import { DEFAULT_URL, WORKSPACE_ACCENTS } from "./constants";
import type { BrowserState, BrowserTab, ClosedTab, Favorite, SplitLayout } from "./types";
import { DEFAULT_ZOOM_FACTOR } from "./zoom";

const DEFAULT_WORKSPACE_SPLIT_LAYOUT: SplitLayout = "horizontal";

export function createId(): string {
  return globalThis.crypto?.randomUUID?.() ?? `${Date.now()}-${Math.random().toString(16).slice(2)}`;
}

export function createTab(title: string, url: string): BrowserTab {
  return {
    id: createId(),
    title,
    url,
    faviconUrl: undefined,
    groupId: null,
    canGoBack: false,
    canGoForward: false,
    isFavorite: false,
    isMuted: false,
    isPinned: false,
    isLoading: false,
    isSleeping: false,
    lastActiveAt: Date.now(),
    zoomFactor: DEFAULT_ZOOM_FACTOR,
    isMediaPlaying: false,
    isCameraOn: false,
    isMicrophoneOn: false,
    hasUnread: false
  };
}

export function createClosedTab(
  title: string,
  url: string,
  partial: Partial<ClosedTab> = {}
): ClosedTab {
  return {
    title,
    url,
    faviconUrl: undefined,
    customTitle: undefined,
    groupId: null,
    canGoBack: false,
    canGoForward: false,
    isMuted: false,
    isPinned: false,
    zoomFactor: DEFAULT_ZOOM_FACTOR,
    closedAt: Date.now(),
    ...partial
  };
}

export function createFavorite(title: string, url: string, tabId?: string): Favorite {
  return {
    id: createId(),
    title,
    ...(tabId ? { tabId } : {}),
    url
  };
}

export function createDefaultState(): BrowserState {
  const chromiumTab = createTab("Chromium", "https://www.chromium.org");
  chromiumTab.isFavorite = true;
  const mdnTab = createTab("MDN", "https://developer.mozilla.org");
  mdnTab.isFavorite = true;
  const githubTab = createTab("GitHub", "https://github.com");
  githubTab.isFavorite = true;

  return {
    activeWorkspaceId: "personal",
    faviconCache: {},
    splitMode: false,
    splitTabId: null,
    splitTabIds: [],
    essentials: [
      createFavorite("GitHub", "https://github.com"),
      createFavorite("MDN", "https://developer.mozilla.org")
    ],
    history: [],
    downloads: [],
    sitePermissions: [],
    settings: {
      chromeAccentMode: "neutral",
      defaultZoomFactor: 1,
      homepage: DEFAULT_URL,
      incognito: "disabled",
      memorySaverEnabled: true,
      memorySaverIdleMinutes: 30,
      perOriginZoom: [],
      searchEngine: "google",
      startupBehavior: "restore",
      theme: "arc-dark",
      autofill: { passwords: [], addresses: [], paymentMethods: [] },
      reader: {
        enabled: false,
        theme: "light",
        fontSize: 16,
        fontFamily: "serif",
        lineHeight: 1.6,
        contentWidth: 70
      },
      translation: {
        provider: "google",
        autoTranslate: false,
        preferredTarget: "zh-CN",
        skipOrigins: []
      },
      printHeaders: true,
      printBackgrounds: false,
      printPaperSize: "A4",
      printScale: 1,
      backgroundAppMode: true,
      hardwareAcceleration: true,
      lowPowerMode: false,
      forceHttps: false,
      safeBrowsingEnabled: true,
      activeProfileId: "personal"
    },
    profiles: [
      { id: "personal", name: "Personal", color: "#7dd3fc", createdAt: Date.now() }
    ],
    extensions: [],
    pendingPwaInstallPrompts: [],
    installedPwaApps: [],
    workspaces: [
      {
        id: "personal",
        name: "Personal",
        accent: "#7dd3fc",
        homepage: DEFAULT_URL,
        profileId: "personal",
        profileName: "Personal",
        splitLayout: DEFAULT_WORKSPACE_SPLIT_LAYOUT,
        splitMode: false,
        ancillaryTabIds: [],
        activeAncillaryTabId: null,
        splitSide: "right",
        closedTabs: [],
        favoriteOrder: [chromiumTab.id, mdnTab.id],
        tabGroups: [],
        splitTabs: [],
        activeSplitId: null,
        tabs: [
          createTab("New Tab", DEFAULT_URL),
          chromiumTab,
          mdnTab
        ],
        activeTabId: null
      },
      {
        id: "work",
        name: "Work",
        accent: "#f0abfc",
        homepage: "https://github.com",
        profileId: "work",
        profileName: "Work",
        splitLayout: DEFAULT_WORKSPACE_SPLIT_LAYOUT,
        splitMode: false,
        ancillaryTabIds: [],
        activeAncillaryTabId: null,
        splitSide: "right",
        closedTabs: [],
        favoriteOrder: [githubTab.id],
        tabGroups: [],
        splitTabs: [],
        activeSplitId: null,
        tabs: [
          createTab("Docs", "https://www.chromium.org"),
          githubTab
        ],
        activeTabId: null
      }
    ]
  };
}

export function getNextWorkspaceAccent(index: number): string {
  return WORKSPACE_ACCENTS[index % WORKSPACE_ACCENTS.length];
}
