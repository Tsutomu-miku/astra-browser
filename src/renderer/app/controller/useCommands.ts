import { shortcutLabels } from "../../common/shortcuts/shortcutLabels";
import { BrowserState, isEssential, isFavorite } from "../../domain/browser";
import { getActiveTab, getActiveWorkspace } from "../../domain/browser/selectors";
import type { Panel } from "../../stores/browserStoreTypes";
import { buildContentCommands } from "../../surfaces/command/model/commandContentEntries";
import { buildSplitCommands } from "../../surfaces/command/model/commandSplitEntries";
import type { Command, CommandActions } from "../../surfaces/command/model/commandTypes";

export interface CommandChromeState {
  compactMode: boolean;
  floatingSidebarOpen: boolean;
  floatingToolbarOpen: boolean;
  sidebarCollapsed: boolean;
}

export function buildCommands(
  state: BrowserState,
  actions: CommandActions,
  setPanel: (panel: Panel) => void,
  chromeState: CommandChromeState
): Command[] {
  const workspace = getActiveWorkspace(state);
  const activeTab = getActiveTab(workspace);
  const sidebarCommandTitle = chromeState.sidebarCollapsed || chromeState.compactMode
    ? "Expand sidebar"
    : "Collapse sidebar";
  const compactModeCommandTitle = chromeState.compactMode ? "Exit compact mode" : "Enter compact mode";
  const floatingSidebarCommandTitle = chromeState.floatingSidebarOpen
    ? "Unpin floating sidebar"
    : "Pin floating sidebar";
  const floatingToolbarCommandTitle = chromeState.floatingToolbarOpen
    ? "Unpin floating toolbar"
    : "Pin floating toolbar";
  const workspaceCommands = state.workspaces.map((candidate) => ({
    title: `Switch to ${candidate.name}`,
    subtitle: "Workspace",
    run: () => actions.switchWorkspace(candidate.id)
  }));
  const moveTabCommands = state.workspaces
    .filter((candidate) => candidate.id !== workspace.id)
    .map((candidate) => ({
      title: `Move tab to ${candidate.name}`,
      subtitle: activeTab.title || activeTab.url,
      run: () => actions.moveTabToWorkspace(activeTab.id, candidate.id)
    }));
  const contentCommands = buildContentCommands(state, workspace, actions);
  const splitCommands = buildSplitCommands(state, workspace, activeTab, actions);
  const tabGroupCommands = workspace.tabGroups.map((group) => ({
    title: group.isCollapsed ? `Expand ${group.name}` : `Collapse ${group.name}`,
    subtitle: "Tab group",
    run: () => actions.toggleTabGroupCollapsed(group.id)
  }));
  const moveToGroupCommands = activeTab.groupId
    ? []
    : workspace.tabGroups.map((group) => ({
      title: `Move tab to ${group.name}`,
      subtitle: "Tab group",
      run: () => actions.assignTabToGroup(activeTab.id, group.id)
    }));
  const workspaceDeleteCommands = state.workspaces.length > 1
    ? [{
      title: "Delete workspace",
      subtitle: workspace.name,
      run: () => actions.deleteWorkspace(workspace.id)
    }]
    : [];
  const compactChromePeekCommands = chromeState.compactMode
    ? [
      {
        title: "Peek floating toolbar",
        subtitle: "Temporarily reveal compact browser controls",
        run: actions.peekCompactChrome
      },
      {
        title: "Peek floating sidebar",
        subtitle: "Temporarily reveal compact tabs and Spaces",
        run: actions.peekCompactChrome
      }
    ]
    : [];

  return [
    {
      title: "New tab",
      subtitle: "Open homepage in this workspace",
      shortcut: shortcutLabels.newTab,
      run: actions.newTab
    },
    {
      title: isFavorite(workspace, activeTab.url) ? "Remove favorite" : "Add favorite",
      subtitle: activeTab.url,
      shortcut: shortcutLabels.favorite,
      run: actions.toggleActiveTabFavorite
    },
    {
      title: isEssential(state, activeTab.url) ? "Remove essential" : "Add essential",
      subtitle: "Show this page across Spaces",
      run: actions.toggleActiveTabEssential
    },
    {
      title: "Reopen closed tab",
      subtitle: workspace.closedTabs[0]?.title ?? "No closed tabs in this workspace",
      shortcut: shortcutLabels.restoreClosedTab,
      run: actions.restoreLastClosedTab
    },
    {
      title: activeTab.isPinned ? "Unpin tab" : "Pin tab",
      subtitle: activeTab.url,
      run: actions.toggleActiveTabPinned
    },
    {
      title: activeTab.isMuted ? "Unmute tab" : "Mute tab",
      subtitle: activeTab.url,
      shortcut: shortcutLabels.mute,
      run: actions.toggleActiveTabMuted
    },
    {
      title: "Preview tab in Glance",
      subtitle: activeTab.url,
      run: () => actions.openGlance(activeTab.url, activeTab.title)
    },
    ...splitCommands,
    {
      title: "Zoom in",
      subtitle: "Increase page zoom",
      shortcut: shortcutLabels.zoomIn,
      run: actions.zoomIn
    },
    {
      title: "Zoom out",
      subtitle: "Decrease page zoom",
      shortcut: shortcutLabels.zoomOut,
      run: actions.zoomOut
    },
    {
      title: "Reset zoom",
      subtitle: "Return page zoom to 100%",
      shortcut: shortcutLabels.resetZoom,
      run: actions.resetActiveTabZoom
    },
    { title: "Sleep inactive tabs", subtitle: "Unload hidden tabs in this Space", run: actions.sleepInactiveTabs },
    {
      title: sidebarCommandTitle,
      subtitle: chromeState.sidebarCollapsed || chromeState.compactMode ? "Restore sidebar controls" : "Enter focus mode",
      shortcut: shortcutLabels.toggleSidebar,
      run: actions.toggleSidebar
    },
    {
      title: compactModeCommandTitle,
      subtitle: chromeState.compactMode ? "Restore toolbar and sidebar chrome" : "Hide toolbar and float browser chrome on hover",
      shortcut: shortcutLabels.toggleCompactMode,
      run: actions.toggleCompactMode
    },
    ...compactChromePeekCommands,
    {
      title: floatingSidebarCommandTitle,
      subtitle: chromeState.floatingSidebarOpen ? "Let the compact sidebar hide automatically" : "Keep the compact sidebar open",
      shortcut: shortcutLabels.toggleFloatingSidebar,
      run: actions.toggleFloatingSidebar
    },
    {
      title: floatingToolbarCommandTitle,
      subtitle: chromeState.floatingToolbarOpen ? "Let the compact toolbar hide automatically" : "Keep the compact toolbar open",
      shortcut: shortcutLabels.toggleFloatingToolbar,
      run: actions.toggleFloatingToolbar
    },
    {
      title: "Focus address bar",
      subtitle: "Search or navigate",
      shortcut: shortcutLabels.focusAddress,
      run: actions.focusAddressBar
    },
    {
      title: "Next tab",
      subtitle: "Select the next tab",
      shortcut: shortcutLabels.nextTab,
      run: () => actions.selectAdjacentTab(1)
    },
    {
      title: "Previous tab",
      subtitle: "Select the previous tab",
      shortcut: shortcutLabels.previousTab,
      run: () => actions.selectAdjacentTab(-1)
    },
    { title: "Duplicate tab", subtitle: activeTab.title || activeTab.url, run: actions.duplicateActiveTab },
    {
      title: activeTab.groupId ? "Ungroup tab" : "Group tab",
      subtitle: activeTab.title || activeTab.url,
      run: activeTab.groupId ? actions.ungroupActiveTab : actions.groupActiveTab
    },
    {
      title: "Show history",
      subtitle: "Open recent browsing",
      shortcut: shortcutLabels.history,
      run: () => setPanel("history")
    },
    {
      title: "Show downloads",
      subtitle: "Open Chromium downloads",
      shortcut: shortcutLabels.downloads,
      run: () => setPanel("downloads")
    },
    { title: "Show settings", subtitle: "Homepage, search, and workspace", run: () => setPanel("settings") },
    {
      title: "Toggle application DevTools",
      subtitle: "Inspect the Astra shell",
      shortcut: shortcutLabels.devTools,
      run: actions.toggleApplicationDevTools
    },
    { title: "New workspace", subtitle: "Create a new Space", run: actions.addWorkspace },
    ...workspaceDeleteCommands,
    { title: "Clear browsing data", subtitle: "History, downloads, permissions, cache, and storage", run: actions.clearBrowsingData },
    { title: "Clear current profile data", subtitle: workspace.profileName, run: () => actions.clearWorkspaceBrowsingData(workspace.id) },
    { title: "Clear history", subtitle: `${state.history.length} recent entries`, run: actions.clearHistory },
    { title: "Close other tabs", subtitle: activeTab.title || activeTab.url, run: actions.closeOtherTabs },
    { title: "Close tabs to the left", subtitle: activeTab.title || activeTab.url, run: actions.closeTabsToLeft },
    { title: "Close tabs to the right", subtitle: activeTab.title || activeTab.url, run: actions.closeTabsToRight },
    {
      title: "Close tab",
      subtitle: activeTab.title || activeTab.url,
      shortcut: shortcutLabels.closeTab,
      run: actions.closeActiveTab
    },
    ...moveTabCommands,
    ...tabGroupCommands,
    ...moveToGroupCommands,
    ...contentCommands,
    ...workspaceCommands
  ];
}
