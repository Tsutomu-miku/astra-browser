import { describe, expect, it } from "vitest";

import { createFavorite, createTab, type Workspace } from "../src/renderer/domain/browser";
import { createTabGroup } from "../src/renderer/domain/tabs/groups";
import { getSidebarTabLocations } from "../src/renderer/surfaces/sidebar/model/sidebarTabLocations";

describe("sidebar tab locations", () => {
  it("keeps tab-backed Favorites out of pinned, grouped, and regular sections", () => {
    const group = createTabGroup("Research");
    const favoriteTab = createTab("Docs", "https://docs.example");
    const pinnedFavoriteTab = { ...createTab("Mail", "https://mail.example"), isPinned: true };
    const groupedFavoriteTab = { ...createTab("Issue", "https://issue.example"), groupId: group.id };
    const groupedTab = { ...createTab("Chromium", "https://chromium.example"), groupId: group.id };
    const pinnedTab = { ...createTab("Calendar", "https://calendar.example"), isPinned: true };
    const regularTab = createTab("News", "https://news.example");
    const workspace = createWorkspace({
      favorites: [
        createFavorite("Docs", favoriteTab.url, favoriteTab.id),
        createFavorite("Mail", pinnedFavoriteTab.url, pinnedFavoriteTab.id),
        createFavorite("Issue", groupedFavoriteTab.url, groupedFavoriteTab.id)
      ],
      tabGroups: [group],
      tabs: [favoriteTab, pinnedFavoriteTab, groupedFavoriteTab, groupedTab, pinnedTab, regularTab]
    });

    const locations = getSidebarTabLocations(workspace);

    expect([...locations.favoriteTabIds]).toEqual([favoriteTab.id, pinnedFavoriteTab.id, groupedFavoriteTab.id]);
    expect(locations.pinnedTabs.map((tab) => tab.title)).toEqual(["Calendar"]);
    expect(locations.groupedTabs[0].tabs.map((tab) => tab.title)).toEqual(["Chromium"]);
    expect(locations.regularTabs.map((tab) => tab.title)).toEqual(["News"]);
  });

  it("uses URL fallback only for legacy Favorites when deriving the Favorite location", () => {
    const tab = createTab("Docs", "https://docs.example");
    const workspace = createWorkspace({
      favorites: [createFavorite("Legacy Docs", tab.url)],
      tabs: [tab]
    });

    expect(getSidebarTabLocations(workspace).regularTabs).toHaveLength(0);
    expect([...getSidebarTabLocations(workspace).favoriteTabIds]).toEqual([tab.id]);
  });
});

function createWorkspace(patch: Partial<Workspace>): Workspace {
  return {
    accent: "#7dd3fc",
    activeTabId: patch.tabs?.[0]?.id ?? null,
    closedTabs: [],
    favorites: [],
    homepage: "https://start.example",
    id: "workspace",
    name: "Workspace",
    profileId: "profile",
    profileName: "Profile",
    tabGroups: [],
    tabs: [],
    ...patch
  };
}
