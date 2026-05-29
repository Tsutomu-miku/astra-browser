import { DEFAULT_URL, WORKSPACE_ACCENTS } from "./constants";
import type { BrowserState, BrowserTab, Favorite } from "./types";
import { DEFAULT_ZOOM_FACTOR } from "./zoom";

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
    isMuted: false,
    isPinned: false,
    isLoading: false,
    isSleeping: false,
    lastActiveAt: Date.now(),
    zoomFactor: DEFAULT_ZOOM_FACTOR
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
      homepage: DEFAULT_URL,
      memorySaverEnabled: true,
      memorySaverIdleMinutes: 30,
      searchEngine: "google",
      startupBehavior: "restore"
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
        favorites: [
          createFavorite("Chromium", "https://www.chromium.org"),
          createFavorite("MDN", "https://developer.mozilla.org")
        ],
        tabGroups: [],
        tabs: [
          createTab("New Tab", DEFAULT_URL)
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
        closedTabs: [],
        favorites: [
          createFavorite("GitHub", "https://github.com")
        ],
        tabGroups: [],
        tabs: [
          createTab("Docs", "https://www.chromium.org")
        ],
        activeTabId: null
      }
    ]
  };
}

export function getNextWorkspaceAccent(index: number): string {
  return WORKSPACE_ACCENTS[index % WORKSPACE_ACCENTS.length];
}
