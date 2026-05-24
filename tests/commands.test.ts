import { describe, expect, it, vi } from "vitest";

import { closeActiveTab, openUrlInActiveWorkspace, toggleActiveTabFavorite } from "../src/renderer/domain/browser-actions";
import { createDefaultState } from "../src/renderer/domain/browser-core";
import { buildCommands } from "../src/renderer/hooks/useCommands";

describe("buildCommands", () => {
  it("includes tabs, favorites, recently closed tabs, and history", () => {
    const withTab = openUrlInActiveWorkspace(createDefaultState(), "example.com", "Example");
    const withFavorite = toggleActiveTabFavorite(withTab);
    const withClosed = closeActiveTab(withFavorite);
    const withSplitCandidate = openUrlInActiveWorkspace(withClosed, "split.example", "Split");
    withSplitCandidate.history.push({
      id: "history",
      title: "History Example",
      url: "https://history.example/",
      workspaceId: withSplitCandidate.activeWorkspaceId,
      visitedAt: Date.now()
    });

    const commands = buildCommands(withSplitCandidate, {
      addWorkspace: vi.fn(),
      clearBrowsingData: vi.fn(),
      clearHistory: vi.fn(),
      clearWorkspaceBrowsingData: vi.fn(),
      assignTabToGroup: vi.fn(),
      closeActiveTab: vi.fn(),
      closeOtherTabs: vi.fn(),
      closeTabsToLeft: vi.fn(),
      closeTabsToRight: vi.fn(),
      deleteWorkspace: vi.fn(),
      duplicateActiveTab: vi.fn(),
      focusAddressBar: vi.fn(),
      groupActiveTab: vi.fn(),
      moveTabToWorkspace: vi.fn(),
      openTabInSplit: vi.fn(),
      newTab: vi.fn(),
      openUrlInActiveWorkspace: vi.fn(),
      restoreLastClosedTab: vi.fn(),
      selectAdjacentTab: vi.fn(),
      resetActiveTabZoom: vi.fn(),
      selectTab: vi.fn(),
      switchWorkspace: vi.fn(),
      toggleActiveTabFavorite: vi.fn(),
      toggleActiveTabMuted: vi.fn(),
      toggleActiveTabPinned: vi.fn(),
      toggleTabGroupCollapsed: vi.fn(),
      toggleSidebar: vi.fn(),
      toggleSplitMode: vi.fn(),
      ungroupActiveTab: vi.fn(),
      zoomIn: vi.fn(),
      zoomOut: vi.fn()
    }, vi.fn());

    expect(commands.some((command) => command.title === "Reopen closed tab")).toBe(true);
    expect(commands.some((command) => command.subtitle.startsWith("Open tab"))).toBe(true);
    expect(commands.some((command) => command.subtitle.startsWith("Favorite"))).toBe(true);
    expect(commands.some((command) => command.subtitle.startsWith("History"))).toBe(true);
    expect(commands.some((command) => command.title === "Toggle sidebar")).toBe(true);
    expect(commands.some((command) => command.title === "Reset zoom")).toBe(true);
    expect(commands.some((command) => command.title === "Duplicate tab")).toBe(true);
    expect(commands.some((command) => command.title === "Group tab")).toBe(true);
    expect(commands.some((command) => command.title === "Next tab")).toBe(true);
    expect(commands.some((command) => command.title === "Previous tab")).toBe(true);
    expect(commands.some((command) => command.title === "Mute tab")).toBe(true);
    expect(commands.some((command) => command.title.startsWith("Move tab to"))).toBe(true);
    expect(commands.some((command) => command.title.includes("in split view"))).toBe(true);
    expect(commands.some((command) => command.title === "Clear browsing data")).toBe(true);
    expect(commands.some((command) => command.title === "Clear current profile data")).toBe(true);
    expect(commands.some((command) => command.title === "Clear history")).toBe(true);
    expect(commands.some((command) => command.title === "Delete workspace")).toBe(true);
    expect(commands.some((command) => command.title === "New workspace")).toBe(true);
    expect(commands.some((command) => command.title === "Close other tabs")).toBe(true);
    expect(commands.some((command) => command.title === "Close tabs to the left")).toBe(true);
    expect(commands.some((command) => command.title === "Close tabs to the right")).toBe(true);
  });
});
