import { BrowserState, isEssential, isFavorite } from "../../domain/browser";
import { getActiveTab, getActiveWorkspace } from "../../domain/browser/selectors";
import type { Panel } from "../../stores/browserStoreTypes";
import { buildContentCommands } from "../../surfaces/command/model/commandContentEntries";
import { buildSplitCommands } from "../../surfaces/command/model/commandSplitEntries";
import type { Command, CommandActions } from "../../surfaces/command/model/commandTypes";

export function buildCommands(
  state: BrowserState,
  actions: CommandActions,
  setPanel: (panel: Panel) => void
): Command[] {
  const workspace = getActiveWorkspace(state);
  const activeTab = getActiveTab(workspace);
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

  return [
    { title: "New tab", subtitle: "Open homepage in this workspace", run: actions.newTab },
    {
      title: isFavorite(workspace, activeTab.url) ? "Remove favorite" : "Add favorite",
      subtitle: activeTab.url,
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
      run: actions.toggleActiveTabMuted
    },
    {
      title: "Preview tab in Glance",
      subtitle: activeTab.url,
      run: () => actions.openGlance(activeTab.url, activeTab.title)
    },
    ...splitCommands,
    { title: "Zoom in", subtitle: "Increase page zoom", run: actions.zoomIn },
    { title: "Zoom out", subtitle: "Decrease page zoom", run: actions.zoomOut },
    { title: "Reset zoom", subtitle: "Return page zoom to 100%", run: actions.resetActiveTabZoom },
    { title: "Sleep inactive tabs", subtitle: "Unload hidden tabs in this Space", run: actions.sleepInactiveTabs },
    {
      title: "Toggle sidebar",
      subtitle: "Enter or leave focus mode",
      run: actions.toggleSidebar
    },
    {
      title: "Toggle compact mode",
      subtitle: "Hide toolbar and float browser chrome on hover",
      run: actions.toggleCompactMode
    },
    {
      title: "Toggle floating sidebar",
      subtitle: "Keep the compact sidebar open until toggled again",
      run: actions.toggleFloatingSidebar
    },
    {
      title: "Toggle floating toolbar",
      subtitle: "Keep the compact toolbar open until toggled again",
      run: actions.toggleFloatingToolbar
    },
    { title: "Focus address bar", subtitle: "Search or navigate", run: actions.focusAddressBar },
    { title: "Next tab", subtitle: "Select the next tab", run: () => actions.selectAdjacentTab(1) },
    { title: "Previous tab", subtitle: "Select the previous tab", run: () => actions.selectAdjacentTab(-1) },
    { title: "Duplicate tab", subtitle: activeTab.title || activeTab.url, run: actions.duplicateActiveTab },
    {
      title: activeTab.groupId ? "Ungroup tab" : "Group tab",
      subtitle: activeTab.title || activeTab.url,
      run: activeTab.groupId ? actions.ungroupActiveTab : actions.groupActiveTab
    },
    { title: "Show history", subtitle: "Open recent browsing", run: () => setPanel("history") },
    { title: "Show downloads", subtitle: "Open Chromium downloads", run: () => setPanel("downloads") },
    { title: "Show settings", subtitle: "Homepage, search, and workspace", run: () => setPanel("settings") },
    { title: "New workspace", subtitle: "Create a new Space", run: actions.addWorkspace },
    ...workspaceDeleteCommands,
    { title: "Clear browsing data", subtitle: "History, downloads, permissions, cache, and storage", run: actions.clearBrowsingData },
    { title: "Clear current profile data", subtitle: workspace.profileName, run: () => actions.clearWorkspaceBrowsingData(workspace.id) },
    { title: "Clear history", subtitle: `${state.history.length} recent entries`, run: actions.clearHistory },
    { title: "Close other tabs", subtitle: activeTab.title || activeTab.url, run: actions.closeOtherTabs },
    { title: "Close tabs to the left", subtitle: activeTab.title || activeTab.url, run: actions.closeTabsToLeft },
    { title: "Close tabs to the right", subtitle: activeTab.title || activeTab.url, run: actions.closeTabsToRight },
    { title: "Close tab", subtitle: activeTab.title || activeTab.url, run: actions.closeActiveTab },
    ...moveTabCommands,
    ...tabGroupCommands,
    ...moveToGroupCommands,
    ...contentCommands,
    ...workspaceCommands
  ];
}
