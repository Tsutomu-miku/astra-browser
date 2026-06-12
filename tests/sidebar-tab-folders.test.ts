import { describe, expect, it } from "vitest";

import { createTab, type SplitLayout, type Workspace } from "../src/renderer/domain/browser";
import { createTabGroup } from "../src/renderer/domain/tabs/groups";
import { getSidebarTabFolders } from "../src/renderer/surfaces/sidebar/model/sidebarTabFolders";

describe("sidebar tab folders", () => {
  it("keeps tab-backed Favorites in the Favorites folder only", () => {
    const group = createTabGroup("Research");
    const favoriteTab = { ...createTab("Docs", "https://docs.example"), isFavorite: true };
    const pinnedFavoriteTab = { ...createTab("Mail", "https://mail.example"), isPinned: true, isFavorite: true };
    const groupedFavoriteTab = { ...createTab("Issue", "https://issue.example"), groupId: group.id, isFavorite: true };
    const groupedTab = { ...createTab("Chromium", "https://chromium.example"), groupId: group.id };
    const pinnedTab = { ...createTab("Calendar", "https://calendar.example"), isPinned: true };
    const regularTab = createTab("News", "https://news.example");
    const workspace = createWorkspace({
      favoriteOrder: [favoriteTab.id, pinnedFavoriteTab.id, groupedFavoriteTab.id],
      tabGroups: [group],
      tabs: [favoriteTab, pinnedFavoriteTab, groupedFavoriteTab, groupedTab, pinnedTab, regularTab]
    });

    const folders = getSidebarTabFolders(workspace);

    expect(folders.regularTabs.map((tab) => tab.title)).toEqual(["Calendar", "News"]);
    expect(folders.regularTabs[0].isPinned).toBe(true);
  });

  it("keeps tabs without isFavorite flag out of the favorites folder", () => {
    const tab = createTab("Docs", "https://docs.example");
    const workspace = createWorkspace({
      favoriteOrder: [],
      tabs: [tab]
    });

    expect(getSidebarTabFolders(workspace).regularTabs).toEqual([tab]);
  });

  it("keeps isFavorite tabs not in favoriteOrder out of the favorites folder", () => {
    const tab = { ...createTab("Docs", "https://docs.example"), isFavorite: true };
    const workspace = createWorkspace({
      favoriteOrder: [],
      tabs: [tab]
    });

    expect(getSidebarTabFolders(workspace).regularTabs).toEqual([tab]);
  });
});

function createWorkspace(patch: Partial<Workspace>): Workspace {
  return {
    accent: "#7dd3fc",
    activeTabId: patch.tabs?.[0]?.id ?? null,
    activeAncillaryTabId: null,
    ancillaryTabIds: [],
    closedTabs: [],
    favoriteOrder: [],
    homepage: "https://start.example",
    id: "workspace",
    name: "Workspace",
    profileId: "profile",
    profileName: "Profile",
    splitLayout: "horizontal" as SplitLayout,
    splitMode: false,
    splitSide: "right",
    splitTabs: [],
    activeSplitId: null,
    tabGroups: [],
    tabs: [],
    ...patch
  };
}
