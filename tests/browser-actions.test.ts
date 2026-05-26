import { describe, expect, it } from "vitest";

import {
  addTab,
  clearBrowsingData,
  clearHistory,
  clearWorkspaceBrowsingData,
  closeActiveTab,
  closeOtherTabs,
  closeTabsToLeft,
  closeTabsToRight,
  closeTab,
  deleteWorkspace,
  duplicateActiveTab,
  duplicateTab,
  fillSplitView,
  groupActiveTab,
  moveTabToWorkspace,
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
  restoreLastClosedTab,
  selectAdjacentTab,
  selectTab,
  setActiveTabZoom,
  sleepInactiveTabs,
  sleepTab,
  stepActiveTabZoom,
  switchWorkspace,
  toggleActiveTabEssential,
  toggleActiveTabFavorite,
  toggleActiveTabMuted,
  toggleActiveTabPinned,
  toggleTabMuted,
  toggleTabPinned,
  toggleSplitMode
} from "../src/renderer/domain/browser-actions";
import { createDefaultState } from "../src/renderer/domain/browser-core";
import { getActiveTab, getActiveWorkspace } from "../src/renderer/domain/selectors";

describe("browser-actions", () => {
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

  it("reorders workspaces while preserving the active workspace", () => {
    const initial = createDefaultState();
    const reordered = reorderWorkspace(initial, "work", "personal", "before");

    expect(reordered.workspaces.map((workspace) => workspace.id)).toEqual(["work", "personal"]);
    expect(reordered.activeWorkspaceId).toBe(initial.activeWorkspaceId);
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
});
