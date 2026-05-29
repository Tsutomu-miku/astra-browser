import { describe, expect, it } from "vitest";

import {
  addTab,
  addTabToFavorites,
  clearBrowsingData,
  clearHistory,
  clearSitePermissionRulesForOrigin,
  clearWorkspaceBrowsingData,
  closeActiveTab,
  closeOtherTabs,
  closeTabGroup,
  closeTabsToLeft,
  closeTabsToRight,
  closeTab,
  deleteWorkspace,
  duplicateActiveTab,
  duplicateTab,
  fillSplitView,
  focusSplitPane,
  assignTabToGroup,
  groupActiveTab,
  moveWorkspaceFavoriteToNewWorkspace,
  moveWorkspaceFavoriteToWorkspace,
  moveTabToNewWorkspace,
  moveTabToWorkspace,
  moveTabToFolderEnd,
  moveTabToFolderPosition,
  openTabInSplit,
  openUrlInSplit,
  openUrlInActiveWorkspace,
  recordHistory,
  removeTabFromSplit,
  removeHistoryEntry,
  reorderWorkspace,
  reorderTab,
  resetActiveTabZoom,
  restoreClosedTab,
  restoreClosedTabToNewWorkspace,
  restoreClosedTabToWorkspace,
  restoreLastClosedTab,
  selectAdjacentTab,
  selectTab,
  setActiveTabZoom,
  sleepIdleTabs,
  sleepInactiveTabs,
  sleepTabGroup,
  sleepTab,
  stepActiveTabZoom,
  switchWorkspace,
  toggleActiveTabEssential,
  toggleActiveTabFavorite,
  removeEssential,
  removeWorkspaceFavorite,
  reorderEssential,
  reorderWorkspaceFavorite,
  toggleActiveTabMuted,
  toggleActiveTabPinned,
  toggleTabEssential,
  toggleTabFavorite,
  toggleTabMuted,
  toggleTabPinned,
  toggleSplitMode
} from "../src/renderer/domain/actions";
import { createDefaultState, createFavorite } from "../src/renderer/domain/browser";
import { getActiveTab, getActiveWorkspace } from "../src/renderer/domain/browser/selectors";

