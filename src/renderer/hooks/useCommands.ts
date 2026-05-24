import { BrowserState, isFavorite } from "../domain/browser-core";
import { getActiveTab, getActiveWorkspace } from "../domain/selectors";
import type { Panel } from "../stores/browserStore";

export interface Command {
  title: string;
  subtitle: string;
  run: () => void;
}

interface CommandActions {
  addWorkspace: () => void;
  clearBrowsingData: () => void;
  clearHistory: () => void;
  clearWorkspaceBrowsingData: (workspaceId: string) => void;
  assignTabToGroup: (tabId: string, groupId: string) => void;
  closeActiveTab: () => void;
  closeOtherTabs: () => void;
  closeTabsToLeft: () => void;
  closeTabsToRight: () => void;
  deleteWorkspace: (workspaceId: string) => void;
  duplicateActiveTab: () => void;
  focusAddressBar: () => void;
  groupActiveTab: () => void;
  moveTabToWorkspace: (tabId: string, workspaceId: string) => void;
  openTabInSplit: (tabId: string) => void;
  newTab: () => void;
  openUrlInActiveWorkspace: (url: string, title?: string) => void;
  restoreLastClosedTab: () => void;
  selectAdjacentTab: (direction: 1 | -1) => void;
  selectTab: (tabId: string) => void;
  resetActiveTabZoom: () => void;
  sleepInactiveTabs: () => void;
  switchWorkspace: (workspaceId: string) => void;
  toggleActiveTabFavorite: () => void;
  toggleActiveTabMuted: () => void;
  toggleActiveTabPinned: () => void;
  toggleTabGroupCollapsed: (groupId: string) => void;
  toggleSidebar: () => void;
  toggleSplitMode: () => void;
  ungroupActiveTab: () => void;
  zoomIn: () => void;
  zoomOut: () => void;
}

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
  const historyCommands = state.history.slice(0, 10).map((entry) => ({
    title: entry.title,
    subtitle: `History · ${entry.url}`,
    run: () => actions.openUrlInActiveWorkspace(entry.url, entry.title)
  }));
  const tabCommands = workspace.tabs.map((tab) => ({
    title: tab.title || tab.url,
    subtitle: `Open tab · ${tab.url}`,
    run: () => actions.selectTab(tab.id)
  }));
  const splitTabCommands = workspace.tabs
    .filter((tab) => tab.id !== activeTab.id)
    .map((tab) => ({
      title: `Open ${tab.title || tab.url} in split view`,
      subtitle: tab.url,
      run: () => actions.openTabInSplit(tab.id)
    }));
  const favoriteCommands = workspace.favorites.map((favorite) => ({
    title: favorite.title,
    subtitle: `Favorite · ${favorite.url}`,
    run: () => actions.openUrlInActiveWorkspace(favorite.url, favorite.title)
  }));
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
      title: state.splitMode ? "Close split view" : "Open split view",
      subtitle: "Show two Chromium webviews side by side",
      run: actions.toggleSplitMode
    },
    { title: "Zoom in", subtitle: "Increase page zoom", run: actions.zoomIn },
    { title: "Zoom out", subtitle: "Decrease page zoom", run: actions.zoomOut },
    { title: "Reset zoom", subtitle: "Return page zoom to 100%", run: actions.resetActiveTabZoom },
    { title: "Sleep inactive tabs", subtitle: "Unload hidden tabs in this Space", run: actions.sleepInactiveTabs },
    {
      title: "Toggle sidebar",
      subtitle: "Enter or leave focus mode",
      run: actions.toggleSidebar
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
    ...splitTabCommands,
    ...tabCommands,
    ...favoriteCommands,
    ...workspaceCommands,
    ...historyCommands
  ];
}
