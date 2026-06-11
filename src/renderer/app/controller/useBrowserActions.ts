/* eslint-disable max-lines */
import { useMemo } from "react";

import type { BrowserStore } from "../../stores/browserStoreTypes";
import type { WebviewAction, WebviewElement } from "../../types/browser-ui";

interface BrowserActionsOptions {
  activeWebview: WebviewElement | undefined;
  focusAddressBar: () => void;
  peekCompactChrome: () => void;
  peekCompactSidebar: () => void;
  peekCompactToolbar: () => void;
  store: BrowserStore;
  webviews: { current: Map<string, WebviewElement> };
}

export function useBrowserActions({
  activeWebview,
  focusAddressBar,
  peekCompactChrome,
  peekCompactSidebar,
  peekCompactToolbar,
  store,
  webviews
}: BrowserActionsOptions) {
  return useMemo(() => ({
    addTabToFavorites: store.addTabToFavorites,
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
    clearPerOriginZoom: store.clearPerOriginZoom,
    clearAllPerOriginZoomSettings: store.clearAllPerOriginZoomSettings,
    setPerOriginZoom: store.setPerOriginZoom,
    duplicateActiveTab: () => {
      store.duplicateActiveTab();
      peekCompactChrome();
    },
    fillSplitView: store.fillSplitView,
    groupActiveTab: store.groupActiveTab,
    groupTab: store.groupTab,
    groupTabsTogether: store.groupTabsTogether,
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
    peekCompactSidebar,
    peekCompactToolbar,
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
    moveWorkspaceFavoriteToWorkspace: (tabId: string, workspaceId: string) => {
      store.moveWorkspaceFavoriteToWorkspace(tabId, workspaceId);
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
    moveWorkspaceFavoriteToNewWorkspace: (tabId: string) => {
      store.moveWorkspaceFavoriteToNewWorkspace(tabId);
      peekCompactChrome();
    },
    restoreClosedTabToNewWorkspace: (closedIndex: number) => {
      store.restoreClosedTabToNewWorkspace(closedIndex);
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
    newTabInGroup: (groupId: string) => {
      store.newTabInGroup(groupId);
      peekCompactChrome();
    },
    openUrlInActiveWorkspace: (url: string, title?: string) => {
      store.openUrlInActiveWorkspace(url, title);
      peekCompactChrome();
    },
    moveTabToFolderEnd: store.moveTabToFolderEnd,
    moveTabToFolderPosition: store.moveTabToFolderPosition,
    moveTabToFavoritePosition: store.moveTabToFavoritePosition,
    recordHistory: store.recordHistory,
    removeEssential: store.removeEssential,
    removeHistoryEntry: store.removeHistoryEntry,
    removeWorkspaceFavorite: store.removeWorkspaceFavorite,
    removeTabFromSplit: store.removeTabFromSplit,
    replaceBrowserState: store.replaceBrowserState,
    reorderEssential: store.reorderEssential,
    reorderTabGroup: store.reorderTabGroup,
    reorderWorkspace: store.reorderWorkspace,
    reorderWorkspaceFavorite: store.reorderWorkspaceFavorite,
    reorderTab: store.reorderTab,
    resetActiveTabZoom: () => store.resetActiveTabZoom(activeWebview),
    printActiveTab: (options?: Record<string, unknown>) => {
      const webContentsId = activeWebview?.getWebContentsId?.();
      if (typeof webContentsId === "number") {
        return window.astraShell?.printWebview(webContentsId, options);
      }
      void activeWebview?.print?.(options);
      return undefined;
    },
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
    toggleActiveDevTools: () => store.toggleActiveDevTools(activeWebview),
    toggleCompactMode: store.toggleCompactMode,
    toggleFloatingSidebar: store.toggleFloatingSidebar,
    toggleFloatingToolbar: store.toggleFloatingToolbar,
    newIncognitoWindow: store.newIncognitoWindow,
    newGuestWindow: store.newGuestWindow,
    syncForceHttps: store.syncForceHttps,
    syncSafeBrowsing: store.syncSafeBrowsing,
    reportSafeBrowsingDecision: store.reportSafeBrowsingDecision,
    checkSafeBrowsingForNavigation: store.checkSafeBrowsingForNavigation,
    dismissSafeBrowsingAlert: store.dismissSafeBrowsingAlert,
    toggleActivePictureInPicture: () => {
      const id = activeWebview?.getWebContentsId?.();
      return store.toggleActivePictureInPicture(typeof id === "number" ? id : undefined);
    },
    syncMediaSessionOnTabSwitch: store.syncMediaSessionOnTabSwitch,
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
    updateReaderSettings: store.updateReaderSettings,
    updateTranslationSettings: store.updateTranslationSettings,
    upsertPassword: store.upsertPassword,
    removePassword: store.removePassword,
    touchPasswordUsed: store.touchPasswordUsed,
    rejectPasswordSavePrompt: store.rejectPasswordSavePrompt,
    acceptPasswordSavePrompt: store.acceptPasswordSavePrompt,
    unlockPasswordVault: store.unlockPasswordVault,
    lockPasswordVault: store.lockPasswordVault,
    decryptPassword: store.decryptPassword,
    upsertAddress: store.upsertAddress,
    removeAddress: store.removeAddress,
    upsertPaymentMethod: store.upsertPaymentMethod,
    removePaymentMethod: store.removePaymentMethod,
    importBookmarks: store.importBookmarks,
    cachePageHtml: store.cachePageHtml,
    openActiveTabReader: store.openActiveTabReader,
    cancelDownload: store.cancelDownload,
    removeDownload: store.removeDownload,
    updateTabGroup: store.updateTabGroup,
    updateTab: store.updateTab,
    updateWorkspaceById: store.updateWorkspaceById,
    updateWorkspace: store.updateWorkspace,
    /* ===== M2.1 Profiles / Extensions / Reset ===== */
    addProfile: store.addProfile,
    removeProfile: store.removeProfile,
    switchProfile: store.switchProfile,
    switchActiveProfile: store.switchActiveProfile,
    addExtension: store.addExtension,
    removeExtension: store.removeExtension,
    toggleExtensionEnabled: store.toggleExtensionEnabled,
    installMv3ExtensionFromFolder: store.installMv3ExtensionFromFolder,
    uninstallMv3Extension: store.uninstallMv3Extension,
    setMv3ExtensionEnabled: store.setMv3ExtensionEnabled,
    reloadInstalledExtensions: store.reloadInstalledExtensions,
    pickFolderAndInstallMv3Extension: store.pickFolderAndInstallMv3Extension,
    resetSettings: store.resetSettings,
    clearAllDownloads: store.clearAllDownloads,
    openUserDataFolder: store.openUserDataFolder,
    restartBrowser: store.restartBrowser,
    zoomIn: () => store.zoomIn(activeWebview),
    zoomOut: () => store.zoomOut(activeWebview),
    /* ===== M2.4 W-3 PWA install ===== */
    ingestPendingPwaInstallPrompt: store.ingestPendingPwaInstallPrompt,
    dismissPendingPwaInstallPrompt: store.dismissPendingPwaInstallPrompt,
    confirmPwaInstall: store.confirmPwaInstall,
    ingestInstalledPwaApp: store.ingestInstalledPwaApp,
    reloadInstalledPwaApps: store.reloadInstalledPwaApps,
    launchInstalledPwa: store.launchInstalledPwa,
    uninstallPwa: store.uninstallPwa,
    /* ===== M2.5 W-10 auto-update ===== */
    refreshAutoUpdateState: store.refreshAutoUpdateState,
    checkForUpdates: store.checkForUpdates,
    downloadUpdate: store.downloadUpdate,
    installUpdateAndRestart: store.installUpdateAndRestart,
    /* ===== ADR-0005 / W-1 multi-window registry ===== */
    switchActiveSpaceForWindow: store.switchActiveSpaceForWindow,
    setActiveTabForWindow: store.setActiveTabForWindow,
    openTabInNewWindow: store.openTabInNewWindow,
    /* ===== ADR-0005 / P-2 autofill ===== */
    autofillBridgePath: store.autofillBridgePath,
    autofillPrompt: store.autofillPrompt,
    showAutofillPopup: store.showAutofillPopup,
    hideAutofillPopup: store.hideAutofillPopup,
    acceptAutofillMatch: store.acceptAutofillMatch,
    saveCurrentFormAsAddress: store.saveCurrentFormAsAddress
  }), [activeWebview, focusAddressBar, peekCompactChrome, peekCompactSidebar, peekCompactToolbar, store, webviews]);
}

export type BrowserActions = ReturnType<typeof useBrowserActions>;
