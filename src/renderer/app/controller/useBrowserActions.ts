import { useMemo } from "react";

import type { BrowserStore } from "../../stores/browserStoreTypes";
import type { WebviewAction, WebviewElement } from "../../types/browser-ui";

interface BrowserActionsOptions {
  activeWebview: WebviewElement | undefined;
  focusAddressBar: () => void;
  peekCompactChrome: () => void;
  store: BrowserStore;
  webviews: { current: Map<string, WebviewElement> };
}

export function useBrowserActions({
  activeWebview,
  focusAddressBar,
  peekCompactChrome,
  store,
  webviews
}: BrowserActionsOptions) {
  return useMemo(() => ({
    addWorkspace: store.addWorkspace,
    assignTabToGroup: store.assignTabToGroup,
    clearBrowsingData: store.clearBrowsingData,
    clearHistory: store.clearHistory,
    clearWorkspaceBrowsingData: store.clearWorkspaceBrowsingData,
    closeActiveTab: () => {
      store.closeActiveTab();
      peekCompactChrome();
    },
    closeTab: (tabId: string) => {
      store.closeTab(tabId);
      peekCompactChrome();
    },
    clearSitePermission: store.clearSitePermission,
    clearSitePermissionsForOrigin: store.clearSitePermissionsForOrigin,
    duplicateActiveTab: () => {
      store.duplicateActiveTab();
      peekCompactChrome();
    },
    fillSplitView: store.fillSplitView,
    groupActiveTab: store.groupActiveTab,
    groupTab: store.groupTab,
    closeOtherTabs: store.closeOtherTabs,
    closeTabGroup: (groupId: string) => {
      store.closeTabGroup(groupId);
      peekCompactChrome();
    },
    closeTabsToLeft: store.closeTabsToLeft,
    closeTabsToRight: store.closeTabsToRight,
    copyText: (text: string) => {
      void navigator.clipboard?.writeText(text);
    },
    deleteWorkspace: store.deleteWorkspace,
    focusAddressBar,
    peekCompactChrome,
    duplicateTab: (tabId: string) => {
      store.duplicateTab(tabId);
      peekCompactChrome();
    },
    duplicateTabGroup: (groupId: string) => {
      store.duplicateTabGroup(groupId);
      peekCompactChrome();
    },
    closeFind: () => {
      activeWebview?.stopFindInPage?.("clearSelection");
      store.setFindOpen(false);
      store.setFindQuery("");
      store.setFindResult(null);
    },
    findInPage: (query: string, forward = true) => {
      store.setFindQuery(query);
      if (query.trim()) {
        store.setFindResult(null);
        activeWebview?.findInPage?.(query, { findNext: true, forward });
      } else {
        store.setFindResult(null);
        activeWebview?.stopFindInPage?.("clearSelection");
      }
    },
    openFind: () => {
      store.setFindOpen(true);
      store.setFindResult(null);
    },
    moveTabToWorkspace: store.moveTabToWorkspace,
    moveTabGroupToWorkspace: (groupId: string, workspaceId: string) => {
      store.moveTabGroupToWorkspace(groupId, workspaceId);
      peekCompactChrome();
    },
    moveTabToNewWorkspace: (tabId: string) => {
      store.moveTabToNewWorkspace(tabId);
      peekCompactChrome();
    },
    moveTabGroupToNewWorkspace: (groupId: string) => {
      store.moveTabGroupToNewWorkspace(groupId);
      peekCompactChrome();
    },
    focusSplitPane: (tabId: string) => {
      store.focusSplitPane(tabId);
      peekCompactChrome();
    },
    closeGlance: store.closeGlance,
    openGlance: store.openGlance,
    openGlanceInSplit: store.openGlanceInSplit,
    openTabInSplit: (tabId: string) => {
      store.openTabInSplit(tabId);
      peekCompactChrome();
    },
    openUrlInSplit: (url: string, title?: string) => {
      store.openUrlInSplit(url, title);
      peekCompactChrome();
    },
    navigateActiveTab: (url: string) => store.navigateActiveTab(url, activeWebview),
    newTab: () => {
      store.newTab();
      peekCompactChrome();
    },
    openUrlInActiveWorkspace: (url: string, title?: string) => {
      store.openUrlInActiveWorkspace(url, title);
      peekCompactChrome();
    },
    recordHistory: store.recordHistory,
    removeEssential: store.removeEssential,
    removeHistoryEntry: store.removeHistoryEntry,
    removeWorkspaceFavorite: store.removeWorkspaceFavorite,
    removeTabFromSplit: store.removeTabFromSplit,
    replaceBrowserState: store.replaceBrowserState,
    reorderEssential: store.reorderEssential,
    reorderWorkspace: store.reorderWorkspace,
    reorderWorkspaceFavorite: store.reorderWorkspaceFavorite,
    reorderTab: store.reorderTab,
    unpinTabToRegularEnd: store.unpinTabToRegularEnd,
    unpinTabToRegularPosition: store.unpinTabToRegularPosition,
    resetActiveTabZoom: () => store.resetActiveTabZoom(activeWebview),
    resolvePermissionRequest: store.resolvePermissionRequest,
    restoreClosedTab: (closedIndex: number) => {
      store.restoreClosedTab(closedIndex);
      peekCompactChrome();
    },
    restoreClosedTabToWorkspace: (closedIndex: number, workspaceId: string) => {
      store.restoreClosedTabToWorkspace(closedIndex, workspaceId);
      peekCompactChrome();
    },
    restoreLastClosedTab: () => {
      store.restoreLastClosedTab();
      peekCompactChrome();
    },
    runWebviewAction: (action: WebviewAction) => store.runWebviewAction(action, activeWebview),
    selectAdjacentTab: (direction: 1 | -1) => {
      store.selectAdjacentTab(direction);
      peekCompactChrome();
    },
    selectTab: (tabId: string) => {
      store.selectTab(tabId);
      peekCompactChrome();
    },
    sleepInactiveTabs: store.sleepInactiveTabs,
    sleepTabGroup: store.sleepTabGroup,
    sleepTab: store.sleepTab,
    setActiveTabZoom: (zoomFactor: number) => store.setActiveTabZoom(zoomFactor, activeWebview),
    setSplitLayout: store.setSplitLayout,
    setSitePermission: store.setSitePermission,
    switchWorkspace: (workspaceId: string) => {
      store.switchWorkspace(workspaceId);
      peekCompactChrome();
    },
    toggleActiveTabFavorite: store.toggleActiveTabFavorite,
    toggleActiveTabEssential: store.toggleActiveTabEssential,
    toggleActiveTabMuted: () => store.toggleActiveTabMuted(activeWebview),
    toggleActiveTabPinned: store.toggleActiveTabPinned,
    toggleCompactMode: store.toggleCompactMode,
    toggleFloatingSidebar: store.toggleFloatingSidebar,
    toggleFloatingToolbar: store.toggleFloatingToolbar,
    toggleApplicationDevTools: () => {
      void window.astraShell?.toggleDevTools();
    },
    toggleTabGroupCollapsed: store.toggleTabGroupCollapsed,
    toggleTabEssential: store.toggleTabEssential,
    toggleTabFavorite: store.toggleTabFavorite,
    toggleTabMuted: (tabId: string) => store.toggleTabMuted(tabId, webviews.current.get(tabId)),
    toggleTabPinned: store.toggleTabPinned,
    toggleSidebar: store.toggleSidebar,
    toggleSplitMode: store.toggleSplitMode,
    ungroupActiveTab: store.ungroupActiveTab,
    ungroupTab: store.ungroupTab,
    ungroupTabGroup: store.ungroupTabGroup,
    updateSettings: store.updateSettings,
    updateTabGroup: store.updateTabGroup,
    updateTab: store.updateTab,
    updateWorkspaceById: store.updateWorkspaceById,
    updateWorkspace: store.updateWorkspace,
    zoomIn: () => store.zoomIn(activeWebview),
    zoomOut: () => store.zoomOut(activeWebview)
  }), [activeWebview, focusAddressBar, peekCompactChrome, store, webviews]);
}

export type BrowserActions = ReturnType<typeof useBrowserActions>;
