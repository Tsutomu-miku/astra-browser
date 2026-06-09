import { describe, expect, it, vi } from "vitest";

import { closeActiveTab, openUrlInActiveWorkspace, sleepInactiveTabs, toggleActiveTabFavorite, toggleSplitMode } from "../src/renderer/domain/actions";
import { createDefaultState } from "../src/renderer/domain/browser";
import { buildCommands } from "../src/renderer/app/controller/useCommands";
import { getCommandRunner } from "../src/renderer/surfaces/command/model/commandIntent";
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
    copyText: vi.fn(),
    deleteWorkspace: vi.fn(),
    duplicateActiveTab: vi.fn(),
    fillSplitView: vi.fn(),
    focusAddressBar: vi.fn(),
    focusSplitPane: vi.fn(),
    groupActiveTab: vi.fn(),
    groupTab: vi.fn(),
    moveTabToWorkspace: vi.fn(),
    navigateActiveTab: vi.fn(),
    openGlance: vi.fn(),
    openFind: vi.fn(),
    peekCompactChrome: vi.fn(),
    peekCompactSidebar: vi.fn(),
    peekCompactToolbar: vi.fn(),
    openTabInSplit: vi.fn(),
    openUrlInSplit: vi.fn(),
    newTab: vi.fn(),
    openUrlInActiveWorkspace: vi.fn(),
    restoreClosedTab: vi.fn(),
    restoreLastClosedTab: vi.fn(),
    runWebviewAction: vi.fn(),
    selectAdjacentTab: vi.fn(),
    resetActiveTabZoom: vi.fn(),
    selectTab: vi.fn(),
    setSplitLayout: vi.fn(),
    sleepInactiveTabs: vi.fn(),
    sleepTab: vi.fn(),
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
    updateSettings: vi.fn(),
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
    expect(commands.some((command) => command.title === "Disable Memory Saver")).toBe(true);
    expect(commands.some((command) => command.title === "Set Memory Saver to 15 minutes")).toBe(true);
    expect(commands.some((command) => command.title === "Sleep current tab")).toBe(true);
    expect(commands.some((command) => command.title === "Duplicate tab")).toBe(true);
    expect(commands.some((command) => command.title === "Group tab")).toBe(true);
    expect(commands.some((command) => command.title === "Next tab")).toBe(true);
    expect(commands.some((command) => command.title === "Previous tab")).toBe(true);
    expect(commands.some((command) => command.title === "Mute tab")).toBe(true);
    expect(commands.some((command) => command.title === "Preview tab in Glance")).toBe(true);
    expect(commands.some((command) => command.title === "Copy current URL")).toBe(true);
    expect(commands.some((command) => command.title === "Copy current page title")).toBe(true);
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
    expect(commands.find((command) => command.title === "Find in page")?.shortcut).toBe("Ctrl/Cmd+F");
    expect(commands.some((command) => command.title === "Show site information")).toBe(true);
    expect(commands.find((command) => command.title === "Reload page")?.shortcut).toBe("Ctrl/Cmd+R");
    expect(commands.find((command) => command.title === "Hard reload")?.shortcut).toBe("Ctrl/Cmd+Shift+R");
    expect(commands.some((command) => command.title === "Add essential")).toBe(true);
    expect(commands.some((command) => command.title === "Close other tabs")).toBe(true);
    expect(commands.some((command) => command.title === "Close tabs to the left")).toBe(true);
    expect(commands.some((command) => command.title === "Close tabs to the right")).toBe(true);
  });

  it("only shows unsplit all when split view is active", () => {
    const withSecondTab = openUrlInActiveWorkspace(createDefaultState(), "example.com", "Example");
    const actions = commandActions();
    const splitState = toggleSplitMode(withSecondTab);
    const commands = buildCommands(splitState, actions, vi.fn(), defaultChromeState);
    const splitTabId = splitState.splitTabIds[0];

    expect(commands.some((command) => command.title === "Close split view")).toBe(true);
    expect(commands.some((command) => command.title === "Unsplit all tabs")).toBe(true);
    expect(commands.some((command) => command.title === "Focus New Tab split pane")).toBe(true);

    commands.find((command) => command.title === "Focus New Tab split pane")?.run();

    expect(actions.focusSplitPane).toHaveBeenCalledWith(splitTabId);
  });

  it("previews and split-opens recently closed entries from command palette modifiers", () => {
    const actions = commandActions();
    const state = closeActiveTab(openUrlInActiveWorkspace(createDefaultState(), "example.com", "Example"));
    const commands = buildCommands(state, actions, vi.fn(), defaultChromeState);
    const recentlyClosedCommand = commands.find((command) => command.title === "Reopen Example")!;

    getCommandRunner(recentlyClosedCommand, { altKey: true, shiftKey: false })();
    getCommandRunner(recentlyClosedCommand, { altKey: false, shiftKey: true })();
    getCommandRunner(recentlyClosedCommand, { altKey: false, shiftKey: false })();

    expect(actions.openGlance).toHaveBeenCalledWith("https://example.com/", "Example");
    expect(actions.openUrlInSplit).toHaveBeenCalledWith("https://example.com/", "Example");
    expect(actions.restoreClosedTab).toHaveBeenCalledWith(0);
  });

  it("labels compact chrome commands by current state", () => {
    const actions = commandActions();
    const commands = buildCommands(createDefaultState(), actions, vi.fn(), {
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

    commands.find((command) => command.title === "Peek floating toolbar")?.run();
    commands.find((command) => command.title === "Peek floating sidebar")?.run();

    expect(actions.peekCompactToolbar).toHaveBeenCalled();
    expect(actions.peekCompactSidebar).toHaveBeenCalled();
    expect(actions.peekCompactChrome).not.toHaveBeenCalled();
  });

  it("copies current page values from command palette actions", () => {
    const actions = commandActions();
    const state = openUrlInActiveWorkspace(createDefaultState(), "example.com", "Example");
    const commands = buildCommands(state, actions, vi.fn(), defaultChromeState);

    commands.find((command) => command.title === "Copy current URL")?.run();
    commands.find((command) => command.title === "Copy current page title")?.run();

    expect(actions.copyText).toHaveBeenCalledWith("https://example.com/");
    expect(actions.copyText).toHaveBeenCalledWith("Example");
  });

  it("opens page tools from command palette actions", () => {
    const actions = commandActions();
    const setPanel = vi.fn();
    const commands = buildCommands(createDefaultState(), actions, setPanel, defaultChromeState);

    commands.find((command) => command.title === "Find in page")?.run();
    commands.find((command) => command.title === "Show site information")?.run();

    expect(actions.openFind).toHaveBeenCalled();
    expect(setPanel).toHaveBeenCalledWith("site");
  });

  it("runs navigation commands from command palette actions", () => {
    const actions = commandActions();
    const state = openUrlInActiveWorkspace(createDefaultState(), "example.com", "Example");
    const activeTab = state.workspaces[0].tabs.find((tab) => tab.id === state.workspaces[0].activeTabId)!;
    activeTab.canGoBack = true;
    activeTab.canGoForward = true;
    const commands = buildCommands(state, actions, vi.fn(), defaultChromeState);

    commands.find((command) => command.title === "Back")?.run();
    commands.find((command) => command.title === "Forward")?.run();
    commands.find((command) => command.title === "Reload page")?.run();
    commands.find((command) => command.title === "Hard reload")?.run();

    expect(commands.find((command) => command.title === "Back")?.shortcut).toBe("Alt+Left / Ctrl/Cmd+[");
    expect(commands.find((command) => command.title === "Forward")?.shortcut).toBe("Alt+Right / Ctrl/Cmd+]");
    expect(actions.runWebviewAction).toHaveBeenCalledWith("goBack");
    expect(actions.runWebviewAction).toHaveBeenCalledWith("goForward");
    expect(actions.runWebviewAction).toHaveBeenCalledWith("reload");
    expect(actions.runWebviewAction).toHaveBeenCalledWith("reloadIgnoringCache");
  });

  it("turns the reload command into stop loading while the active tab loads", () => {
    const actions = commandActions();
    const state = openUrlInActiveWorkspace(createDefaultState(), "example.com", "Example");
    const activeTab = state.workspaces[0].tabs.find((tab) => tab.id === state.workspaces[0].activeTabId)!;
    activeTab.isLoading = true;
    const commands = buildCommands(state, actions, vi.fn(), defaultChromeState);

    commands.find((command) => command.title === "Stop loading")?.run();

    expect(commands.some((command) => command.title === "Reload page")).toBe(false);
    expect(actions.runWebviewAction).toHaveBeenCalledWith("stop");
  });

  it("sleeps the active tab from command palette actions", () => {
    const actions = commandActions();
    const state = openUrlInActiveWorkspace(createDefaultState(), "example.com", "Example");
    const activeTabId = state.workspaces[0].activeTabId;
    const commands = buildCommands(state, actions, vi.fn(), defaultChromeState);

    commands.find((command) => command.title === "Sleep current tab")?.run();

    expect(actions.sleepTab).toHaveBeenCalledWith(activeTabId);
  });

  it("summarizes memory saver state for sleep inactive tab commands", () => {
    const state = openUrlInActiveWorkspace(createDefaultState(), "example.com", "Example");
    const commands = buildCommands(state, commandActions(), vi.fn(), defaultChromeState);

    expect(commands.find((command) => command.title === "Sleep inactive tabs")?.subtitle).toBe("3 releasable · 0 sleeping · 1 protected");
  });

  it("toggles automatic Memory Saver from command palette actions", () => {
    const actions = commandActions();
    const state = createDefaultState();
    const commands = buildCommands(state, actions, vi.fn(), defaultChromeState);

    commands.find((command) => command.title === "Disable Memory Saver")?.run();

    state.settings.memorySaverEnabled = false;
    const disabledCommands = buildCommands(state, actions, vi.fn(), defaultChromeState);
    disabledCommands.find((command) => command.title === "Enable Memory Saver")?.run();

    expect(commands.find((command) => command.title === "Disable Memory Saver")?.subtitle).toBe("Auto-sleep idle tabs after 30 minutes");
    expect(disabledCommands.find((command) => command.title === "Enable Memory Saver")?.subtitle).toBe("Keep background tabs awake until manually slept");
    expect(actions.updateSettings).toHaveBeenCalledWith({ memorySaverEnabled: false });
    expect(actions.updateSettings).toHaveBeenCalledWith({ memorySaverEnabled: true });
  });

  it("sets Memory Saver delay from command palette actions", () => {
    const actions = commandActions();
    const state = createDefaultState();
    state.settings.memorySaverEnabled = false;
    const commands = buildCommands(state, actions, vi.fn(), defaultChromeState);

    commands.find((command) => command.title === "Set Memory Saver to 15 minutes")?.run();

    expect(commands.find((command) => command.title === "Set Memory Saver to 15 minutes")?.subtitle).toBe("Also enables automatic idle tab sleeping");
    expect(actions.updateSettings).toHaveBeenCalledWith({
      memorySaverEnabled: true,
      memorySaverIdleMinutes: 15
    });
  });

  it("omits the current Memory Saver delay command", () => {
    const state = createDefaultState();
    state.settings.memorySaverIdleMinutes = 15;
    const commands = buildCommands(state, commandActions(), vi.fn(), defaultChromeState);

    expect(commands.some((command) => command.title === "Set Memory Saver to 15 minutes")).toBe(false);
    expect(commands.find((command) => command.title === "Set Memory Saver to 30 minutes")?.subtitle).toBe("Auto-sleep idle tabs after 30 minutes");
  });

  it("hides the current tab sleep command when focus has nowhere to move", () => {
    const state = createDefaultState();
    const workspace = state.workspaces[0];
    workspace.tabs = workspace.tabs.filter((tab) => !tab.isFavorite);
    workspace.favoriteOrder = [];
    const commands = buildCommands(state, commandActions(), vi.fn(), defaultChromeState);

    expect(commands.some((command) => command.title === "Sleep current tab")).toBe(false);
  });

  it("labels sleeping tabs in command palette open-tab entries", () => {
    const actions = commandActions();
    const first = openUrlInActiveWorkspace(createDefaultState(), "first.test", "First");
    const second = openUrlInActiveWorkspace(first, "second.test", "Second");
    const slept = sleepInactiveTabs(second);
    const sleepingTab = slept.workspaces[0].tabs.find((tab) => tab.title === "First")!;
    const commands = buildCommands(slept, actions, vi.fn(), defaultChromeState);
    const sleepingCommand = commands.find((command) => command.title === "First");

    expect(sleepingCommand?.subtitle).toBe("Sleeping tab · https://first.test/");

    sleepingCommand?.run();

    expect(actions.selectTab).toHaveBeenCalledWith(sleepingTab.id);
  });

  it("labels the active tab in command palette open-tab entries", () => {
    const state = openUrlInActiveWorkspace(createDefaultState(), "example.com", "Example");
    const commands = buildCommands(state, commandActions(), vi.fn(), defaultChromeState);
    const activeTabCommand = commands.find((command) => command.title === "Example");

    expect(activeTabCommand?.subtitle).toBe("Active tab · https://example.com/");
  });
});
