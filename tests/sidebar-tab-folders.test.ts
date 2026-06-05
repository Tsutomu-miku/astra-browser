import { describe, expect, it } from "vitest";

import { createFavorite, createTab, type SplitLayout, type Workspace } from "../src/renderer/domain/browser";
import { createTabGroup } from "../src/renderer/domain/tabs/groups";
import { getSidebarTabFolders } from "../src/renderer/surfaces/sidebar/model/sidebarTabFolders";

describe("sidebar tab folders", () => {
  it("keeps tab-backed Favorites in the Favorites folder only", () => {
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

    const folders = getSidebarTabFolders(workspace);

    expect(folders.pinnedTabs.map((tab) => tab.title)).toEqual(["Calendar"]);
    expect(folders.groupedTabs[0].tabs.map((tab) => tab.title)).toEqual(["Chromium"]);
    expect(folders.regularTabs.map((tab) => tab.title)).toEqual(["News"]);
  });

  it("keeps URL-only legacy Favorites out of tab folder ownership", () => {
    const tab = createTab("Docs", "https://docs.example");
    const workspace = createWorkspace({
      favorites: [createFavorite("Legacy Docs", tab.url)],
      tabs: [tab]
    });

    expect(getSidebarTabFolders(workspace).regularTabs).toEqual([tab]);
  });

  it("uses URL fallback for stale tab-backed Favorites when deriving folder ownership", () => {
    const tab = createTab("Docs", "https://docs.example");
    const workspace = createWorkspace({
      favorites: [createFavorite("Legacy Docs", tab.url, "missing-tab")],
      tabs: [tab]
    });

    expect(getSidebarTabFolders(workspace).regularTabs).toHaveLength(0);
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
    splitLayout: "horizontal" as SplitLayout,
    tabGroups: [],
    tabs: [],
    ...patch
  };
}
