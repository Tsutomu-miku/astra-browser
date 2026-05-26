import { useCallback, useEffect, useMemo, useRef, useState } from "react";

import { getNumberShortcutTarget } from "../common/shortcuts/numberShortcutTargets";
import { getActiveTab, getActiveWorkspace } from "../domain/selectors";
import { useBrowserStore, type SplitLayout } from "../stores/browserStore";
import type { WebviewElement } from "../types/browser-ui";
import { useBrowserEffects } from "./useBrowserEffects";
import { buildCommands } from "./useCommands";
import type { ShortcutIntent } from "./keyboardShortcuts";

const COMPACT_CHROME_PEEK_MS = 1400;

export function useBrowserController() {
  const store = useBrowserStore();
  const webviews = useRef(new Map<string, WebviewElement>());
  const chromePeekTimeout = useRef<number | null>(null);
  const [compactChromePeeking, setCompactChromePeeking] = useState(false);
  const activeWorkspace = getActiveWorkspace(store.state);
  const activeTab = getActiveTab(activeWorkspace);
  const activeWebview = webviews.current.get(activeTab.id);

  const focusAddressBar = useCallback(() => {
    store.setCommandOpen(false);
    const sidebarInput = document.getElementById("sidebarAddressInput") as HTMLInputElement | null;
    const topbarInput = document.getElementById("addressInput") as HTMLInputElement | null;
    const input = store.compactMode && sidebarInput?.offsetParent ? sidebarInput : topbarInput;
    input?.focus();
    input?.select();
  }, [store]);

  const peekCompactChrome = useCallback(() => {
    if (!store.compactMode) return;

    if (chromePeekTimeout.current) {
      window.clearTimeout(chromePeekTimeout.current);
    }

    setCompactChromePeeking(true);
    chromePeekTimeout.current = window.setTimeout(() => {
      setCompactChromePeeking(false);
      chromePeekTimeout.current = null;
    }, COMPACT_CHROME_PEEK_MS);
  }, [store.compactMode]);

  useEffect(() => () => {
    if (chromePeekTimeout.current) {
      window.clearTimeout(chromePeekTimeout.current);
    }
  }, []);

  const actions = useMemo(() => ({
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
    duplicateActiveTab: () => {
      store.duplicateActiveTab();
      peekCompactChrome();
    },
    fillSplitView: store.fillSplitView,
    groupActiveTab: store.groupActiveTab,
    closeOtherTabs: store.closeOtherTabs,
    closeTabsToLeft: store.closeTabsToLeft,
    closeTabsToRight: store.closeTabsToRight,
    deleteWorkspace: store.deleteWorkspace,
    focusAddressBar,
    duplicateTab: (tabId: string) => {
      store.duplicateTab(tabId);
      peekCompactChrome();
    },
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
    removeHistoryEntry: store.removeHistoryEntry,
    removeTabFromSplit: store.removeTabFromSplit,
    replaceBrowserState: store.replaceBrowserState,
    reorderWorkspace: store.reorderWorkspace,
    reorderTab: store.reorderTab,
    resetActiveTabZoom: () => store.resetActiveTabZoom(activeWebview),
    resolvePermissionRequest: store.resolvePermissionRequest,
    restoreClosedTab: (closedIndex: number) => {
      store.restoreClosedTab(closedIndex);
      peekCompactChrome();
    },
    restoreLastClosedTab: () => {
      store.restoreLastClosedTab();
      peekCompactChrome();
    },
    runWebviewAction: (action: Parameters<typeof store.runWebviewAction>[0]) => store.runWebviewAction(action, activeWebview),
    selectAdjacentTab: (direction: 1 | -1) => {
      store.selectAdjacentTab(direction);
      peekCompactChrome();
    },
    selectTab: (tabId: string) => {
      store.selectTab(tabId);
      peekCompactChrome();
    },
    sleepInactiveTabs: store.sleepInactiveTabs,
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
  }), [activeWebview, focusAddressBar, peekCompactChrome, store]);
  const commands = useMemo(() => buildCommands(store.state, actions, store.setPanel), [actions, store]);

  const handleShortcut = useCallback((intent: ShortcutIntent) => {
    if (intent.type === "openCommand") {
      store.setCommandOpen(true);
      store.setCommandQuery("");
    } else if (intent.type === "openDownloads") {
      store.setPanel("downloads");
    } else if (intent.type === "openFind") {
      store.setFindOpen(true);
    } else if (intent.type === "findMatch") {
      if (store.findQuery.trim()) {
        actions.findInPage(store.findQuery, intent.direction > 0);
      } else {
        store.setFindOpen(true);
      }
    } else if (intent.type === "openHistory") {
      store.setPanel("history");
    } else if (intent.type === "focusAddress") {
      focusAddressBar();
    } else if (intent.type === "closePanels") {
      store.closeGlance();
      store.setCommandOpen(false);
      store.setFindOpen(false);
      store.setPanel(null);
    } else if (intent.type === "selectWorkspaceIndex") {
      const workspace = store.state.workspaces[intent.index];
      if (workspace) actions.switchWorkspace(workspace.id);
    } else if (intent.type === "selectAdjacentWorkspace") {
      const currentIndex = store.state.workspaces.findIndex((workspace) => workspace.id === store.state.activeWorkspaceId);
      const workspace = store.state.workspaces[getWrappedIndex(currentIndex, store.state.workspaces.length, intent.direction)];
      if (workspace) actions.switchWorkspace(workspace.id);
    } else if (intent.type === "selectTabIndex") {
      const target = getNumberShortcutTarget(store.state.essentials, activeWorkspace, intent.index);
      if (target?.type === "essential") actions.openUrlInActiveWorkspace(target.url, target.title);
      if (target?.type === "tab") actions.selectTab(target.tabId);
    } else if (intent.type === "selectLastTab") {
      const tab = activeWorkspace.tabs.at(-1);
      if (tab) actions.selectTab(tab.id);
    } else if (intent.type === "selectAdjacentTab") {
      actions.selectAdjacentTab(intent.direction);
    } else if (intent.type === "newTab") {
      actions.newTab();
    } else if (intent.type === "restoreClosedTab") {
      actions.restoreLastClosedTab();
    } else if (intent.type === "closeTab") {
      actions.closeActiveTab();
    } else if (intent.type === "goHome") {
      actions.navigateActiveTab(activeWorkspace.homepage);
    } else if (intent.type === "navigateHistory") {
      actions.runWebviewAction(intent.direction < 0 ? "goBack" : "goForward");
    } else if (intent.type === "reloadPage") {
      actions.runWebviewAction(intent.hard ? "reloadIgnoringCache" : "reload");
    } else if (intent.type === "toggleFavorite") {
      actions.toggleActiveTabFavorite();
    } else if (intent.type === "toggleMute") {
      actions.toggleActiveTabMuted();
    } else if (intent.type === "zoomIn") {
      store.zoomIn(activeWebview);
    } else if (intent.type === "zoomOut") {
      store.zoomOut(activeWebview);
    } else if (intent.type === "resetZoom") {
      store.resetActiveTabZoom(activeWebview);
    } else if (intent.type === "toggleSplitGrid") {
      store.setSplitLayout("grid");
      store.fillSplitView();
    } else if (intent.type === "toggleSplitHorizontal") {
      activateSplitLayout("horizontal");
    } else if (intent.type === "toggleSplitVertical") {
      activateSplitLayout("vertical");
    } else if (intent.type === "unsplitAll") {
      if (store.state.splitMode) store.toggleSplitMode();
    } else {
      shortcutActions[intent.type]?.();
    }
  }, [actions, activeWebview, activeWorkspace.tabs, focusAddressBar, store]);

  function activateSplitLayout(layout: SplitLayout) {
    store.setSplitLayout(layout);
    if (!store.state.splitMode) {
      store.toggleSplitMode();
    }
  }

  useEffect(() => {
    useBrowserStore.getState().setAddressValue(activeTab.url);
  }, [activeTab.url]);

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
    compactChromePeeking,
    compactMode: store.compactMode,
    findOpen: store.findOpen,
    findQuery: store.findQuery,
    floatingSidebarOpen: store.floatingSidebarOpen,
    floatingToolbarOpen: store.floatingToolbarOpen,
    glance: store.glance,
    panel: store.panel,
    permissionRequest: store.permissionRequest,
    setAddressValue: store.setAddressValue,
    setCommandOpen: store.setCommandOpen,
    setCommandQuery: store.setCommandQuery,
    setFindOpen: store.setFindOpen,
    setFindQuery: store.setFindQuery,
    setPanel: store.setPanel,
    sidebarCollapsed: store.sidebarCollapsed,
    splitLayout: store.splitLayout,
    state: store.state,
    webviews
  };
}

function getWrappedIndex(index: number, length: number, direction: 1 | -1): number {
  if (length <= 0) return 0;
  const currentIndex = index < 0 ? 0 : index;
  return (currentIndex + direction + length) % length;
}

const shortcutActions: Partial<Record<ShortcutIntent["type"], () => void>> = {
  closeTab: () => useBrowserStore.getState().closeActiveTab(),
  closePanels: () => {
    useBrowserStore.getState().closeGlance();
  },
  toggleCompactMode: () => useBrowserStore.getState().toggleCompactMode(),
  toggleFloatingSidebar: () => useBrowserStore.getState().toggleFloatingSidebar(),
  toggleFloatingToolbar: () => useBrowserStore.getState().toggleFloatingToolbar(),
  newTab: () => useBrowserStore.getState().newTab(),
  fillSplitGrid: () => useBrowserStore.getState().fillSplitView(),
  restoreClosedTab: () => useBrowserStore.getState().restoreLastClosedTab(),
  toggleSidebar: () => useBrowserStore.getState().toggleSidebar(),
  toggleSplit: () => useBrowserStore.getState().toggleSplitMode()
};
