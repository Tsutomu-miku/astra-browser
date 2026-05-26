import { describe, expect, it } from "vitest";

import { createFavorite, createTab } from "../src/renderer/domain/browser-core";
import { createTabGroup } from "../src/renderer/domain/tab-groups";
import {
  clampSidebarSearchIndex,
  filterSidebarItems,
  getNextSidebarSearchIndex,
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
    const grouped = createTab("Chromium", "https://chromium.example");
    const regular = createTab("News", "https://news.example");
    const result = filterSidebarItems({
      essentials: [essential],
      favorites: [favorite],
      groupedTabs: [{ group, tabs: [grouped] }],
      pinnedTabs: [pinned],
      regularTabs: [regular]
    }, "");

    expect(getSidebarSearchTargets(result).map((target) => `${target.type}:${target.title}`)).toEqual([
      "essential:Inbox",
      "tab:Mail",
      "favorite:Docs",
      "tab:Chromium",
      "tab:News"
    ]);
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
});
