import { describe, expect, it } from "vitest";

import { createFavorite, createTab } from "../src/renderer/domain/browser";
import { createTabGroup } from "../src/renderer/domain/tabs/groups";
import {
  clampSidebarSearchIndex,
  filterSidebarItems,
  getNextSidebarSearchIndex,
  getSidebarSearchActionHints,
  getSidebarSearchTargetElementId,
  getSidebarSearchTargets
} from "../src/renderer/surfaces/sidebar/sidebarFiltering";

describe("sidebar filtering", () => {
  it("filters essentials, pinned, favorite, grouped, and regular sidebar items", () => {
    const group = createTabGroup("Research");
    const groupedTab = { ...createTab("Chromium Docs", "https://chromium.org"), groupId: group.id };
    const result = filterSidebarItems({
      essentials: [
        createFavorite("Chromium", "https://www.chromium.org"),
        createFavorite("Docs", "https://docs.example")
      ],
      favorites: [
        createFavorite("MDN", "https://developer.mozilla.org"),
        createFavorite("GitHub", "https://github.com")
      ],
      groupedTabs: [{ group, tabs: [groupedTab, createTab("Calendar", "https://calendar.example")] }],
      pinnedTabs: [createTab("Mail", "https://mail.example")],
      regularTabs: [createTab("News", "https://news.example")]
    }, "chromium");

    expect(result.isFiltering).toBe(true);
    expect(result.hasMatches).toBe(true);
    expect(result.essentials.map((essential) => essential.title)).toEqual(["Chromium"]);
    expect(result.groupedTabs).toHaveLength(1);
    expect(result.groupedTabs[0].tabs.map((tab) => tab.title)).toEqual(["Chromium Docs"]);
    expect(result.favorites).toHaveLength(0);
    expect(result.pinnedTabs).toHaveLength(0);
    expect(result.regularTabs).toHaveLength(0);
  });

  it("keeps all tabs in a group when the group name matches", () => {
    const group = createTabGroup("Research");
    const result = filterSidebarItems({
      essentials: [],
      favorites: [],
      groupedTabs: [{ group, tabs: [createTab("A", "https://a.example"), createTab("B", "https://b.example")] }],
      pinnedTabs: [],
      regularTabs: []
    }, "research");

    expect(result.groupedTabs[0].tabs).toHaveLength(2);
  });

  it("reports empty filtered results", () => {
    const result = filterSidebarItems({
      essentials: [],
      favorites: [],
      groupedTabs: [],
      pinnedTabs: [],
      regularTabs: [createTab("News", "https://news.example")]
    }, "missing");

    expect(result.isFiltering).toBe(true);
    expect(result.hasMatches).toBe(false);
  });

  it("flattens search targets in rendered sidebar order", () => {
    const group = createTabGroup("Research");
    const essential = createFavorite("Inbox", "https://mail.example");
    const pinned = createTab("Mail", "https://mail.example");
    const favorite = createFavorite("Docs", "https://docs.example");
    const favoriteTab = createTab("Docs Tab", favorite.url);
    const grouped = createTab("Chromium", "https://chromium.example");
    const regular = createTab("News", "https://news.example");
    const result = filterSidebarItems({
      essentials: [essential],
      favorites: [favorite],
      groupedTabs: [{ group, tabs: [grouped] }],
      pinnedTabs: [pinned],
      regularTabs: [regular],
      workspaceTabs: [pinned, favoriteTab, grouped, regular]
    }, "");

    const targets = getSidebarSearchTargets(result);
    expect(targets.map((target) => `${target.type}:${target.title}`)).toEqual([
      "essential:Inbox",
      "tab:Mail",
      "favorite:Docs Tab",
      "tab:Chromium",
      "tab:News"
    ]);
    expect(targets.find((target) => target.type === "favorite")).toMatchObject({
      tabId: favoriteTab.id,
      type: "favorite"
    });
  });

  it("falls back by URL for stale Favorite tab ids in search targets", () => {
    const favorite = createFavorite("Docs", "https://docs.example", "missing-tab");
    const favoriteTab = createTab("Docs Tab", favorite.url);
    const result = filterSidebarItems({
      essentials: [],
      favorites: [favorite],
      groupedTabs: [],
      pinnedTabs: [],
      regularTabs: [],
      workspaceTabs: [favoriteTab]
    }, "");

    expect(getSidebarSearchTargets(result).find((target) => target.type === "favorite")).toMatchObject({
      tabId: favoriteTab.id,
      type: "favorite"
    });
  });

  it("matches tab-backed Favorites by the current backing tab title", () => {
    const favorite = createFavorite("Old Docs", "https://docs.example", "docs-tab");
    const favoriteTab = {
      ...createTab("Current Project Brief", "https://docs.example/current"),
      id: "docs-tab"
    };
    const result = filterSidebarItems({
      essentials: [],
      favorites: [favorite],
      groupedTabs: [],
      pinnedTabs: [],
      regularTabs: [],
      workspaceTabs: [favoriteTab]
    }, "brief");

    expect(result.favorites).toEqual([favorite]);
    expect(getSidebarSearchTargets(result)).toEqual([{
      id: favorite.id,
      tabId: favoriteTab.id,
      title: favoriteTab.title,
      type: "favorite",
      url: favoriteTab.url
    }]);
  });

  it("clamps and wraps keyboard search selection", () => {
    expect(clampSidebarSearchIndex(-1, 3)).toBe(0);
    expect(clampSidebarSearchIndex(5, 3)).toBe(2);
    expect(clampSidebarSearchIndex(Number.NaN, 3)).toBe(0);
    expect(clampSidebarSearchIndex(1, 0)).toBe(0);
    expect(getNextSidebarSearchIndex(2, 3, "ArrowDown")).toBe(0);
    expect(getNextSidebarSearchIndex(0, 3, "ArrowUp")).toBe(2);
    expect(getNextSidebarSearchIndex(1, 3, "Home")).toBe(0);
    expect(getNextSidebarSearchIndex(1, 3, "End")).toBe(2);
  });

  it("exposes preview and split action hints for search targets", () => {
    expect(getSidebarSearchActionHints({
      type: "favorite",
      id: "docs",
      title: "Docs",
      url: "https://docs.example"
    })).toEqual([
      { id: "preview", modifier: "Alt", label: "Preview" },
      { id: "split", modifier: "Shift", label: "Split" }
    ]);
    expect(getSidebarSearchActionHints(undefined)).toEqual([]);
  });

  it("builds stable DOM ids for rendered search targets", () => {
    expect(getSidebarSearchTargetElementId({
      type: "tab",
      id: "tab-1",
      title: "Docs",
      url: "https://docs.example"
    })).toBe("sidebar-search-tab-tab-1");
    expect(getSidebarSearchTargetElementId({
      type: "favorite",
      id: "docs",
      title: "Docs",
      url: "https://docs.example"
    })).toBe("sidebar-search-favorite-docs");
  });
});
