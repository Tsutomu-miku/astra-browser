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
  newTabInGroup,
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
  setWorkspaceSplitLayout,
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
  toggleSplitMode,
  updateBrowserState,
  updateTab,
  updateTabGroup
} from "../src/renderer/domain/actions";
import { createDefaultState, createFavorite, createTab } from "../src/renderer/domain/browser";
import { getActiveTab, getActiveWorkspace } from "../src/renderer/domain/browser/selectors";

describe("domain actions", () => {
  it("adds, pins, favorites, and closes tabs immutably", () => {
    const initial = createDefaultState();
    const withTab = addTab(initial);
    const withPin = toggleActiveTabPinned(withTab);
    const withFavorite = toggleActiveTabFavorite(withPin);
    const afterClose = closeActiveTab(withFavorite);

    expect(getActiveWorkspace(withTab).tabs).toHaveLength(4);
    expect(getActiveTab(getActiveWorkspace(withPin)).isPinned).toBe(true);
    expect(getActiveTab(getActiveWorkspace(withFavorite)).isFavorite).toBe(true);
    expect(getActiveWorkspace(afterClose).tabs).toHaveLength(3);
    expect(getActiveWorkspace(afterClose).closedTabs[0].url).toBe(getActiveTab(getActiveWorkspace(withFavorite)).url);
    expect(initial.workspaces[0].tabs).toHaveLength(3);
  });

  it("opens new and replacement tabs at the active workspace homepage", () => {
    const base = createDefaultState();
    const initial = {
      ...base,
      workspaces: base.workspaces.map((workspace) => {
        if (workspace.id !== "personal") return workspace;
        const onlyNewTab = workspace.tabs.filter((tab) => !tab.isFavorite);
        return {
          ...workspace,
          homepage: "https://space.example/",
          tabs: onlyNewTab,
          favoriteOrder: []
        };
      })
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

  it("restores the most recently closed tab with full metadata", () => {
    const base = openUrlInActiveWorkspace(createDefaultState(), "example.com", "Example");
    const originalId = getActiveWorkspace(base).activeTabId!;
    // Add a second tab first so the group survives the close (empty groups
    // are pruned, which would null the restored tab's groupId even if the
    // closed snapshot preserved it).
    const withSecond = openUrlInActiveWorkspace(base, "other.test", "Other");
    // groupActiveTab creates a real group and assigns the active tab to it.
    const grouped = groupActiveTab(withSecond);
    const groupedWorkspace = getActiveWorkspace(grouped);
    const groupId = groupedWorkspace.tabGroups[0]?.id;
    expect(groupId).toBeTruthy();
    // Put the original tab in the same group, then mark it muted/etc.
    // Note: pinned and grouped are mutually exclusive, so this test exercises
    // grouped (not pinned) metadata alongside muted/customTitle/zoom.
    const assigned = assignTabToGroup(grouped, originalId, groupId);
    const prepared = updateBrowserState(assigned, (draft) => {
      const tab = getActiveWorkspace(draft).tabs.find((candidate) => candidate.id === originalId)!;
      tab.customTitle = "My Tab";
      tab.isMuted = true;
      tab.isMediaPlaying = true;
      tab.isCameraOn = true;
      tab.isMicrophoneOn = true;
      tab.hasUnread = true;
    });
    // Zoom needs to be applied to the active tab — ensure original is active.
    const activated = updateBrowserState(prepared, (draft) => {
      getActiveWorkspace(draft).activeTabId = originalId;
    });
    const zoomed = setActiveTabZoom(activated, 1.5);
    const closed = closeTab(zoomed, originalId);
    const closedWorkspace = getActiveWorkspace(closed);

    expect(closedWorkspace.closedTabs).toHaveLength(1);
    const closedSnapshot = closedWorkspace.closedTabs[0];
    expect(closedSnapshot.isPinned).toBe(false);
    expect(closedSnapshot.isMuted).toBe(true);
    expect(closedSnapshot.groupId).toBe(groupId);
    expect(closedSnapshot.customTitle).toBe("My Tab");
    expect(closedSnapshot.zoomFactor).toBe(1.5);
    // Runtime-only flags must NOT leak into the closed-tab snapshot.
    expect((closedSnapshot as unknown as Record<string, unknown>).isMediaPlaying).toBeUndefined();
    expect((closedSnapshot as unknown as Record<string, unknown>).isCameraOn).toBeUndefined();
    expect((closedSnapshot as unknown as Record<string, unknown>).isMicrophoneOn).toBeUndefined();
    expect((closedSnapshot as unknown as Record<string, unknown>).hasUnread).toBeUndefined();

    const restored = restoreLastClosedTab(closed);
    const restoredTab = getActiveTab(getActiveWorkspace(restored));
    expect(restoredTab.id).not.toBe(originalId);
    expect(restoredTab.url).toBe("https://example.com/");
    expect(restoredTab.title).toBe("Example");
    expect(restoredTab.customTitle).toBe("My Tab");
    expect(restoredTab.groupId).toBe(groupId);
    expect(restoredTab.isMuted).toBe(true);
    expect(restoredTab.isPinned).toBe(false);
    expect(restoredTab.zoomFactor).toBe(1.5);
    // Runtime flags are reset for the newly restored tab.
    expect(restoredTab.isMediaPlaying).toBe(false);
    expect(restoredTab.isCameraOn).toBe(false);
    expect(restoredTab.isMicrophoneOn).toBe(false);
    expect(restoredTab.hasUnread).toBe(false);
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
    expect(source.tabs.map((candidate) => candidate.id)).toContain(source.activeTabId);
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
    expect(nextWorkspace.closedTabs.map((tab) => tab.title)).toEqual(["Third", "Second", "MDN", "Chromium", "New Tab"]);
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

    expect(titles).toEqual(["New Tab", "Chromium", "MDN", "First", "Second"]);
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

    expect(nextWorkspace.tabs.map((tab) => tab.title)).toEqual(["New Tab", "Chromium", "MDN", "First"]);
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
    expect(nextWorkspace.closedTabs.map((tab) => tab.title).slice(0, 2)).toEqual(["First", "MDN"]);
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
    expect(workspace.tabs.map((tab) => tab.title)).toEqual(["New Tab", "Chromium", "MDN", "Third"]);
    expect(getActiveTab(workspace).title).toBe("Third");
    expect(workspace.closedTabs.map((tab) => tab.title).slice(0, 2)).toEqual(["News", "Docs"]);
  });

  it("replaces the last open tab when closing its tab group", () => {
    const base = createDefaultState();
    const personal = base.workspaces.find((ws) => ws.id === "personal")!;
    // Strip default favorites so the grouped tab is the last remaining tab.
    personal.tabs = personal.tabs.filter((tab) => tab.title === "New Tab");
    personal.favoriteOrder = [];
    const grouped = groupActiveTab(openUrlInActiveWorkspace(base, "docs.test", "Docs"));
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

  it("resets transient runtime flags when duplicating a tab", () => {
    const base = openUrlInActiveWorkspace(createDefaultState(), "media.example", "Media");
    const originalId = getActiveWorkspace(base).activeTabId!;
    const prepared = updateBrowserState(base, (draft) => {
      const tab = getActiveWorkspace(draft).tabs.find((candidate) => candidate.id === originalId)!;
      tab.isMediaPlaying = true;
      tab.isCameraOn = true;
      tab.isMicrophoneOn = true;
      tab.isSleeping = true;
      tab.hasUnread = true;
    });
    const duplicated = duplicateTab(prepared, originalId);
    const copy = getActiveWorkspace(duplicated).tabs.find(
      (candidate) => candidate.id !== originalId && candidate.url === "https://media.example/"
    )!;

    expect(copy).toBeTruthy();
    expect(copy.id).not.toBe(originalId);
    expect(copy.isMediaPlaying).toBe(false);
    expect(copy.isCameraOn).toBe(false);
    expect(copy.isMicrophoneOn).toBe(false);
    expect(copy.isSleeping).toBe(false);
    expect(copy.hasUnread).toBe(false);
    expect(copy.canGoBack).toBe(false);
    expect(copy.canGoForward).toBe(false);
    expect(copy.isLoading).toBe(false);
    // Persistent preferences should still be carried over.
    expect(copy.zoomFactor).toBe(getActiveWorkspace(prepared).tabs.find((t) => t.id === originalId)!.zoomFactor);
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
    const addedFavWorkspace = getActiveWorkspace(addedFavorite);
    expect(addedFavWorkspace.tabs.find((t) => t.id === docsTab.id)?.isFavorite).toBe(true);
    expect(addedFavWorkspace.favoriteOrder).toContain(docsTab.id);
    expect(addedEssential.essentials.at(-1)).toMatchObject({
      title: "Docs",
      url: docsTab.url
    });
    expect(getActiveWorkspace(removedFavorite).tabs.find((t) => t.id === docsTab.id)?.isFavorite).toBe(false);
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
    const favorites = getActiveWorkspace(withDuplicateFavorite).tabs.filter((t) => t.isFavorite && t.url === docsTab.url);

    expect(favorites.map((t) => t.id)).toContain(docsTab.id);
    expect(favorites.map((t) => t.id)).toContain(duplicateTab.id);
  });

  it("toggles Favorites by tab identity", () => {
    const first = openUrlInActiveWorkspace(createDefaultState(), "docs.example", "Docs");
    const second = openUrlInActiveWorkspace(first, "docs.example", "Docs duplicate");
    const workspace = getActiveWorkspace(second);
    const docsTab = workspace.tabs.find((tab) => tab.title === "Docs")!;
    const duplicateTab = workspace.tabs.find((tab) => tab.title === "Docs duplicate")!;
    const withDocsFavorite = addTabToFavorites(second, docsTab.id);
    const withDuplicateFavorite = toggleTabFavorite(withDocsFavorite, duplicateTab.id);
    const withoutDuplicateFavorite = toggleTabFavorite(withDuplicateFavorite, duplicateTab.id);
    const remainingFavorites = getActiveWorkspace(withoutDuplicateFavorite).tabs.filter((t) => t.isFavorite && t.url === docsTab.url);

    expect(getActiveWorkspace(withDuplicateFavorite).tabs.filter((t) => t.isFavorite).map((t) => t.id)).toContain(duplicateTab.id);
    expect(remainingFavorites).toHaveLength(1);
    expect(remainingFavorites[0].id).toBe(docsTab.id);
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
    expect(workspace.tabs.find((t) => t.id === docsTab.id)?.isFavorite).toBe(true);
    expect(workspace.favoriteOrder).toContain(docsTab.id);
  });

  it("removes favorites by tab id and essentials by url", () => {
    const withFavorite = toggleActiveTabFavorite(openUrlInActiveWorkspace(createDefaultState(), "docs.example", "Docs"));
    const withEssential = toggleActiveTabEssential(withFavorite);
    const workspace = getActiveWorkspace(withEssential);
    const tabId = getActiveTab(workspace).id;
    const url = getActiveTab(workspace).url;
    const withoutFavorite = removeWorkspaceFavorite(withEssential, tabId);
    const withoutEssential = removeEssential(withoutFavorite, url);

    expect(getActiveWorkspace(withoutFavorite).tabs.find((t) => t.id === tabId)?.isFavorite).toBe(false);
    expect(withoutEssential.essentials.some((essential) => essential.url === url)).toBe(false);
  });

  it("removes a single Favorite when duplicate Favorite URLs exist", () => {
    const first = openUrlInActiveWorkspace(createDefaultState(), "docs.example", "Docs");
    const second = openUrlInActiveWorkspace(first, "docs.example", "Docs duplicate");
    const workspace = getActiveWorkspace(second);
    const docsTab = workspace.tabs.find((tab) => tab.title === "Docs")!;
    const duplicateTab = workspace.tabs.find((tab) => tab.title === "Docs duplicate")!;
    const withFavorites = addTabToFavorites(addTabToFavorites(second, docsTab.id), duplicateTab.id);
    const withoutDuplicate = removeWorkspaceFavorite(withFavorites, duplicateTab.id);
    const remainingFavorites = getActiveWorkspace(withoutDuplicate).tabs.filter((t) => t.isFavorite && t.url === docsTab.url);

    expect(remainingFavorites).toHaveLength(1);
    expect(remainingFavorites[0].id).toBe(docsTab.id);
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
    expect(closedWorkspace.favoriteOrder.includes(docsTab.id)).toBe(false);
    expect(closedWorkspace.tabs.some((t) => t.isFavorite && t.url === docsTab.url)).toBe(false);
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

    expect(moved.isPinned).toBe(false);
    expect(moved.groupId).toBeNull();
    expect(moved.isFavorite).toBe(true);
    expect(getActiveWorkspace(favorited).favoriteOrder).toContain(first.id);
  });

  it("moves Favorite-backed tabs out of Favorites through tab folder actions", () => {
    const withFirst = openUrlInActiveWorkspace(createDefaultState(), "first.test", "First");
    const workspace = getActiveWorkspace(withFirst);
    const first = getActiveTab(workspace);
    const favorited = moveTabToFolderEnd(withFirst, first.id, { type: "favorites" });
    const unpinned = moveTabToFolderEnd(favorited, first.id, { type: "tabs" });

    expect(getActiveWorkspace(unpinned).tabs.find((t) => t.id === first.id)?.isFavorite).toBe(false);
    expect(getActiveWorkspace(unpinned).tabs.at(-1)?.id).toBe(first.id);
  });

  it("clears favorite status when a favorite tab is moved into a non-favorite group", () => {
    const grouped = groupActiveTab(createDefaultState());
    const workspace = getActiveWorkspace(grouped);
    const group = workspace.tabGroups[0];
    const favoriteTab = workspace.tabs.find((tab) => tab.isFavorite)!;

    const moved = moveTabToFolderEnd(grouped, favoriteTab.id, { type: "group", groupId: group.id });
    const movedTab = getActiveWorkspace(moved).tabs.find((tab) => tab.id === favoriteTab.id)!;

    expect(movedTab.groupId).toBe(group.id);
    expect(movedTab.isFavorite).toBe(false);
    expect(getActiveWorkspace(moved).favoriteOrder).not.toContain(movedTab.id);
  });

  it("places dragged tabs after a group without joining it", () => {
    const withNews = openUrlInActiveWorkspace(groupActiveTab(createDefaultState()), "news.example", "News");
    const workspace = getActiveWorkspace(withNews);
    const group = workspace.tabGroups[0];
    const groupedTarget = workspace.tabs.find((tab) => tab.groupId === group.id)!;
    const newsTab = workspace.tabs.find((tab) => tab.title === "News")!;
    const grouped = moveTabToFolderPosition(withNews, newsTab.id, groupedTarget.id, "after");
    const updatedWorkspace = getActiveWorkspace(grouped);
    const moved = updatedWorkspace.tabs.find((tab) => tab.id === newsTab.id)!;

    // before/after only reorders; it never drops the tab into the target's group.
    expect(moved.isPinned).toBe(false);
    expect(moved.groupId).toBeNull();
    const groupIndices = updatedWorkspace.tabs
      .map((tab, index) => (tab.groupId === group.id ? index : -1))
      .filter((index) => index >= 0);
    expect(updatedWorkspace.tabs.indexOf(moved)).toBe(Math.max(...groupIndices) + 1);
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
    // Strip default favorites so the test controls the full favorite set.
    workspace.tabs = workspace.tabs.filter((tab) => !tab.isFavorite);
    workspace.favoriteOrder = [];
    const first = createTab("First", "https://first.example");
    first.isFavorite = true;
    const second = createTab("Second", "https://second.example");
    second.isFavorite = true;
    const third = createTab("Third", "https://third.example");
    third.isFavorite = true;
    workspace.tabs.push(first, second, third);
    workspace.favoriteOrder = [first.id, second.id, third.id];

    const movedBefore = reorderWorkspaceFavorite(initial, third.id, first.id, "before");
    const movedAfter = reorderWorkspaceFavorite(movedBefore, first.id, second.id, "after");
    const ignored = reorderWorkspaceFavorite(movedAfter, "missing", second.id, "before");

    const wsBefore = getActiveWorkspace(movedBefore);
    expect(wsBefore.favoriteOrder.map((id) => wsBefore.tabs.find((t) => t.id === id)?.title)).toEqual(["Third", "First", "Second"]);
    const wsAfter = getActiveWorkspace(movedAfter);
    expect(wsAfter.favoriteOrder.map((id) => wsAfter.tabs.find((t) => t.id === id)?.title)).toEqual(["Third", "Second", "First"]);
    const wsIgnored = getActiveWorkspace(ignored);
    expect(wsIgnored.favoriteOrder.map((id) => wsIgnored.tabs.find((t) => t.id === id)?.title)).toEqual(["Third", "Second", "First"]);
  });

  it("moves a Space favorite to another workspace", () => {
    const base = createDefaultState();
    const personalWs = base.workspaces.find((ws) => ws.id === "personal")!;
    personalWs.tabs = personalWs.tabs.filter((tab) => !tab.isFavorite);
    personalWs.favoriteOrder = [];
    const initial = openUrlInActiveWorkspace(base, "docs.example", "Docs");
    const workspace = getActiveWorkspace(initial);
    const docsTab = getActiveTab(workspace);
    docsTab.isFavorite = true;
    workspace.favoriteOrder = [docsTab.id];

    const moved = moveWorkspaceFavoriteToWorkspace(initial, docsTab.id, "work");
    const personal = moved.workspaces.find((candidate) => candidate.id === "personal")!;
    const work = moved.workspaces.find((candidate) => candidate.id === "work")!;

    expect(moved.activeWorkspaceId).toBe("work");
    expect(personal.favoriteOrder).toHaveLength(0);
    expect(personal.tabs.some((tab) => tab.id === docsTab.id)).toBe(false);
    expect(work.tabs.some((tab) => tab.id === docsTab.id)).toBe(true);
    expect(work.activeTabId).toBe(docsTab.id);
    expect(work.tabs.find((t) => t.id === docsTab.id)?.isFavorite).toBe(true);
    expect(work.favoriteOrder).toContain(docsTab.id);
  });

  it("preserves the Favorites folder when a Favorite tab moves to another workspace", () => {
    const base = createDefaultState();
    const personalWs = base.workspaces.find((ws) => ws.id === "personal")!;
    personalWs.tabs = personalWs.tabs.filter((tab) => !tab.isFavorite);
    personalWs.favoriteOrder = [];
    const initial = openUrlInActiveWorkspace(base, "docs.example", "Docs");
    const workspace = getActiveWorkspace(initial);
    const docsTab = getActiveTab(workspace);
    docsTab.isFavorite = true;
    workspace.favoriteOrder = [docsTab.id];

    const moved = moveTabToWorkspace(initial, docsTab.id, "work");
    const personal = moved.workspaces.find((candidate) => candidate.id === "personal")!;
    const work = moved.workspaces.find((candidate) => candidate.id === "work")!;

    expect(personal.favoriteOrder).toHaveLength(0);
    expect(work.tabs.some((tab) => tab.id === docsTab.id)).toBe(true);
    expect(work.tabs.find((t) => t.id === docsTab.id)?.isFavorite).toBe(true);
    expect(work.favoriteOrder).toContain(docsTab.id);
  });

  it("preserves the Favorites folder when a Favorite tab creates a new workspace", () => {
    const base = createDefaultState();
    const personalWs = base.workspaces.find((ws) => ws.id === "personal")!;
    personalWs.tabs = personalWs.tabs.filter((tab) => !tab.isFavorite);
    personalWs.favoriteOrder = [];
    const initial = openUrlInActiveWorkspace(base, "docs.example", "Docs");
    const workspace = getActiveWorkspace(initial);
    const docsTab = getActiveTab(workspace);
    docsTab.isFavorite = true;
    workspace.favoriteOrder = [docsTab.id];

    const moved = moveTabToNewWorkspace(initial, docsTab.id);
    const source = moved.workspaces.find((candidate) => candidate.id === "personal")!;
    const target = getActiveWorkspace(moved);

    expect(source.favoriteOrder).toHaveLength(0);
    expect(target.tabs.find((t) => t.id === docsTab.id)?.isFavorite).toBe(true);
    expect(target.favoriteOrder).toEqual([docsTab.id]);
    expect(target.tabs.map((tab) => tab.id)).toEqual([docsTab.id]);
  });

  it("moves a Space favorite to a new workspace", () => {
    const base = createDefaultState();
    const personalWs = base.workspaces.find((ws) => ws.id === "personal")!;
    personalWs.tabs = personalWs.tabs.filter((tab) => !tab.isFavorite);
    personalWs.favoriteOrder = [];
    const initial = openUrlInActiveWorkspace(base, "docs.example", "Docs");
    const workspace = getActiveWorkspace(initial);
    const docsTab = getActiveTab(workspace);
    docsTab.isFavorite = true;
    workspace.favoriteOrder = [docsTab.id];

    const moved = moveWorkspaceFavoriteToNewWorkspace(initial, docsTab.id);
    const source = moved.workspaces.find((candidate) => candidate.id === "personal")!;
    const target = getActiveWorkspace(moved);

    expect(moved.workspaces).toHaveLength(initial.workspaces.length + 1);
    expect(source.favoriteOrder).toHaveLength(0);
    expect(source.tabs.some((tab) => tab.id === docsTab.id)).toBe(false);
    expect(target.name).toBe("Docs");
    expect(target.tabs.find((t) => t.id === docsTab.id)?.isFavorite).toBe(true);
    expect(target.favoriteOrder).toEqual([docsTab.id]);
    expect(target.tabs.map((tab) => tab.id)).toEqual([docsTab.id]);
    expect(getActiveTab(target).id).toBe(docsTab.id);
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

  it("caches tab favicons by site and reuses them on navigation", () => {
    const initial = createDefaultState();
    const tab = getActiveTab(getActiveWorkspace(initial));
    const faviconUrl = "https://docs.example/favicon.ico";
    const withFavicon = updateTab(initial, tab.id, {
      faviconUrl,
      url: "https://docs.example/page-one"
    });
    const withSiblingPage = updateTab(withFavicon, tab.id, {
      faviconUrl: undefined,
      url: "https://docs.example/page-two"
    });
    const updatedTab = getActiveTab(getActiveWorkspace(withSiblingPage));

    expect(withFavicon.faviconCache["https://docs.example"]).toBe(faviconUrl);
    expect(updatedTab.faviconUrl).toBe(faviconUrl);
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

  it("clears loading and navigation affordances when sleeping a releasable tab", () => {
    const first = openUrlInActiveWorkspace(createDefaultState(), "first.test", "First");
    const second = openUrlInActiveWorkspace(first, "second.test", "Second");
    const background = getActiveWorkspace(second).tabs.find((tab) => tab.title === "First")!;
    background.isLoading = true;
    background.canGoBack = true;
    background.canGoForward = true;

    const slept = sleepInactiveTabs(second);
    const sleepingTab = getActiveWorkspace(slept).tabs.find((tab) => tab.id === background.id)!;

    expect(sleepingTab.isSleeping).toBe(true);
    expect(sleepingTab.isLoading).toBe(false);
    expect(sleepingTab.canGoBack).toBe(false);
    expect(sleepingTab.canGoForward).toBe(false);
  });

  it("refreshes sleeping tab activity when waking it into split view", () => {
    const first = openUrlInActiveWorkspace(createDefaultState(), "first.test", "First");
    const second = openUrlInActiveWorkspace(first, "second.test", "Second");
    const slept = sleepInactiveTabs(second);
    const sleepingTab = getActiveWorkspace(slept).tabs.find((tab) => tab.title === "First")!;
    sleepingTab.lastActiveAt = 1;

    const split = openTabInSplit(slept, sleepingTab.id);
    const splitTab = getActiveWorkspace(split).tabs.find((tab) => tab.id === sleepingTab.id)!;

    expect(splitTab.isSleeping).toBe(false);
    expect(splitTab.lastActiveAt).toBeGreaterThan(1);
    expect(split.splitTabIds).toContain(sleepingTab.id);
  });

  it("keeps Memory Saver manual sleep as a no-op when no tabs can be released", () => {
    const base = createDefaultState();
    const personalWs = base.workspaces.find((ws) => ws.id === "personal")!;
    personalWs.tabs = personalWs.tabs.filter((tab) => !tab.isFavorite);
    personalWs.favoriteOrder = [];
    const state = base;
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
    const base = createDefaultState();
    const personalWs = base.workspaces.find((ws) => ws.id === "personal")!;
    personalWs.tabs = personalWs.tabs.filter((tab) => !tab.isFavorite);
    personalWs.favoriteOrder = [];
    const first = openUrlInActiveWorkspace(base, "first.test", "First");
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

  it("keeps manual sleepTab as a no-op for pinned tabs", () => {
    const first = openUrlInActiveWorkspace(createDefaultState(), "first.test", "First");
    const second = openUrlInActiveWorkspace(first, "second.test", "Second");
    const workspace = getActiveWorkspace(second);
    const pinned = workspace.tabs.find((tab) => tab.title === "First")!;
    pinned.isPinned = true;
    const active = getActiveTab(workspace);

    expect(active.id).not.toBe(pinned.id);
    expect(sleepTab(second, pinned.id)).toBe(second);
  });

  it("keeps manual sleepTab as a no-op for non-active split-view tabs", () => {
    const first = openUrlInActiveWorkspace(createDefaultState(), "first.test", "First");
    const second = openUrlInActiveWorkspace(first, "second.test", "Second");
    const split = openTabInSplit(second, getActiveWorkspace(second).tabs[0].id);
    const workspace = getActiveWorkspace(split);
    const splitTab = workspace.tabs.find((tab) => split.splitTabIds.includes(tab.id))!;
    const active = getActiveTab(workspace);

    expect(splitTab.id).not.toBe(active.id);
    expect(sleepTab(split, splitTab.id)).toBe(split);
  });

  it("keeps manual sleepTab as a no-op for already sleeping tabs", () => {
    const first = openUrlInActiveWorkspace(createDefaultState(), "first.test", "First");
    const second = openUrlInActiveWorkspace(first, "second.test", "Second");
    const workspace = getActiveWorkspace(second);
    const background = workspace.tabs.find((tab) => tab.title === "First")!;
    background.isSleeping = true;

    expect(sleepTab(second, background.id)).toBe(second);
  });

  it("creates a new tab at the end of a tab group and selects it", () => {
    const grouped = groupActiveTab(openUrlInActiveWorkspace(createDefaultState(), "docs.test", "Docs"));
    const group = getActiveWorkspace(grouped).tabGroups[0];
    const withNews = openUrlInActiveWorkspace(grouped, "news.test", "News");
    const assigned = assignTabToGroup(withNews, getActiveTab(getActiveWorkspace(withNews)).id, group.id);
    const afterNew = newTabInGroup(assigned, group.id);
    const workspace = getActiveWorkspace(afterNew);
    const groupTabs = workspace.tabs.filter((tab) => tab.groupId === group.id);
    const active = getActiveTab(workspace);

    expect(groupTabs).toHaveLength(3);
    expect(groupTabs[groupTabs.length - 1].title).toBe("New Tab");
    expect(active.groupId).toBe(group.id);
    expect(active.title).toBe("New Tab");
    expect(afterNew.splitMode).toBe(false);
  });

  it("ignores newTabInGroup when the target group does not exist", () => {
    const state = createDefaultState();
    expect(newTabInGroup(state, "missing-group")).toBe(state);
  });

  it("persists split layout per workspace via setWorkspaceSplitLayout", () => {
    const initial = createDefaultState();
    expect(getActiveWorkspace(initial).splitLayout).toBe("horizontal");

    const vertical = setWorkspaceSplitLayout(initial, "vertical");
    expect(getActiveWorkspace(vertical).splitLayout).toBe("vertical");

    const switched = switchWorkspace(vertical, "work");
    expect(getActiveWorkspace(switched).splitLayout).toBe("horizontal");
    expect(switched.workspaces.find((w) => w.id === "personal")?.splitLayout).toBe("vertical");

    const grid = setWorkspaceSplitLayout(switched, "grid");
    expect(getActiveWorkspace(grid).splitLayout).toBe("grid");
    expect(grid.workspaces.find((w) => w.id === "personal")?.splitLayout).toBe("vertical");
  });

  it("returns identity for invalid setWorkspaceSplitLayout calls", () => {
    const state = createDefaultState();
    expect(setWorkspaceSplitLayout(state, "bogus" as never)).toBe(state);
  });

  it("fillSplitView sets the workspace split layout to grid", () => {
    const initial = createDefaultState();
    expect(getActiveWorkspace(initial).splitLayout).toBe("horizontal");

    const filled = fillSplitView(initial);
    expect(getActiveWorkspace(filled).splitLayout).toBe("grid");
  });
});
