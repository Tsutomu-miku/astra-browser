import { describe, expect, it } from "vitest";

import {
  createDefaultState,
  formatBytes,
  getHomepageUrl,
  getHostInitial,
  getNextWorkspaceAccent,
  getWorkspaceHomepageUrl,
  isInternalNewTabUrl,
  isEssential,
  isFavorite,
  normalizeAddress,
  normalizeState,
  applyStartupBehavior
} from "../src/renderer/domain/browser-core";

describe("browser-core", () => {
  it("normalizes plain domains and search queries", () => {
    expect(normalizeAddress("example.com")).toBe("https://example.com/");
    expect(normalizeAddress("zen browser", "duckduckgo")).toBe("https://duckduckgo.com/?q=zen%20browser");
    expect(normalizeAddress("")).toBe("astra://newtab");
    expect(isInternalNewTabUrl("astra://newtab")).toBe(true);
  });

  it("normalizes persisted state shape for migrations", () => {
    const state = normalizeState({
      activeWorkspaceId: "missing",
      settings: {
        homepage: "about:blank",
        searchEngine: "unknown" as never
      },
      workspaces: [
        {
          id: "space",
          name: "",
          accent: "bad",
          closedTabs: [{ url: "closed.example" }],
          favorites: [{ url: "github.com" }],
          tabs: [],
          activeTabId: null
        }
      ]
    });

    expect(state.activeWorkspaceId).toBe("space");
    expect(state.settings.searchEngine).toBe("google");
    expect(state.settings.startupBehavior).toBe("restore");
    expect(state.workspaces[0].name).toBe("Space");
    expect(state.workspaces[0].homepage).toBe("about:blank");
    expect(state.workspaces[0].favorites[0].url).toBe("https://github.com/");
    expect(state.workspaces[0].closedTabs[0].url).toBe("https://closed.example/");
    expect(state.essentials).toEqual([]);
    expect(state.workspaces[0].tabs).toHaveLength(1);
    expect(state.workspaces[0].tabs[0].url).toBe("about:blank");
    expect(state.workspaces[0].tabs[0].canGoBack).toBe(false);
    expect(state.workspaces[0].tabs[0].canGoForward).toBe(false);
    expect(state.workspaces[0].tabs[0].isMuted).toBe(false);
    expect(state.workspaces[0].tabs[0].zoomFactor).toBe(1);
  });

  it("can reset restored tabs to homepage on startup", () => {
    const state = normalizeState({
      settings: {
        homepage: "start.example",
        startupBehavior: "homepage"
      },
      activeWorkspaceId: "space",
      splitMode: true,
      splitTabId: "second",
      workspaces: [
        {
          id: "space",
          name: "Space",
          homepage: "space.example",
          tabGroups: [{ id: "group", name: "Docs", color: "#123456", isCollapsed: false }],
          tabs: [
            { id: "first", title: "First", url: "first.example", groupId: "group" },
            { id: "second", title: "Second", url: "second.example", groupId: "group" }
          ],
          activeTabId: "second"
        }
      ]
    });
    const started = applyStartupBehavior(state);

    expect(started.splitMode).toBe(false);
    expect(started.splitTabId).toBeNull();
    expect(started.splitTabIds).toEqual([]);
    expect(started.workspaces[0].tabs).toHaveLength(1);
    expect(started.workspaces[0].tabs[0].url).toBe("https://space.example/");
    expect(started.workspaces[0].tabGroups).toHaveLength(0);
  });

  it("supports homepage, favorite, favicon, accent, and byte helpers", () => {
    const state = createDefaultState();
    expect(getHomepageUrl(state)).toBe("astra://newtab");
    expect(state.workspaces[0].tabs[0].url).toBe("astra://newtab");

    state.settings.homepage = "chromium.org";

    expect(getHomepageUrl(state)).toBe("https://chromium.org/");
    expect(getWorkspaceHomepageUrl(state, { homepage: "space.example" })).toBe("https://space.example/");
    expect(getHostInitial("https://developer.mozilla.org")).toBe("D");
    expect(isFavorite(state.workspaces[0], "https://www.chromium.org")).toBe(true);
    expect(isEssential(state, "https://github.com")).toBe(true);
    expect(getNextWorkspaceAccent(2)).toBe("#86efac");
    expect(formatBytes(1536)).toBe("1.5 KB");
  });
});
