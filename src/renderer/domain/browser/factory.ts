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
      homepage: DEFAULT_URL,
      memorySaverEnabled: true,
      memorySaverIdleMinutes: 30,
      searchEngine: "google",
      startupBehavior: "restore",
      theme: "arc-dark"
    },
    workspaces: [
      {
        id: "personal",
        name: "Personal",
        accent: "#7dd3fc",
        homepage: DEFAULT_URL,
        profileId: "personal",
        profileName: "Personal",
        closedTabs: [],
        favoriteOrder: [chromiumTab.id, mdnTab.id],
        tabGroups: [],
        tabs: [
          createTab("New Tab", DEFAULT_URL),
          chromiumTab,
          mdnTab
        ],
        activeTabId: null,
        splitLayout: DEFAULT_WORKSPACE_SPLIT_LAYOUT
      },
      {
        id: "work",
        name: "Work",
        accent: "#f0abfc",
        homepage: "https://github.com",
        profileId: "work",
        profileName: "Work",
        closedTabs: [],
        favoriteOrder: [githubTab.id],
        tabGroups: [],
        tabs: [
          createTab("Docs", "https://www.chromium.org"),
          githubTab
        ],
        activeTabId: null,
        splitLayout: DEFAULT_WORKSPACE_SPLIT_LAYOUT
      }
    ]
  };
}

export function getNextWorkspaceAccent(index: number): string {
  return WORKSPACE_ACCENTS[index % WORKSPACE_ACCENTS.length];
}
