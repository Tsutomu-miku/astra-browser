import { describe, expect, it } from "vitest";

import { createFavorite, createTab } from "../src/renderer/domain/browser-core";
import { createTabGroup } from "../src/renderer/domain/tab-groups";
import { filterSidebarItems } from "../src/renderer/surfaces/sidebar/sidebarFiltering";

describe("sidebar filtering", () => {
  it("filters pinned, favorite, grouped, and regular sidebar items", () => {
    const group = createTabGroup("Research");
    const groupedTab = { ...createTab("Chromium Docs", "https://chromium.org"), groupId: group.id };
    const result = filterSidebarItems({
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
    expect(result.groupedTabs).toHaveLength(1);
    expect(result.groupedTabs[0].tabs.map((tab) => tab.title)).toEqual(["Chromium Docs"]);
    expect(result.favorites).toHaveLength(0);
    expect(result.pinnedTabs).toHaveLength(0);
    expect(result.regularTabs).toHaveLength(0);
  });

  it("keeps all tabs in a group when the group name matches", () => {
    const group = createTabGroup("Research");
    const result = filterSidebarItems({
      favorites: [],
      groupedTabs: [{ group, tabs: [createTab("A", "https://a.example"), createTab("B", "https://b.example")] }],
      pinnedTabs: [],
      regularTabs: []
    }, "research");

    expect(result.groupedTabs[0].tabs).toHaveLength(2);
  });

  it("reports empty filtered results", () => {
    const result = filterSidebarItems({
      favorites: [],
      groupedTabs: [],
      pinnedTabs: [],
      regularTabs: [createTab("News", "https://news.example")]
    }, "missing");

    expect(result.isFiltering).toBe(true);
    expect(result.hasMatches).toBe(false);
  });
});
