import { describe, expect, it, vi } from "vitest";

import { closeActiveTab, openUrlInActiveWorkspace, toggleActiveTabFavorite, toggleSplitMode } from "../src/renderer/domain/actions";
import { createDefaultState } from "../src/renderer/domain/browser";
import { buildCommands } from "../src/renderer/app/controller/useCommands";
import type { CommandChromeState } from "../src/renderer/app/controller/useCommands";

const defaultChromeState: CommandChromeState = {
  compactMode: false,
  floatingSidebarOpen: false,
  floatingToolbarOpen: false,
  sidebarCollapsed: false
};

function commandActions() {
  return {
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
    fillSplitView: vi.fn(),
    focusAddressBar: vi.fn(),
    groupActiveTab: vi.fn(),
    groupTab: vi.fn(),
    moveTabToWorkspace: vi.fn(),
    openGlance: vi.fn(),
    peekCompactChrome: vi.fn(),
    openTabInSplit: vi.fn(),
    openUrlInSplit: vi.fn(),
    newTab: vi.fn(),
    openUrlInActiveWorkspace: vi.fn(),
    restoreClosedTab: vi.fn(),
    restoreLastClosedTab: vi.fn(),
    selectAdjacentTab: vi.fn(),
    resetActiveTabZoom: vi.fn(),
    selectTab: vi.fn(),
    setSplitLayout: vi.fn(),
    sleepInactiveTabs: vi.fn(),
    switchWorkspace: vi.fn(),
    toggleActiveTabFavorite: vi.fn(),
    toggleActiveTabEssential: vi.fn(),
    toggleActiveTabMuted: vi.fn(),
    toggleActiveTabPinned: vi.fn(),
    toggleCompactMode: vi.fn(),
    toggleFloatingSidebar: vi.fn(),
    toggleFloatingToolbar: vi.fn(),
    toggleApplicationDevTools: vi.fn(),
    toggleTabGroupCollapsed: vi.fn(),
    toggleSidebar: vi.fn(),
    toggleSplitMode: vi.fn(),
    ungroupActiveTab: vi.fn(),
    ungroupTab: vi.fn(),
    zoomIn: vi.fn(),
    zoomOut: vi.fn()
  };
}

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

    const commands = buildCommands(withSplitCandidate, commandActions(), vi.fn(), defaultChromeState);

    expect(commands.some((command) => command.title === "Reopen closed tab")).toBe(true);
    expect(commands.some((command) => command.subtitle.startsWith("Open tab"))).toBe(true);
    expect(commands.some((command) => command.subtitle.startsWith("Essential"))).toBe(true);
    expect(commands.some((command) => command.subtitle.startsWith("Favorite"))).toBe(true);
    expect(commands.some((command) => command.subtitle.startsWith("Recently closed"))).toBe(true);
    expect(commands.some((command) => command.subtitle.startsWith("History"))).toBe(true);
    expect(commands.some((command) => command.title === "Collapse sidebar")).toBe(true);
    expect(commands.find((command) => command.title === "Collapse sidebar")?.shortcut).toBe("Alt+B");
    expect(commands.some((command) => command.title === "Reset zoom")).toBe(true);
    expect(commands.find((command) => command.title === "Reset zoom")?.shortcut).toBe("Ctrl/Cmd+0");
    expect(commands.some((command) => command.title === "Sleep inactive tabs")).toBe(true);
    expect(commands.some((command) => command.title === "Duplicate tab")).toBe(true);
    expect(commands.some((command) => command.title === "Group tab")).toBe(true);
    expect(commands.some((command) => command.title === "Next tab")).toBe(true);
    expect(commands.some((command) => command.title === "Previous tab")).toBe(true);
    expect(commands.some((command) => command.title === "Mute tab")).toBe(true);
    expect(commands.some((command) => command.title === "Preview tab in Glance")).toBe(true);
    expect(commands.some((command) => command.title.startsWith("Move tab to"))).toBe(true);
    expect(commands.some((command) => command.title.includes("in split view"))).toBe(true);
    expect(commands.some((command) => command.title === "Fill split grid")).toBe(true);
    expect(commands.some((command) => command.title === "Unsplit all tabs")).toBe(false);
    expect(commands.some((command) => command.title === "Split layout horizontal")).toBe(true);
    expect(commands.some((command) => command.title === "Split layout vertical")).toBe(true);
    expect(commands.some((command) => command.title === "Split layout grid")).toBe(true);
    expect(commands.some((command) => command.title === "Clear browsing data")).toBe(true);
    expect(commands.some((command) => command.title === "Clear current profile data")).toBe(true);
    expect(commands.some((command) => command.title === "Clear history")).toBe(true);
    expect(commands.find((command) => command.title === "Toggle application DevTools")?.shortcut).toBe("F12 / Ctrl/Cmd+Shift+I");
    expect(commands.some((command) => command.title === "Delete workspace")).toBe(true);
    expect(commands.some((command) => command.title === "New workspace")).toBe(true);
    expect(commands.some((command) => command.title === "Enter compact mode")).toBe(true);
    expect(commands.some((command) => command.title === "Peek floating toolbar")).toBe(false);
    expect(commands.some((command) => command.title === "Peek floating sidebar")).toBe(false);
    expect(commands.some((command) => command.title === "Pin floating sidebar")).toBe(true);
    expect(commands.some((command) => command.title === "Pin floating toolbar")).toBe(true);
    expect(commands.find((command) => command.title === "Enter compact mode")?.shortcut).toBe("Ctrl/Cmd+Alt+C");
    expect(commands.some((command) => command.title === "Add essential")).toBe(true);
    expect(commands.some((command) => command.title === "Close other tabs")).toBe(true);
    expect(commands.some((command) => command.title === "Close tabs to the left")).toBe(true);
    expect(commands.some((command) => command.title === "Close tabs to the right")).toBe(true);
  });

  it("only shows unsplit all when split view is active", () => {
    const withSecondTab = openUrlInActiveWorkspace(createDefaultState(), "example.com", "Example");
    const splitState = toggleSplitMode(withSecondTab);
    const commands = buildCommands(splitState, commandActions(), vi.fn(), defaultChromeState);

    expect(commands.some((command) => command.title === "Close split view")).toBe(true);
    expect(commands.some((command) => command.title === "Unsplit all tabs")).toBe(true);
  });

  it("labels compact chrome commands by current state", () => {
    const commands = buildCommands(createDefaultState(), commandActions(), vi.fn(), {
      compactMode: true,
      floatingSidebarOpen: true,
      floatingToolbarOpen: true,
      sidebarCollapsed: true
    });

    expect(commands.some((command) => command.title === "Expand sidebar")).toBe(true);
    expect(commands.some((command) => command.title === "Exit compact mode")).toBe(true);
    expect(commands.some((command) => command.title === "Peek floating toolbar")).toBe(true);
    expect(commands.some((command) => command.title === "Peek floating sidebar")).toBe(true);
    expect(commands.some((command) => command.title === "Unpin floating sidebar")).toBe(true);
    expect(commands.some((command) => command.title === "Unpin floating toolbar")).toBe(true);
  });
});
