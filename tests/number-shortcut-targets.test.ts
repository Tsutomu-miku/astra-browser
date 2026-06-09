import { describe, expect, it } from "vitest";

import {
  getLastNumberShortcutTabTarget,
  getNumberShortcutTarget,
  getNumberShortcutTabs
} from "../src/renderer/common/shortcuts/numberShortcutTargets";
import { createFavorite, createTab, type TabGroup, type Workspace } from "../src/renderer/domain/browser";

function workspaceWithTabs(
  tabs: Workspace["tabs"],
  tabGroups: Workspace["tabGroups"] = [],
  favoriteOrder?: Workspace["favoriteOrder"]
): Pick<Workspace, "favoriteOrder" | "tabs" | "tabGroups"> {
  const order = favoriteOrder ?? tabs.filter((tab) => tab.isFavorite).map((tab) => tab.id);
  return { favoriteOrder: order, tabGroups, tabs };
}

describe("getNumberShortcutTarget", () => {
  it("orders Essentials before pinned tabs and regular tabs", () => {
    const essential = createFavorite("Mail", "https://mail.example");
    const regular = createTab("Docs", "https://docs.example");
    const pinned = { ...createTab("Calendar", "https://calendar.example"), isPinned: true };
    const secondRegular = createTab("News", "https://news.example");

    const workspace = workspaceWithTabs([regular, pinned, secondRegular]);

    expect(getNumberShortcutTarget([essential], workspace, 0)).toEqual({
      type: "essential",
      title: essential.title,
      url: essential.url
    });
    expect(getNumberShortcutTarget([essential], workspace, 1)).toEqual({ type: "tab", tabId: pinned.id });
    expect(getNumberShortcutTarget([essential], workspace, 2)).toEqual({ type: "tab", tabId: regular.id });
    expect(getNumberShortcutTarget([essential], workspace, 3)).toEqual({ type: "tab", tabId: secondRegular.id });
    expect(getNumberShortcutTabs(workspace).map((tab) => tab.id)).toEqual([
      pinned.id,
      regular.id,
      secondRegular.id
    ]);
  });

  it("orders grouped tabs by sidebar group order before regular tabs", () => {
    const firstRegular = createTab("Docs", "https://docs.example");
    const groupedInSecondGroup = { ...createTab("Design", "https://design.example"), groupId: "group-b" };
    const groupedInFirstGroup = { ...createTab("Planning", "https://planning.example"), groupId: "group-a" };
    const lastRegular = createTab("News", "https://news.example");
    const pinned = { ...createTab("Pinned", "https://pinned.example"), isPinned: true };
    const workspace = workspaceWithTabs(
      [firstRegular, groupedInSecondGroup, lastRegular, pinned, groupedInFirstGroup],
      [tabGroup("group-a", "Group A"), tabGroup("group-b", "Group B")]
    );

    expect(getNumberShortcutTabs(workspace).map((tab) => tab.id)).toEqual([
      pinned.id,
      groupedInFirstGroup.id,
      groupedInSecondGroup.id,
      firstRegular.id,
      lastRegular.id
    ]);
    expect(getNumberShortcutTarget([], workspace, 1)).toEqual({ type: "tab", tabId: groupedInFirstGroup.id });
  });

  it("orders tab-backed Favorites after pinned tabs and excludes them from regular folders", () => {
    const pinned = { ...createTab("Pinned", "https://pinned.example"), isPinned: true };
    const favoriteTab = { ...createTab("Favorite", "https://favorite.example"), isFavorite: true };
    const groupedFavoriteTab = { ...createTab("Grouped Favorite", "https://grouped-favorite.example"), groupId: "group", isFavorite: true };
    const grouped = { ...createTab("Grouped", "https://grouped.example"), groupId: "group" };
    const regular = createTab("Docs", "https://docs.example");
    const workspace = workspaceWithTabs(
      [regular, favoriteTab, grouped, pinned, groupedFavoriteTab],
      [tabGroup("group", "Group")]
    );

    expect(getNumberShortcutTabs(workspace).map((tab) => tab.id)).toEqual([
      pinned.id,
      favoriteTab.id,
      groupedFavoriteTab.id,
      grouped.id,
      regular.id
    ]);
    expect(getNumberShortcutTarget([], workspace, 1)).toEqual({ type: "tab", tabId: favoriteTab.id });
  });

  it("non-favorite tabs are not ordered as favorites", () => {
    const favoriteTab = { ...createTab("Favorite", "https://favorite.example"), isFavorite: true };
    const regular = createTab("Docs", "https://docs.example");
    const workspace = workspaceWithTabs([regular, favoriteTab]);

    // Only favoriteTab should appear in the favorites section; regular stays in normal order
    expect(getNumberShortcutTabs(workspace).map((tab) => tab.id)).toEqual([
      favoriteTab.id,
      regular.id
    ]);
    // favoriteOrder in workspace should only contain the favorite tab
    expect(workspace.favoriteOrder).toEqual([favoriteTab.id]);
  });

  it("skips tabs hidden inside collapsed groups for visual-order shortcuts", () => {
    const hiddenGrouped = { ...createTab("Hidden", "https://hidden.example"), groupId: "group" };
    const regular = createTab("Docs", "https://docs.example");
    const workspace = workspaceWithTabs(
      [hiddenGrouped, regular],
      [tabGroup("group", "Group", true)]
    );

    expect(getNumberShortcutTabs(workspace).map((tab) => tab.id)).toEqual([regular.id]);
    expect(getLastNumberShortcutTabTarget(workspace)).toEqual({ type: "tab", tabId: regular.id });
  });

  it("selects the last tab using sidebar visual order", () => {
    const trailingPinned = { ...createTab("Pinned", "https://pinned.example"), isPinned: true };
    const firstRegular = createTab("Docs", "https://docs.example");
    const lastRegular = createTab("News", "https://news.example");
    const workspace = workspaceWithTabs([firstRegular, lastRegular, trailingPinned]);

    expect(getLastNumberShortcutTabTarget(workspace)).toEqual({ type: "tab", tabId: lastRegular.id });
  });

  it("returns null outside the available shortcut targets", () => {
    const workspace = workspaceWithTabs([createTab("Docs", "https://docs.example")]);

    expect(getNumberShortcutTarget([], workspace, -1)).toBeNull();
    expect(getNumberShortcutTarget([], workspace, 1)).toBeNull();
  });
});

function tabGroup(id: string, name: string, isCollapsed = false): TabGroup {
  return {
    color: "#7dd3fc",
    id,
    isCollapsed,
    name
  };
}