describe("domain actions", () => {
  it("adds, pins, favorites, and closes tabs immutably", () => {
    const initial = createDefaultState();
    const withTab = addTab(initial);
    const withPin = toggleActiveTabPinned(withTab);
    const withFavorite = toggleActiveTabFavorite(withPin);
    const afterClose = closeActiveTab(withFavorite);

    expect(getActiveWorkspace(withTab).tabs).toHaveLength(2);
    expect(getActiveTab(getActiveWorkspace(withPin)).isPinned).toBe(true);
    expect(getActiveWorkspace(withFavorite).favorites.at(-1)?.url).toBe(getActiveTab(getActiveWorkspace(withFavorite)).url);
    expect(getActiveWorkspace(afterClose).tabs).toHaveLength(1);
    expect(getActiveWorkspace(afterClose).closedTabs[0].url).toBe(getActiveTab(getActiveWorkspace(withFavorite)).url);
    expect(initial.workspaces[0].tabs).toHaveLength(1);
  });

  it("opens new and replacement tabs at the active workspace homepage", () => {
    const base = createDefaultState();
    const initial = {
      ...base,
      workspaces: base.workspaces.map((workspace) => workspace.id === "personal"
        ? { ...workspace, homepage: "https://space.example/" }
        : workspace)
    };
    const withTab = addTab(initial);
    const closed = closeActiveTab(withTab);
    const closedAgain = closeActiveTab(closed);

    expect(getActiveTab(getActiveWorkspace(withTab)).url).toBe("https://space.example/");
    expect(getActiveTab(getActiveWorkspace(closedAgain)).url).toBe("https://space.example/");
  });

  it("restores the most recently closed tab", () => {
    const opened = openUrlInActiveWorkspace(createDefaultState(), "example.com", "Example");
    const closed = closeActiveTab(opened);
    const restored = restoreLastClosedTab(closed);
    const workspace = getActiveWorkspace(restored);

    expect(getActiveTab(workspace).url).toBe("https://example.com/");
    expect(workspace.closedTabs).toHaveLength(0);
  });

  it("restores a selected recently closed tab", () => {
    const first = openUrlInActiveWorkspace(createDefaultState(), "first.test", "First");
    const second = openUrlInActiveWorkspace(first, "second.test", "Second");
    const closedSecond = closeActiveTab(second);
    const closedBoth = closeActiveTab(closedSecond);
    const restored = restoreClosedTab(closedBoth, 1);
    const workspace = getActiveWorkspace(restored);

    expect(getActiveTab(workspace).title).toBe("Second");
    expect(workspace.closedTabs.map((tab) => tab.title)).toEqual(["First"]);
  });

  it("restores a recently closed tab into a target workspace", () => {
    const opened = openUrlInActiveWorkspace(createDefaultState(), "example.com", "Example");
    const closed = closeActiveTab(opened);
    const restored = restoreClosedTabToWorkspace(closed, 0, "work");
    const personal = restored.workspaces.find((workspace) => workspace.id === "personal")!;
    const work = restored.workspaces.find((workspace) => workspace.id === "work")!;

    expect(restored.activeWorkspaceId).toBe("work");
    expect(personal.closedTabs).toHaveLength(0);
    expect(work.tabs.at(-1)?.title).toBe("Example");
    expect(work.activeTabId).toBe(work.tabs.at(-1)?.id);
  });

  it("restores a recently closed tab into a new workspace", () => {
    const opened = openUrlInActiveWorkspace(createDefaultState(), "example.com", "Example");
    const closed = closeActiveTab(opened);
    const restored = restoreClosedTabToNewWorkspace(closed, 0);
    const workspace = getActiveWorkspace(restored);

    expect(restored.workspaces).toHaveLength(closed.workspaces.length + 1);
    expect(workspace.name).toBe("Example");
    expect(getActiveTab(workspace).url).toBe("https://example.com/");
    expect(restored.workspaces.find((candidate) => candidate.id === "personal")?.closedTabs).toHaveLength(0);
  });

  it("ignores invalid recently closed indexes", () => {
    const opened = openUrlInActiveWorkspace(createDefaultState(), "example.com", "Example");
    const closed = closeActiveTab(opened);
    const ignored = restoreClosedTab(closed, 5);
    const workspace = getActiveWorkspace(ignored);

    expect(getActiveTab(workspace).title).toBe(getActiveTab(getActiveWorkspace(closed)).title);
    expect(workspace.closedTabs.map((tab) => tab.url)).toEqual(getActiveWorkspace(closed).closedTabs.map((tab) => tab.url));
  });

  it("closes a background tab without changing the active tab", () => {
    const firstOpen = openUrlInActiveWorkspace(createDefaultState(), "first.test", "First");
    const secondOpen = openUrlInActiveWorkspace(firstOpen, "second.test", "Second");
    const workspace = getActiveWorkspace(secondOpen);
    const backgroundTab = workspace.tabs.at(-2)!;
    const activeTab = getActiveTab(workspace);
    const closed = closeTab(secondOpen, backgroundTab.id);

    expect(getActiveTab(getActiveWorkspace(closed)).id).toBe(activeTab.id);
    expect(getActiveWorkspace(closed).tabs.some((tab) => tab.id === backgroundTab.id)).toBe(false);
    expect(getActiveWorkspace(closed).closedTabs[0].url).toBe(backgroundTab.url);
  });

  it("moves a tab into a new workspace from sidebar organization", () => {
    const opened = openUrlInActiveWorkspace(createDefaultState(), "docs.example", "Docs");
    const tab = getActiveTab(getActiveWorkspace(opened));
    const moved = moveTabToNewWorkspace(opened, tab.id);
    const source = moved.workspaces.find((workspace) => workspace.id === "personal")!;
    const target = getActiveWorkspace(moved);

    expect(moved.workspaces).toHaveLength(opened.workspaces.length + 1);
    expect(target.name).toBe("Docs");
    expect(target.tabs.map((candidate) => candidate.id)).toEqual([tab.id]);
    expect(target.activeTabId).toBe(tab.id);
    expect(source.tabs.some((candidate) => candidate.id === tab.id)).toBe(false);
    expect(source.activeTabId).toBe(source.tabs[0].id);
    expect(moved.splitMode).toBe(false);
  });

  it("closes other tabs and stores them as recently closed", () => {
    const first = openUrlInActiveWorkspace(createDefaultState(), "first.test", "First");
    const second = openUrlInActiveWorkspace(first, "second.test", "Second");
    const closed = closeOtherTabs(second);
    const workspace = getActiveWorkspace(closed);

    expect(workspace.tabs).toHaveLength(1);
    expect(getActiveTab(workspace).title).toBe("Second");
    expect(workspace.closedTabs.map((tab) => tab.title)).toContain("First");
    expect(closed.splitMode).toBe(false);
    expect(closed.splitTabId).toBeNull();
    expect(closed.splitTabIds).toEqual([]);
  });

  it("closes other tabs around a background target tab", () => {
    const first = openUrlInActiveWorkspace(createDefaultState(), "first.test", "First");
    const second = openUrlInActiveWorkspace(first, "second.test", "Second");
    const third = openUrlInActiveWorkspace(second, "third.test", "Third");
    const workspace = getActiveWorkspace(third);
    const firstTab = workspace.tabs.find((tab) => tab.title === "First")!;
    const closed = closeOtherTabs(third, firstTab.id);
    const nextWorkspace = getActiveWorkspace(closed);

    expect(nextWorkspace.tabs.map((tab) => tab.title)).toEqual(["First"]);
    expect(getActiveTab(nextWorkspace).title).toBe("First");
    expect(nextWorkspace.closedTabs.map((tab) => tab.title)).toEqual(["Third", "Second", "New Tab"]);
  });

  it("closes tabs to the right and keeps left tabs", () => {
    const first = openUrlInActiveWorkspace(createDefaultState(), "first.test", "First");
    const second = openUrlInActiveWorkspace(first, "second.test", "Second");
    const third = openUrlInActiveWorkspace(second, "third.test", "Third");
    const workspace = getActiveWorkspace(third);
    const secondTab = workspace.tabs.find((tab) => tab.title === "Second")!;
    const selected = switchWorkspace(third, workspace.id);
    const closed = closeTabsToRight({
      ...selected,
      workspaces: selected.workspaces.map((candidate) => candidate.id === workspace.id
        ? { ...candidate, activeTabId: secondTab.id }
        : candidate)
    });
    const titles = getActiveWorkspace(closed).tabs.map((tab) => tab.title);

    expect(titles).toEqual(["New Tab", "First", "Second"]);
    expect(getActiveTab(getActiveWorkspace(closed)).title).toBe("Second");
    expect(getActiveWorkspace(closed).closedTabs[0].title).toBe("Third");
  });

  it("closes tabs to the right of a background target tab", () => {
    const first = openUrlInActiveWorkspace(createDefaultState(), "first.test", "First");
    const second = openUrlInActiveWorkspace(first, "second.test", "Second");
    const third = openUrlInActiveWorkspace(second, "third.test", "Third");
    const workspace = getActiveWorkspace(third);
    const firstTab = workspace.tabs.find((tab) => tab.title === "First")!;
    const closed = closeTabsToRight(third, firstTab.id);
    const nextWorkspace = getActiveWorkspace(closed);

    expect(nextWorkspace.tabs.map((tab) => tab.title)).toEqual(["New Tab", "First"]);
    expect(getActiveTab(nextWorkspace).title).toBe("First");
    expect(nextWorkspace.closedTabs.map((tab) => tab.title).slice(0, 2)).toEqual(["Third", "Second"]);
  });

  it("closes tabs to the left and keeps right tabs", () => {
    const first = openUrlInActiveWorkspace(createDefaultState(), "first.test", "First");
    const second = openUrlInActiveWorkspace(first, "second.test", "Second");
    const third = openUrlInActiveWorkspace(second, "third.test", "Third");
    const workspace = getActiveWorkspace(third);
    const secondTab = workspace.tabs.find((tab) => tab.title === "Second")!;
    const selected = switchWorkspace(third, workspace.id);
    const closed = closeTabsToLeft({
      ...selected,
      workspaces: selected.workspaces.map((candidate) => candidate.id === workspace.id
        ? { ...candidate, activeTabId: secondTab.id }
        : candidate)
    });
    const titles = getActiveWorkspace(closed).tabs.map((tab) => tab.title);

    expect(titles).toEqual(["Second", "Third"]);
    expect(getActiveTab(getActiveWorkspace(closed)).title).toBe("Second");
    expect(getActiveWorkspace(closed).closedTabs[0].title).toBe("First");
  });

  it("closes tabs to the left of a background target tab", () => {
    const first = openUrlInActiveWorkspace(createDefaultState(), "first.test", "First");
    const second = openUrlInActiveWorkspace(first, "second.test", "Second");
    const third = openUrlInActiveWorkspace(second, "third.test", "Third");
    const workspace = getActiveWorkspace(third);
    const secondTab = workspace.tabs.find((tab) => tab.title === "Second")!;
    const closed = closeTabsToLeft(third, secondTab.id);
    const nextWorkspace = getActiveWorkspace(closed);

    expect(nextWorkspace.tabs.map((tab) => tab.title)).toEqual(["Second", "Third"]);
    expect(getActiveTab(nextWorkspace).title).toBe("Second");
    expect(nextWorkspace.closedTabs.map((tab) => tab.title).slice(0, 2)).toEqual(["First", "New Tab"]);
  });

  it("closes a tab group into recently closed and keeps the nearest tab active", () => {
    const grouped = groupActiveTab(openUrlInActiveWorkspace(createDefaultState(), "docs.test", "Docs"));
    const group = getActiveWorkspace(grouped).tabGroups[0];
    const withNews = openUrlInActiveWorkspace(grouped, "news.test", "News");
    const newsTab = getActiveTab(getActiveWorkspace(withNews));
    const assigned = assignTabToGroup(withNews, newsTab.id, group.id);
    const withThird = openUrlInActiveWorkspace(assigned, "third.test", "Third");
    const selectedGroupTab = selectTab(withThird, newsTab.id);
    const closed = closeTabGroup(selectedGroupTab, group.id);
    const workspace = getActiveWorkspace(closed);

    expect(workspace.tabGroups).toHaveLength(0);
    expect(workspace.tabs.map((tab) => tab.title)).toEqual(["New Tab", "Third"]);
    expect(getActiveTab(workspace).title).toBe("Third");
    expect(workspace.closedTabs.map((tab) => tab.title).slice(0, 2)).toEqual(["News", "Docs"]);
  });

  it("replaces the last open tab when closing its tab group", () => {
    const grouped = groupActiveTab(openUrlInActiveWorkspace(createDefaultState(), "docs.test", "Docs"));
    const group = getActiveWorkspace(grouped).tabGroups[0];
    const defaultTab = getActiveWorkspace(grouped).tabs.find((tab) => tab.title === "New Tab")!;
    const onlyGroup = closeTab(grouped, defaultTab.id);
    const closed = closeTabGroup(onlyGroup, group.id);
    const workspace = getActiveWorkspace(closed);

    expect(workspace.tabs).toHaveLength(1);
    expect(getActiveTab(workspace).title).toBe("New Tab");
    expect(workspace.tabGroups).toHaveLength(0);
    expect(workspace.closedTabs[0].title).toBe("Docs");
    expect(closed.splitMode).toBe(false);
  });

  it("duplicates the active tab next to the original", () => {
    const opened = openUrlInActiveWorkspace(createDefaultState(), "example.com", "Example");
    const active = getActiveTab(getActiveWorkspace(opened));
    const duplicated = duplicateActiveTab(opened);
    const workspace = getActiveWorkspace(duplicated);
    const copy = getActiveTab(workspace);

    expect(workspace.tabs).toHaveLength(getActiveWorkspace(opened).tabs.length + 1);
    expect(copy.id).not.toBe(active.id);
    expect(copy.url).toBe(active.url);
    expect(copy.title).toBe(active.title);
    expect(workspace.tabs.findIndex((tab) => tab.id === copy.id)).toBe(
      workspace.tabs.findIndex((tab) => tab.id === active.id) + 1
    );
  });

  it("duplicates a chosen background tab and activates the copy", () => {
    const first = openUrlInActiveWorkspace(createDefaultState(), "first.test", "First");
    const second = openUrlInActiveWorkspace(first, "second.test", "Second");
    const workspace = getActiveWorkspace(second);
    const original = workspace.tabs.find((tab) => tab.title === "First")!;
    const duplicated = duplicateTab(second, original.id);
    const duplicatedWorkspace = getActiveWorkspace(duplicated);
    const copy = getActiveTab(duplicatedWorkspace);

    expect(copy.id).not.toBe(original.id);
    expect(copy.title).toBe("First");
    expect(copy.url).toBe(original.url);
    expect(copy.canGoBack).toBe(false);
    expect(copy.canGoForward).toBe(false);
    expect(duplicatedWorkspace.tabs.findIndex((tab) => tab.id === copy.id)).toBe(
      duplicatedWorkspace.tabs.findIndex((tab) => tab.id === original.id) + 1
    );
  });

  it("toggles pinned and muted state for a chosen background tab", () => {
    const grouped = groupActiveTab(openUrlInActiveWorkspace(createDefaultState(), "docs.example", "Docs"));
    const withActiveNews = openUrlInActiveWorkspace(grouped, "news.example", "News");
    const docsTab = getActiveWorkspace(withActiveNews).tabs.find((tab) => tab.title === "Docs")!;
    const pinned = toggleTabPinned(withActiveNews, docsTab.id);
    const muted = toggleTabMuted(pinned, docsTab.id);
    const workspace = getActiveWorkspace(muted);
    const updatedDocs = workspace.tabs.find((tab) => tab.id === docsTab.id)!;

    expect(getActiveTab(workspace).title).toBe("News");
    expect(updatedDocs.isPinned).toBe(true);
    expect(updatedDocs.groupId).toBeNull();
    expect(updatedDocs.isMuted).toBe(true);
    expect(workspace.tabGroups).toHaveLength(0);
  });

  it("toggles favorites and essentials for a chosen background tab", () => {
    const first = openUrlInActiveWorkspace(createDefaultState(), "docs.example", "Docs");
    const second = openUrlInActiveWorkspace(first, "news.example", "News");
    const workspace = getActiveWorkspace(second);
    const docsTab = workspace.tabs.find((tab) => tab.title === "Docs")!;
    const addedFavorite = toggleTabFavorite(second, docsTab.id);
    const addedEssential = toggleTabEssential(addedFavorite, docsTab.id);
    const removedFavorite = toggleTabFavorite(addedEssential, docsTab.id);
    const removedEssential = toggleTabEssential(removedFavorite, docsTab.id);

    expect(getActiveTab(getActiveWorkspace(addedEssential)).title).toBe("News");
    expect(getActiveWorkspace(addedFavorite).favorites.at(-1)).toMatchObject({
      tabId: docsTab.id,
      title: "Docs",
      url: docsTab.url
    });
    expect(addedEssential.essentials.at(-1)).toMatchObject({
      title: "Docs",
      url: docsTab.url
    });
    expect(getActiveWorkspace(removedFavorite).favorites.some((favorite) => favorite.url === docsTab.url)).toBe(false);
    expect(removedEssential.essentials.some((essential) => essential.url === docsTab.url)).toBe(false);
  });

  it("adds tab-backed Favorites by tab identity instead of URL equality", () => {
    const first = openUrlInActiveWorkspace(createDefaultState(), "docs.example", "Docs");
    const second = openUrlInActiveWorkspace(first, "docs.example", "Docs duplicate");
    const workspace = getActiveWorkspace(second);
    const docsTab = workspace.tabs.find((tab) => tab.title === "Docs")!;
    const duplicateTab = workspace.tabs.find((tab) => tab.title === "Docs duplicate")!;
    const withDocsFavorite = addTabToFavorites(second, docsTab.id);
    const withDuplicateFavorite = addTabToFavorites(withDocsFavorite, duplicateTab.id);
    const favorites = getActiveWorkspace(withDuplicateFavorite).favorites.filter((favorite) => favorite.url === docsTab.url);

    expect(favorites.map((favorite) => favorite.tabId)).toContain(docsTab.id);
    expect(favorites.map((favorite) => favorite.tabId)).toContain(duplicateTab.id);
  });

  it("toggles Favorites by tab identity before falling back to legacy URL entries", () => {
    const first = openUrlInActiveWorkspace(createDefaultState(), "docs.example", "Docs");
    const second = openUrlInActiveWorkspace(first, "docs.example", "Docs duplicate");
    const workspace = getActiveWorkspace(second);
    const docsTab = workspace.tabs.find((tab) => tab.title === "Docs")!;
    const duplicateTab = workspace.tabs.find((tab) => tab.title === "Docs duplicate")!;
    const withDocsFavorite = addTabToFavorites(second, docsTab.id);
    const withDuplicateFavorite = toggleTabFavorite(withDocsFavorite, duplicateTab.id);
    const withoutDuplicateFavorite = toggleTabFavorite(withDuplicateFavorite, duplicateTab.id);
    const remainingFavorites = getActiveWorkspace(withoutDuplicateFavorite).favorites.filter((favorite) => favorite.url === docsTab.url);

    expect(getActiveWorkspace(withDuplicateFavorite).favorites.map((favorite) => favorite.tabId)).toContain(duplicateTab.id);
    expect(remainingFavorites).toHaveLength(1);
    expect(remainingFavorites[0].tabId).toBe(docsTab.id);
  });

  it("upgrades legacy URL Favorites when a matching tab is added to Favorites", () => {
    const withDocs = openUrlInActiveWorkspace(createDefaultState(), "docs.example", "Docs");
    const docsTab = getActiveTab(getActiveWorkspace(withDocs));
    const legacyFavorite = createFavorite("Legacy docs", docsTab.url);
    const legacyState = {
      ...withDocs,
      workspaces: withDocs.workspaces.map((workspace) => workspace.id === withDocs.activeWorkspaceId
        ? { ...workspace, favorites: [legacyFavorite] }
        : workspace)
    };
    const upgraded = addTabToFavorites(legacyState, docsTab.id);
    const favorites = getActiveWorkspace(upgraded).favorites;

    expect(favorites).toHaveLength(1);
    expect(favorites[0]).toMatchObject({
      tabId: docsTab.id,
      title: "Docs",
      url: docsTab.url
    });
  });

  it("moves tabs into Favorites as their only sidebar folder", () => {
    const grouped = groupActiveTab(openUrlInActiveWorkspace(createDefaultState(), "docs.example", "Docs"));
    const docsTab = getActiveTab(getActiveWorkspace(grouped));
    const pinned = toggleTabPinned(grouped, docsTab.id);
    const favorited = addTabToFavorites(pinned, docsTab.id);
    const workspace = getActiveWorkspace(favorited);
    const tab = workspace.tabs.find((candidate) => candidate.id === docsTab.id)!;

    expect(tab.isPinned).toBe(false);
    expect(tab.groupId).toBeNull();
    expect(workspace.tabGroups).toHaveLength(0);
    expect(workspace.favorites.find((favorite) => favorite.tabId === docsTab.id)).toMatchObject({
      tabId: docsTab.id,
      title: "Docs",
      url: docsTab.url
    });
  });

  it("removes quick entries by url without requiring a matching tab", () => {
    const withFavorite = toggleActiveTabFavorite(openUrlInActiveWorkspace(createDefaultState(), "docs.example", "Docs"));
    const withEssential = toggleActiveTabEssential(withFavorite);
    const url = getActiveTab(getActiveWorkspace(withEssential)).url;
    const withoutFavorite = removeWorkspaceFavorite(withEssential, url);
    const withoutEssential = removeEssential(withoutFavorite, url);

    expect(getActiveWorkspace(withoutFavorite).favorites.some((favorite) => favorite.url === url)).toBe(false);
    expect(withoutEssential.essentials.some((essential) => essential.url === url)).toBe(false);
  });

  it("removes a single Favorite by id when duplicate Favorite URLs exist", () => {
    const first = openUrlInActiveWorkspace(createDefaultState(), "docs.example", "Docs");
    const second = openUrlInActiveWorkspace(first, "docs.example", "Docs duplicate");
    const workspace = getActiveWorkspace(second);
    const docsTab = workspace.tabs.find((tab) => tab.title === "Docs")!;
    const duplicateTab = workspace.tabs.find((tab) => tab.title === "Docs duplicate")!;
    const withFavorites = addTabToFavorites(addTabToFavorites(second, docsTab.id), duplicateTab.id);
    const duplicateFavorite = getActiveWorkspace(withFavorites).favorites.find((favorite) => favorite.tabId === duplicateTab.id)!;
    const withoutDuplicate = removeWorkspaceFavorite(withFavorites, duplicateFavorite.id);
    const remainingFavorites = getActiveWorkspace(withoutDuplicate).favorites.filter((favorite) => favorite.url === docsTab.url);

    expect(remainingFavorites).toHaveLength(1);
    expect(remainingFavorites[0].tabId).toBe(docsTab.id);
  });

  it("removes tab-backed Favorites when their tab is closed", () => {
    const first = openUrlInActiveWorkspace(createDefaultState(), "docs.example", "Docs");
    const second = openUrlInActiveWorkspace(first, "news.example", "News");
    const workspace = getActiveWorkspace(second);
    const docsTab = workspace.tabs.find((tab) => tab.title === "Docs")!;
    const favorited = addTabToFavorites(second, docsTab.id);
    const closed = closeTab(favorited, docsTab.id);
    const closedWorkspace = getActiveWorkspace(closed);

    expect(closedWorkspace.closedTabs[0].url).toBe(docsTab.url);
    expect(closedWorkspace.favorites.some((favorite) => favorite.tabId === docsTab.id)).toBe(false);
    expect(closedWorkspace.favorites.some((favorite) => favorite.url === docsTab.url)).toBe(false);
  });

  it("keeps legacy URL Favorites when a matching tab is closed", () => {
    const opened = openUrlInActiveWorkspace(createDefaultState(), "docs.example", "Docs");
    const docsTab = getActiveTab(getActiveWorkspace(opened));
    const legacyFavorite = createFavorite("Legacy docs", docsTab.url);
    const legacyState = {
      ...opened,
      workspaces: opened.workspaces.map((workspace) => workspace.id === opened.activeWorkspaceId
        ? { ...workspace, favorites: [legacyFavorite] }
        : workspace)
    };
    const closed = closeTab(legacyState, docsTab.id);

    expect(getActiveWorkspace(closed).favorites).toEqual([legacyFavorite]);
  });

  it("reorders tabs while preserving the active tab", () => {
    const withFirst = openUrlInActiveWorkspace(createDefaultState(), "first.test", "First");
    const withSecond = openUrlInActiveWorkspace(withFirst, "second.test", "Second");
    const withThird = openUrlInActiveWorkspace(withSecond, "third.test", "Third");
    const workspace = getActiveWorkspace(withThird);
    const first = workspace.tabs.find((tab) => tab.title === "First")!;
    const third = workspace.tabs.find((tab) => tab.title === "Third")!;
    const reordered = reorderTab(withThird, first.id, third.id, "after");
    const titles = getActiveWorkspace(reordered).tabs.map((tab) => tab.title);

    expect(titles.slice(-2)).toEqual(["Third", "First"]);
    expect(getActiveTab(getActiveWorkspace(reordered)).title).toBe("Third");
  });

  it("moves dragged tabs into the target tab folder", () => {
    const withFirst = openUrlInActiveWorkspace(createDefaultState(), "first.test", "First");
    const withSecond = openUrlInActiveWorkspace(withFirst, "second.test", "Second");
    const workspace = getActiveWorkspace(withSecond);
    const first = workspace.tabs.find((tab) => tab.title === "First")!;
    const second = workspace.tabs.find((tab) => tab.title === "Second")!;
    const pinned = toggleTabPinned(withSecond, first.id);
    const unpinned = moveTabToFolderPosition(pinned, first.id, second.id, "after");
    const tabs = getActiveWorkspace(unpinned).tabs;
    const moved = tabs.find((tab) => tab.id === first.id)!;

    expect(moved.isPinned).toBe(false);
    expect(moved.groupId).toBeNull();
    expect(tabs.map((tab) => tab.title).slice(-2)).toEqual(["Second", "First"]);
  });

  it("pins dragged regular tabs when placing them next to pinned tabs", () => {
    const withFirst = openUrlInActiveWorkspace(createDefaultState(), "first.test", "First");
    const withSecond = openUrlInActiveWorkspace(withFirst, "second.test", "Second");
    const workspace = getActiveWorkspace(withSecond);
    const first = workspace.tabs.find((tab) => tab.title === "First")!;
    const second = workspace.tabs.find((tab) => tab.title === "Second")!;
    const withPinnedTarget = toggleTabPinned(withSecond, first.id);
    const pinned = moveTabToFolderPosition(withPinnedTarget, second.id, first.id, "before");
    const tabs = getActiveWorkspace(pinned).tabs;
    const moved = tabs.find((tab) => tab.id === second.id)!;

    expect(moved.isPinned).toBe(true);
    expect(moved.groupId).toBeNull();
    expect(tabs.map((tab) => tab.title).slice(-2)).toEqual(["Second", "First"]);
  });

  it("moves dragged tabs to the regular folder end", () => {
    const withFirst = openUrlInActiveWorkspace(createDefaultState(), "first.test", "First");
    const withSecond = openUrlInActiveWorkspace(withFirst, "second.test", "Second");
    const workspace = getActiveWorkspace(withSecond);
    const first = workspace.tabs.find((tab) => tab.title === "First")!;
    const pinned = toggleTabPinned(withSecond, first.id);
    const unpinned = moveTabToFolderEnd(pinned, first.id, { type: "tabs" });
    const tabs = getActiveWorkspace(unpinned).tabs;
    const moved = tabs.find((tab) => tab.id === first.id)!;

    expect(moved.isPinned).toBe(false);
    expect(moved.groupId).toBeNull();
    expect(tabs.at(-1)?.id).toBe(first.id);
  });

  it("moves dragged tabs into the Favorites folder through tab folder actions", () => {
    const withFirst = openUrlInActiveWorkspace(createDefaultState(), "first.test", "First");
    const workspace = getActiveWorkspace(withFirst);
    const first = getActiveTab(workspace);
    const pinned = toggleTabPinned(withFirst, first.id);
    const favorited = moveTabToFolderEnd(pinned, first.id, { type: "favorites" });
    const moved = getActiveWorkspace(favorited).tabs.find((tab) => tab.id === first.id)!;
    const favorite = getActiveWorkspace(favorited).favorites.find((candidate) => candidate.tabId === first.id);

    expect(moved.isPinned).toBe(false);
    expect(moved.groupId).toBeNull();
    expect(favorite).toMatchObject({
      tabId: first.id,
      title: "First",
      url: "https://first.test/"
    });
  });

  it("moves Favorite-backed tabs out of Favorites through tab folder actions", () => {
    const withFirst = openUrlInActiveWorkspace(createDefaultState(), "first.test", "First");
    const workspace = getActiveWorkspace(withFirst);
    const first = getActiveTab(workspace);
    const favorited = moveTabToFolderEnd(withFirst, first.id, { type: "favorites" });
    const unpinned = moveTabToFolderEnd(favorited, first.id, { type: "tabs" });

    expect(getActiveWorkspace(unpinned).favorites.some((favorite) => favorite.tabId === first.id)).toBe(false);
    expect(getActiveWorkspace(unpinned).tabs.at(-1)?.id).toBe(first.id);
  });

  it("moves dragged tabs into grouped folders", () => {
    const withNews = openUrlInActiveWorkspace(groupActiveTab(createDefaultState()), "news.example", "News");
    const workspace = getActiveWorkspace(withNews);
    const group = workspace.tabGroups[0];
    const groupedTarget = workspace.tabs.find((tab) => tab.groupId === group.id)!;
    const newsTab = workspace.tabs.find((tab) => tab.title === "News")!;
    const grouped = moveTabToFolderPosition(withNews, newsTab.id, groupedTarget.id, "after");
    const moved = getActiveWorkspace(grouped).tabs.find((tab) => tab.id === newsTab.id)!;

    expect(moved.isPinned).toBe(false);
    expect(moved.groupId).toBe(group.id);
  });

  it("reorders workspaces while preserving the active workspace", () => {
    const initial = createDefaultState();
    const reordered = reorderWorkspace(initial, "work", "personal", "before");

    expect(reordered.workspaces.map((workspace) => workspace.id)).toEqual(["work", "personal"]);
    expect(reordered.activeWorkspaceId).toBe(initial.activeWorkspaceId);
  });

  it("reorders active workspace favorites", () => {
    const initial = createDefaultState();
    const workspace = getActiveWorkspace(initial);
    const first = createFavorite("First", "https://first.example");
    const second = createFavorite("Second", "https://second.example");
    const third = createFavorite("Third", "https://third.example");
    workspace.favorites = [first, second, third];

    const movedBefore = reorderWorkspaceFavorite(initial, third.id, first.id, "before");
    const movedAfter = reorderWorkspaceFavorite(movedBefore, first.id, second.id, "after");
    const ignored = reorderWorkspaceFavorite(movedAfter, "missing", second.id, "before");

    expect(getActiveWorkspace(movedBefore).favorites.map((favorite) => favorite.title)).toEqual(["Third", "First", "Second"]);
    expect(getActiveWorkspace(movedAfter).favorites.map((favorite) => favorite.title)).toEqual(["Third", "Second", "First"]);
    expect(getActiveWorkspace(ignored).favorites.map((favorite) => favorite.title)).toEqual(["Third", "Second", "First"]);
  });

  it("moves a Space favorite to another workspace", () => {
    const initial = openUrlInActiveWorkspace(createDefaultState(), "docs.example", "Docs");
    const workspace = getActiveWorkspace(initial);
    const docsTab = getActiveTab(workspace);
    const favorite = createFavorite("Docs", docsTab.url, docsTab.id);
    workspace.favorites = [favorite];

    const moved = moveWorkspaceFavoriteToWorkspace(initial, favorite.id, "work");
    const personal = moved.workspaces.find((candidate) => candidate.id === "personal")!;
    const work = moved.workspaces.find((candidate) => candidate.id === "work")!;

    expect(moved.activeWorkspaceId).toBe("work");
    expect(personal.favorites).toHaveLength(0);
    expect(personal.tabs.some((tab) => tab.id === docsTab.id)).toBe(false);
    expect(work.tabs.some((tab) => tab.id === docsTab.id)).toBe(true);
    expect(work.activeTabId).toBe(docsTab.id);
    expect(work.favorites.at(-1)).toMatchObject({
      tabId: docsTab.id,
      title: "Docs",
      url: "https://docs.example/"
    });
  });

  it("preserves the Favorites folder when a Favorite tab moves to another workspace", () => {
    const initial = openUrlInActiveWorkspace(createDefaultState(), "docs.example", "Docs");
    const workspace = getActiveWorkspace(initial);
    const docsTab = getActiveTab(workspace);
    workspace.favorites = [createFavorite("Docs", docsTab.url, docsTab.id)];

    const moved = moveTabToWorkspace(initial, docsTab.id, "work");
    const personal = moved.workspaces.find((candidate) => candidate.id === "personal")!;
    const work = moved.workspaces.find((candidate) => candidate.id === "work")!;

    expect(personal.favorites).toHaveLength(0);
    expect(work.tabs.some((tab) => tab.id === docsTab.id)).toBe(true);
    expect(work.favorites).toContainEqual(expect.objectContaining({ tabId: docsTab.id, title: "Docs" }));
  });

  it("upgrades legacy URL Favorite folder membership when its matching tab moves to another workspace", () => {
    const initial = openUrlInActiveWorkspace(createDefaultState(), "docs.example", "Docs");
    const workspace = getActiveWorkspace(initial);
    const docsTab = getActiveTab(workspace);
    workspace.favorites = [createFavorite("Legacy Docs", docsTab.url)];

    const moved = moveTabToWorkspace(initial, docsTab.id, "work");
    const personal = moved.workspaces.find((candidate) => candidate.id === "personal")!;
    const work = moved.workspaces.find((candidate) => candidate.id === "work")!;

    expect(personal.favorites).toHaveLength(0);
    expect(work.favorites).toContainEqual(expect.objectContaining({
      tabId: docsTab.id,
      title: "Docs",
      url: "https://docs.example/"
    }));
  });

  it("preserves the Favorites folder when a Favorite tab creates a new workspace", () => {
    const initial = openUrlInActiveWorkspace(createDefaultState(), "docs.example", "Docs");
    const workspace = getActiveWorkspace(initial);
    const docsTab = getActiveTab(workspace);
    workspace.favorites = [createFavorite("Docs", docsTab.url, docsTab.id)];

    const moved = moveTabToNewWorkspace(initial, docsTab.id);
    const source = moved.workspaces.find((candidate) => candidate.id === "personal")!;
    const target = getActiveWorkspace(moved);

    expect(source.favorites).toHaveLength(0);
    expect(target.favorites).toEqual([expect.objectContaining({ tabId: docsTab.id, title: "Docs" })]);
    expect(target.tabs.map((tab) => tab.id)).toEqual([docsTab.id]);
  });

  it("creates a backing tab when moving a legacy Space favorite to another workspace", () => {
    const initial = createDefaultState();
    const workspace = getActiveWorkspace(initial);
    const favorite = createFavorite("Docs", "https://docs.example");
    workspace.favorites = [favorite];

    const moved = moveWorkspaceFavoriteToWorkspace(initial, favorite.id, "work");
    const work = moved.workspaces.find((candidate) => candidate.id === "work")!;
    const movedFavorite = work.favorites.at(-1)!;
    const createdTab = work.tabs.find((tab) => tab.id === movedFavorite.tabId)!;

    expect(createdTab).toMatchObject({
      title: "Docs",
      url: "https://docs.example/"
    });
    expect(work.activeTabId).toBe(createdTab.id);
  });

  it("moves a Space favorite to a new workspace", () => {
    const initial = openUrlInActiveWorkspace(createDefaultState(), "docs.example", "Docs");
    const workspace = getActiveWorkspace(initial);
    const docsTab = getActiveTab(workspace);
    const favorite = createFavorite("Docs", docsTab.url, docsTab.id);
    workspace.favorites = [favorite];

    const moved = moveWorkspaceFavoriteToNewWorkspace(initial, favorite.id);
    const source = moved.workspaces.find((candidate) => candidate.id === "personal")!;
    const target = getActiveWorkspace(moved);

    expect(moved.workspaces).toHaveLength(initial.workspaces.length + 1);
    expect(source.favorites).toHaveLength(0);
    expect(source.tabs.some((tab) => tab.id === docsTab.id)).toBe(false);
    expect(target.name).toBe("Docs");
    expect(target.favorites).toHaveLength(1);
    expect(target.favorites[0].tabId).toBe(docsTab.id);
    expect(target.tabs.map((tab) => tab.id)).toEqual([docsTab.id]);
    expect(getActiveTab(target).id).toBe(docsTab.id);
  });

  it("creates a backing tab when moving a legacy Space favorite to a new workspace", () => {
    const initial = createDefaultState();
    const workspace = getActiveWorkspace(initial);
    const favorite = createFavorite("Docs", "https://docs.example");
    workspace.favorites = [favorite];

    const moved = moveWorkspaceFavoriteToNewWorkspace(initial, favorite.id);
    const target = getActiveWorkspace(moved);

    expect(target.favorites[0].tabId).toBe(target.tabs[0].id);
    expect(target.tabs).toHaveLength(1);
    expect(getActiveTab(target).url).toBe("https://docs.example/");
  });

  it("reorders global essentials", () => {
    const initial = createDefaultState();
    const first = createFavorite("First", "https://first.example");
    const second = createFavorite("Second", "https://second.example");
    const third = createFavorite("Third", "https://third.example");
    initial.essentials = [first, second, third];

    const movedBefore = reorderEssential(initial, third.id, first.id, "before");
    const movedAfter = reorderEssential(movedBefore, first.id, second.id, "after");
    const ignored = reorderEssential(movedAfter, "missing", second.id, "before");

    expect(movedBefore.essentials.map((essential) => essential.title)).toEqual(["Third", "First", "Second"]);
    expect(movedAfter.essentials.map((essential) => essential.title)).toEqual(["Third", "Second", "First"]);
    expect(ignored.essentials.map((essential) => essential.title)).toEqual(["Third", "Second", "First"]);
  });

  it("deletes the active workspace and keeps at least one workspace", () => {
    const initial = createDefaultState();
    const deleted = deleteWorkspace(initial, "personal");
    const ignored = deleteWorkspace(deleted, "work");

    expect(deleted.workspaces.map((workspace) => workspace.id)).toEqual(["work"]);
    expect(deleted.activeWorkspaceId).toBe("work");
    expect(deleted.splitMode).toBe(false);
    expect(ignored.workspaces).toHaveLength(1);
  });

  it("cycles through adjacent tabs", () => {
    const first = openUrlInActiveWorkspace(createDefaultState(), "first.test", "First");
    const second = openUrlInActiveWorkspace(first, "second.test", "Second");
    const previous = selectAdjacentTab(second, -1);
    const next = selectAdjacentTab(previous, 1);

    expect(getActiveTab(getActiveWorkspace(previous)).title).toBe("First");
    expect(getActiveTab(getActiveWorkspace(next)).title).toBe("Second");
  });

  it("switches workspaces and creates split targets", () => {
    const initial = createDefaultState();
    const switched = switchWorkspace(initial, "work");
    const split = toggleSplitMode(switched);

    expect(switched.activeWorkspaceId).toBe("work");
    expect(split.splitMode).toBe(true);
    expect(split.splitTabId).toBeTruthy();
  });

  it("opens a chosen background tab in split view without changing the active tab", () => {
    const first = openUrlInActiveWorkspace(createDefaultState(), "first.test", "First");
    const second = openUrlInActiveWorkspace(first, "second.test", "Second");
    const workspace = getActiveWorkspace(second);
    const firstTab = workspace.tabs.find((tab) => tab.title === "First")!;
    const split = openTabInSplit(second, firstTab.id);

    expect(getActiveTab(getActiveWorkspace(split)).title).toBe("Second");
    expect(split.splitMode).toBe(true);
    expect(split.splitTabId).toBe(firstTab.id);
    expect(split.splitTabIds).toEqual([firstTab.id]);
  });

  it("fills split view with up to four visible tabs", () => {
    const first = openUrlInActiveWorkspace(createDefaultState(), "first.test", "First");
    const second = openUrlInActiveWorkspace(first, "second.test", "Second");
    const third = openUrlInActiveWorkspace(second, "third.test", "Third");
    const filled = fillSplitView(third);
    const workspace = getActiveWorkspace(filled);

    expect(filled.splitMode).toBe(true);
    expect(filled.splitTabIds).toHaveLength(3);
    expect(new Set([workspace.activeTabId, ...filled.splitTabIds]).size).toBe(4);
  });

  it("opens a url directly into split view without changing the active tab", () => {
    const initial = createDefaultState();
    const activeTab = getActiveTab(getActiveWorkspace(initial));
    const split = openUrlInSplit(initial, "preview.example", "Preview");
    const workspace = getActiveWorkspace(split);
    const preview = workspace.tabs.find((tab) => tab.title === "Preview")!;

    expect(getActiveTab(workspace).id).toBe(activeTab.id);
    expect(preview.url).toBe("https://preview.example/");
    expect(split.splitTabIds).toEqual([preview.id]);
  });

  it("focuses a split pane while keeping the previous active tab in the split", () => {
    const first = openUrlInActiveWorkspace(createDefaultState(), "first.test", "First");
    const second = openUrlInActiveWorkspace(first, "second.test", "Second");
    const third = openUrlInActiveWorkspace(second, "third.test", "Third");
    const filled = fillSplitView(third);
    const previousActiveId = getActiveWorkspace(filled).activeTabId!;
    const splitTargetId = filled.splitTabIds[1]!;
    const focused = focusSplitPane(filled, splitTargetId);

    expect(getActiveWorkspace(focused).activeTabId).toBe(splitTargetId);
    expect(focused.splitTabIds).toContain(previousActiveId);
    expect(focused.splitTabIds).not.toContain(splitTargetId);
    expect(focused.splitTabIds).toHaveLength(filled.splitTabIds.length);
  });

  it("removes an individual background split pane and clears split from the active pane", () => {
    const first = openUrlInActiveWorkspace(createDefaultState(), "first.test", "First");
    const second = openUrlInActiveWorkspace(first, "second.test", "Second");
    const third = openUrlInActiveWorkspace(second, "third.test", "Third");
    const filled = fillSplitView(third);
    const workspace = getActiveWorkspace(filled);
    const firstSplitId = filled.splitTabIds[0]!;
    const secondSplitId = filled.splitTabIds[1]!;
    const withoutOnePane = removeTabFromSplit(filled, firstSplitId);
    const clearedFromActive = removeTabFromSplit(filled, workspace.activeTabId!);

    expect(withoutOnePane.splitMode).toBe(true);
    expect(withoutOnePane.splitTabIds).not.toContain(firstSplitId);
    expect(withoutOnePane.splitTabIds).toContain(secondSplitId);
    expect(getActiveWorkspace(withoutOnePane).tabs.some((tab) => tab.id === firstSplitId)).toBe(true);
    expect(clearedFromActive.splitMode).toBe(false);
    expect(clearedFromActive.splitTabId).toBeNull();
    expect(clearedFromActive.splitTabIds).toEqual([]);
  });

  it("toggles the active tab as a global essential", () => {
    const opened = openUrlInActiveWorkspace(createDefaultState(), "mail.example", "Mail");
    const added = toggleActiveTabEssential(opened);
    const removed = toggleActiveTabEssential(added);

    expect(added.essentials.some((essential) => essential.url === "https://mail.example/")).toBe(true);
    expect(removed.essentials.some((essential) => essential.url === "https://mail.example/")).toBe(false);
  });

  it("moves an active tab into another workspace and keeps a source tab", () => {
    const opened = openUrlInActiveWorkspace(createDefaultState(), "example.com", "Example");
    const movingTab = getActiveTab(getActiveWorkspace(opened));
    const moved = moveTabToWorkspace(opened, movingTab.id, "work");
    const source = moved.workspaces.find((workspace) => workspace.id === "personal")!;
    const target = moved.workspaces.find((workspace) => workspace.id === "work")!;

    expect(moved.activeWorkspaceId).toBe("work");
    expect(target.activeTabId).toBe(movingTab.id);
    expect(target.tabs.at(-1)?.url).toBe("https://example.com/");
    expect(source.tabs.some((tab) => tab.id === movingTab.id)).toBe(false);
    expect(source.tabs.length).toBeGreaterThan(0);
    expect(moved.splitMode).toBe(false);
  });

  it("opens urls and records bounded history", () => {
    const opened = openUrlInActiveWorkspace(createDefaultState(), "example.com", "Example");
    const activeTab = getActiveTab(getActiveWorkspace(opened));
    const withHistory = recordHistory(opened, activeTab.id, activeTab.url);

    expect(activeTab.url).toBe("https://example.com/");
    expect(withHistory.history[0].title).toBe("Example");
    expect(recordHistory(withHistory, activeTab.id, "about:blank").history).toHaveLength(1);
  });

  it("clears local browsing data without removing tabs", () => {
    const opened = openUrlInActiveWorkspace(createDefaultState(), "example.com", "Example");
    const activeTab = getActiveTab(getActiveWorkspace(opened));
    const withHistory = recordHistory(opened, activeTab.id, activeTab.url);
    withHistory.downloads.push({
      id: "download",
      filename: "file.zip",
      totalBytes: 10,
      receivedBytes: 10,
      savePath: "/tmp/file.zip",
      state: "completed",
      startedAt: 1
    });
    withHistory.sitePermissions.push({
      profileId: "personal",
      origin: "https://example.com",
      permission: "media",
      decision: "allow",
      updatedAt: 1
    });
    const cleared = clearBrowsingData(withHistory);

    expect(cleared.history).toHaveLength(0);
    expect(cleared.downloads).toHaveLength(0);
    expect(cleared.sitePermissions).toHaveLength(0);
    expect(getActiveWorkspace(cleared).tabs).toHaveLength(getActiveWorkspace(withHistory).tabs.length);
  });

  it("clears browsing data for one workspace profile", () => {
    const personalOpen = openUrlInActiveWorkspace(createDefaultState(), "personal.test", "Personal");
    const personalTab = getActiveTab(getActiveWorkspace(personalOpen));
    const withPersonalHistory = recordHistory(personalOpen, personalTab.id, personalTab.url);
    const workSelected = switchWorkspace(withPersonalHistory, "work");
    const workOpen = openUrlInActiveWorkspace(workSelected, "work.test", "Work");
    const workTab = getActiveTab(getActiveWorkspace(workOpen));
    const withHistory = recordHistory(workOpen, workTab.id, workTab.url);
    const withPermissions = {
      ...withHistory,
      sitePermissions: [
        {
          profileId: "personal",
          origin: "https://personal.test",
          permission: "media",
          decision: "allow" as const,
          updatedAt: 1
        },
        {
          profileId: "work",
          origin: "https://work.test",
          permission: "notifications",
          decision: "block" as const,
          updatedAt: 2
        }
      ]
    };
    const cleared = clearWorkspaceBrowsingData(withPermissions, "personal");

    expect(cleared.history.map((entry) => entry.workspaceId)).toEqual(["work"]);
    expect(cleared.sitePermissions.map((rule) => rule.profileId)).toEqual(["work"]);
    expect(cleared.downloads).toEqual(withPermissions.downloads);
    expect(getActiveWorkspace(cleared).tabs).toHaveLength(getActiveWorkspace(withPermissions).tabs.length);
  });

  it("clears site permissions for one origin in one profile", () => {
    const state = {
      ...createDefaultState(),
      sitePermissions: [
        { profileId: "personal", origin: "https://example.com", permission: "media", decision: "allow" as const, updatedAt: 1 },
        { profileId: "personal", origin: "https://example.com", permission: "geolocation", decision: "block" as const, updatedAt: 2 },
        { profileId: "work", origin: "https://example.com", permission: "media", decision: "allow" as const, updatedAt: 3 }
      ]
    };

    const cleared = clearSitePermissionRulesForOrigin(state, "personal", "https://example.com");

    expect(cleared.sitePermissions).toEqual([
      { profileId: "work", origin: "https://example.com", permission: "media", decision: "allow", updatedAt: 3 }
    ]);
  });

  it("removes single history entries and clears history only", () => {
    const first = openUrlInActiveWorkspace(createDefaultState(), "first.test", "First");
    const firstTab = getActiveTab(getActiveWorkspace(first));
    const withFirstHistory = recordHistory(first, firstTab.id, firstTab.url);
    const second = openUrlInActiveWorkspace(withFirstHistory, "second.test", "Second");
    const secondTab = getActiveTab(getActiveWorkspace(second));
    const withHistory = recordHistory(second, secondTab.id, secondTab.url);
    const removed = removeHistoryEntry(withHistory, withHistory.history[1].id);
    const cleared = clearHistory({ ...removed, downloads: [{ id: "download", filename: "a.zip", totalBytes: 1, receivedBytes: 1, savePath: "", state: "completed", startedAt: 1 }] });

    expect(removed.history.map((entry) => entry.title)).toEqual(["Second"]);
    expect(cleared.history).toHaveLength(0);
    expect(cleared.downloads).toHaveLength(1);
  });

  it("updates active tab zoom", () => {
    const zoomed = setActiveTabZoom(createDefaultState(), 1.5);
    const stepped = stepActiveTabZoom(zoomed, -1);
    const reset = resetActiveTabZoom(stepped);

    expect(getActiveTab(getActiveWorkspace(zoomed)).zoomFactor).toBe(1.5);
    expect(getActiveTab(getActiveWorkspace(stepped)).zoomFactor).toBe(1.4);
    expect(getActiveTab(getActiveWorkspace(reset)).zoomFactor).toBe(1);
  });

  it("toggles active tab mute state", () => {
    const muted = toggleActiveTabMuted(createDefaultState());
    const unmuted = toggleActiveTabMuted(muted);

    expect(getActiveTab(getActiveWorkspace(muted)).isMuted).toBe(true);
    expect(getActiveTab(getActiveWorkspace(unmuted)).isMuted).toBe(false);
  });

  it("sleeps inactive tabs and wakes them when selected", () => {
    const first = openUrlInActiveWorkspace(createDefaultState(), "first.test", "First");
    const second = openUrlInActiveWorkspace(first, "second.test", "Second");
    const slept = sleepInactiveTabs(second);
    const workspace = getActiveWorkspace(slept);
    const firstTab = workspace.tabs.find((tab) => tab.title === "First")!;
    const selected = selectTab(slept, firstTab.id);

    expect(getActiveTab(workspace).title).toBe("Second");
    expect(firstTab.isSleeping).toBe(true);
    expect(getActiveTab(getActiveWorkspace(selected)).title).toBe("First");
    expect(getActiveTab(getActiveWorkspace(selected)).isSleeping).toBe(false);
  });

  it("keeps Memory Saver manual sleep as a no-op when no tabs can be released", () => {
    const state = createDefaultState();
    getActiveWorkspace(state).activeTabId = getActiveWorkspace(state).tabs[0].id;
    const withBackground = openUrlInActiveWorkspace(state, "docs.test", "Docs");
    const slept = sleepInactiveTabs(withBackground);

    expect(sleepInactiveTabs(state)).toBe(state);
    expect(sleepInactiveTabs(slept)).toBe(slept);
  });

  it("sleeps a selected tab by moving focus to a neighbor", () => {
    const first = openUrlInActiveWorkspace(createDefaultState(), "first.test", "First");
    const second = openUrlInActiveWorkspace(first, "second.test", "Second");
    const active = getActiveTab(getActiveWorkspace(second));
    const slept = sleepTab(second, active.id);
    const workspace = getActiveWorkspace(slept);
    const sleepingTab = workspace.tabs.find((tab) => tab.id === active.id)!;

    expect(sleepingTab.isSleeping).toBe(true);
    expect(getActiveTab(workspace).id).not.toBe(active.id);
    expect(slept.splitMode).toBe(false);
  });

  it("moves focus forward before sleeping the first active tab", () => {
    const first = openUrlInActiveWorkspace(createDefaultState(), "first.test", "First");
    const second = openUrlInActiveWorkspace(first, "second.test", "Second");
    const workspace = getActiveWorkspace(second);
    const firstTab = workspace.tabs.find((tab) => tab.title === "New Tab")!;
    const secondTab = workspace.tabs.find((tab) => tab.title === "First")!;
    secondTab.isSleeping = true;
    workspace.activeTabId = firstTab.id;

    const slept = sleepTab(second, firstTab.id);
    const sleptWorkspace = getActiveWorkspace(slept);

    expect(sleptWorkspace.tabs.find((tab) => tab.id === firstTab.id)?.isSleeping).toBe(true);
    expect(getActiveTab(sleptWorkspace).id).toBe(secondTab.id);
    expect(getActiveTab(sleptWorkspace).isSleeping).toBe(false);
  });

  it("sleeps a tab group while protecting active and split tabs", () => {
    const grouped = groupActiveTab(openUrlInActiveWorkspace(createDefaultState(), "docs.test", "Docs"));
    const group = getActiveWorkspace(grouped).tabGroups[0];
    const withNews = openUrlInActiveWorkspace(grouped, "news.test", "News");
    const newsTab = getActiveTab(getActiveWorkspace(withNews));
    const groupedNews = assignTabToGroup(withNews, newsTab.id, group.id);
    const withLater = openUrlInActiveWorkspace(groupedNews, "later.test", "Later");
    const laterTab = getActiveTab(getActiveWorkspace(withLater));
    const groupedLater = assignTabToGroup(withLater, laterTab.id, group.id);
    const docsTab = getActiveWorkspace(groupedLater).tabs.find((tab) => tab.title === "Docs")!;
    const split = openTabInSplit(groupedLater, docsTab.id);
    const slept = sleepTabGroup(split, group.id);
    const workspace = getActiveWorkspace(slept);

    expect(workspace.tabs.find((tab) => tab.title === "News")?.isSleeping).toBe(true);
    expect(workspace.tabs.find((tab) => tab.title === "Docs")?.isSleeping).toBe(false);
    expect(getActiveTab(workspace).title).toBe("Later");
    expect(getActiveTab(workspace).isSleeping).toBe(false);
    expect(slept.splitTabIds).toContain(docsTab.id);
  });

  it("keeps sleeping a protected tab group as a no-op", () => {
    const grouped = groupActiveTab(openUrlInActiveWorkspace(createDefaultState(), "docs.test", "Docs"));
    const group = getActiveWorkspace(grouped).tabGroups[0];
    const activeTab = getActiveTab(getActiveWorkspace(grouped));

    expect(activeTab.groupId).toBe(group.id);
    expect(sleepTabGroup(grouped, group.id)).toBe(grouped);
  });

  it("automatically sleeps idle background tabs while protecting active split and pinned tabs", () => {
    const first = openUrlInActiveWorkspace(createDefaultState(), "first.test", "First");
    const second = openUrlInActiveWorkspace(first, "second.test", "Second");
    const third = openUrlInActiveWorkspace(second, "third.test", "Third");
    const splitCandidate = getActiveWorkspace(third).tabs.find((tab) => tab.title === "First")!;
    const split = openTabInSplit(third, splitCandidate.id);
    const workspace = getActiveWorkspace(split);
    const active = getActiveTab(workspace);
    const splitTab = workspace.tabs.find((tab) => tab.title === "First")!;
    const background = workspace.tabs.find((tab) => tab.title === "Second")!;
    const pinned = workspace.tabs.find((tab) => tab.title === "New Tab")!;
    const now = 1_000_000;
    background.lastActiveAt = now - 31 * 60_000;
    active.lastActiveAt = now - 31 * 60_000;
    splitTab.lastActiveAt = now - 31 * 60_000;
    pinned.isPinned = true;
    pinned.lastActiveAt = now - 31 * 60_000;

    const slept = sleepIdleTabs(split, now);
    const sleptWorkspace = getActiveWorkspace(slept);

    expect(sleptWorkspace.tabs.find((tab) => tab.id === background.id)?.isSleeping).toBe(true);
    expect(sleptWorkspace.tabs.find((tab) => tab.id === active.id)?.isSleeping).toBe(false);
    expect(sleptWorkspace.tabs.find((tab) => tab.id === splitTab.id)?.isSleeping).toBe(false);
    expect(sleptWorkspace.tabs.find((tab) => tab.id === pinned.id)?.isSleeping).toBe(false);
  });

  it("does not sleep idle tabs when automatic Memory Saver is disabled", () => {
    const first = openUrlInActiveWorkspace(createDefaultState(), "first.test", "First");
    const second = openUrlInActiveWorkspace(first, "second.test", "Second");
    const workspace = getActiveWorkspace(second);
    const background = workspace.tabs.find((tab) => tab.title === "First")!;
    const now = 1_000_000;
    second.settings.memorySaverEnabled = false;
    background.lastActiveAt = now - 31 * 60_000;

    expect(sleepIdleTabs(second, now)).toBe(second);
  });

  it("keeps automatic Memory Saver as a no-op when idle tabs are protected", () => {
    const first = openUrlInActiveWorkspace(createDefaultState(), "first.test", "First");
    const workspace = getActiveWorkspace(first);
    const active = getActiveTab(workspace);
    const pinned = workspace.tabs.find((tab) => tab.title === "New Tab")!;
    const now = 1_000_000;
    active.lastActiveAt = now - 31 * 60_000;
    pinned.isPinned = true;
    pinned.lastActiveAt = now - 31 * 60_000;

    expect(sleepIdleTabs(first, now)).toBe(first);
  });
});
