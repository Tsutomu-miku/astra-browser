import { useCallback, useEffect, useMemo, useRef } from "react";

import { getActiveTab, getActiveWorkspace } from "../domain/selectors";
import { useBrowserStore } from "../stores/browserStore";
import type { WebviewElement } from "../types/browser-ui";
import { useBrowserEffects } from "./useBrowserEffects";
import { buildCommands } from "./useCommands";
import type { ShortcutIntent } from "./keyboardShortcuts";

export function useBrowserController() {
  const store = useBrowserStore();
  const webviews = useRef(new Map<string, WebviewElement>());
  const activeWorkspace = getActiveWorkspace(store.state);
  const activeTab = getActiveTab(activeWorkspace);
  const activeWebview = webviews.current.get(activeTab.id);

  const focusAddressBar = useCallback(() => {
    store.setCommandOpen(false);
    const input = document.getElementById("addressInput") as HTMLInputElement | null;
    input?.focus();
    input?.select();
  }, [store]);

  const actions = useMemo(() => ({
    addWorkspace: store.addWorkspace,
    assignTabToGroup: store.assignTabToGroup,
    clearBrowsingData: store.clearBrowsingData,
    clearHistory: store.clearHistory,
    clearWorkspaceBrowsingData: store.clearWorkspaceBrowsingData,
    closeActiveTab: store.closeActiveTab,
    closeTab: store.closeTab,
    clearSitePermission: store.clearSitePermission,
    duplicateActiveTab: store.duplicateActiveTab,
    groupActiveTab: store.groupActiveTab,
    closeOtherTabs: store.closeOtherTabs,
    closeTabsToLeft: store.closeTabsToLeft,
    closeTabsToRight: store.closeTabsToRight,
    deleteWorkspace: store.deleteWorkspace,
    focusAddressBar,
    duplicateTab: store.duplicateTab,
    closeFind: () => {
      activeWebview?.stopFindInPage?.("clearSelection");
      store.setFindOpen(false);
      store.setFindQuery("");
    },
    findInPage: (query: string, forward = true) => {
      store.setFindQuery(query);
      if (query.trim()) {
        activeWebview?.findInPage?.(query, { findNext: true, forward });
      }
    },
    moveTabToWorkspace: store.moveTabToWorkspace,
    openTabInSplit: store.openTabInSplit,
    navigateActiveTab: (url: string) => store.navigateActiveTab(url, activeWebview),
    newTab: store.newTab,
    openUrlInActiveWorkspace: store.openUrlInActiveWorkspace,
    recordHistory: store.recordHistory,
    removeHistoryEntry: store.removeHistoryEntry,
    replaceBrowserState: store.replaceBrowserState,
    reorderWorkspace: store.reorderWorkspace,
    reorderTab: store.reorderTab,
    resetActiveTabZoom: () => store.resetActiveTabZoom(activeWebview),
    resolvePermissionRequest: store.resolvePermissionRequest,
    restoreClosedTab: store.restoreClosedTab,
    restoreLastClosedTab: store.restoreLastClosedTab,
    runWebviewAction: (action: Parameters<typeof store.runWebviewAction>[0]) => store.runWebviewAction(action, activeWebview),
    selectAdjacentTab: store.selectAdjacentTab,
    selectTab: store.selectTab,
    sleepInactiveTabs: store.sleepInactiveTabs,
    sleepTab: store.sleepTab,
    setActiveTabZoom: (zoomFactor: number) => store.setActiveTabZoom(zoomFactor, activeWebview),
    setSitePermission: store.setSitePermission,
    switchWorkspace: store.switchWorkspace,
    toggleActiveTabFavorite: store.toggleActiveTabFavorite,
    toggleActiveTabMuted: () => store.toggleActiveTabMuted(activeWebview),
    toggleActiveTabPinned: store.toggleActiveTabPinned,
    toggleTabGroupCollapsed: store.toggleTabGroupCollapsed,
    toggleTabMuted: (tabId: string) => store.toggleTabMuted(tabId, webviews.current.get(tabId)),
    toggleTabPinned: store.toggleTabPinned,
    toggleSidebar: store.toggleSidebar,
    toggleSplitMode: store.toggleSplitMode,
    ungroupActiveTab: store.ungroupActiveTab,
    updateSettings: store.updateSettings,
    updateTabGroup: store.updateTabGroup,
    updateTab: store.updateTab,
    updateWorkspace: store.updateWorkspace,
    zoomIn: () => store.zoomIn(activeWebview),
    zoomOut: () => store.zoomOut(activeWebview)
  }), [activeWebview, focusAddressBar, store]);
  const commands = useMemo(() => buildCommands(store.state, actions, store.setPanel), [actions, store]);

  const handleShortcut = useCallback((intent: ShortcutIntent) => {
    if (intent.type === "openCommand") {
      store.setCommandOpen(true);
      store.setCommandQuery("");
    } else if (intent.type === "openFind") {
      store.setFindOpen(true);
    } else if (intent.type === "focusAddress") {
      focusAddressBar();
    } else if (intent.type === "closePanels") {
      store.setCommandOpen(false);
      store.setFindOpen(false);
      store.setPanel(null);
    } else if (intent.type === "selectWorkspaceIndex") {
      const workspace = store.state.workspaces[intent.index];
      if (workspace) store.switchWorkspace(workspace.id);
    } else if (intent.type === "selectTabIndex") {
      const tab = activeWorkspace.tabs[intent.index];
      if (tab) store.selectTab(tab.id);
    } else if (intent.type === "selectAdjacentTab") {
      store.selectAdjacentTab(intent.direction);
    } else if (intent.type === "zoomIn") {
      store.zoomIn(activeWebview);
    } else if (intent.type === "zoomOut") {
      store.zoomOut(activeWebview);
    } else if (intent.type === "resetZoom") {
      store.resetActiveTabZoom(activeWebview);
    } else {
      shortcutActions[intent.type]?.();
    }
  }, [activeWorkspace.tabs, focusAddressBar, store]);

  useEffect(() => {
    store.setAddressValue(activeTab.url);
  }, [activeTab.url, store]);

  useBrowserEffects({
    ingestDownload: store.ingestDownload,
    ingestPermissionRequest: store.ingestPermissionRequest,
    onShortcut: handleShortcut,
    sitePermissions: store.state.sitePermissions,
    workspaces: store.state.workspaces
  });

  return {
    actions,
    activeTab,
    activeWebview,
    activeWorkspace,
    addressValue: store.addressValue,
    commands,
    commandOpen: store.commandOpen,
    commandQuery: store.commandQuery,
    findOpen: store.findOpen,
    findQuery: store.findQuery,
    panel: store.panel,
    permissionRequest: store.permissionRequest,
    setAddressValue: store.setAddressValue,
    setCommandOpen: store.setCommandOpen,
    setCommandQuery: store.setCommandQuery,
    setFindOpen: store.setFindOpen,
    setFindQuery: store.setFindQuery,
    setPanel: store.setPanel,
    sidebarCollapsed: store.sidebarCollapsed,
    state: store.state,
    webviews
  };
}

const shortcutActions: Partial<Record<ShortcutIntent["type"], () => void>> = {
  closeTab: () => useBrowserStore.getState().closeActiveTab(),
  newTab: () => useBrowserStore.getState().newTab(),
  restoreClosedTab: () => useBrowserStore.getState().restoreLastClosedTab(),
  toggleSidebar: () => useBrowserStore.getState().toggleSidebar(),
  toggleSplit: () => useBrowserStore.getState().toggleSplitMode()
};
