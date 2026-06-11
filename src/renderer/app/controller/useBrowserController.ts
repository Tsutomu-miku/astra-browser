import { useCallback, useEffect, useMemo, useRef } from "react";

import { getActiveTab, getActiveWorkspace } from "../../domain/browser/selectors";
import { registerReadyWebview, unregisterWebview } from "../../platform/webviewLifecycle";
import { useBrowserStore } from "../../stores/browserStore";
import type { WebviewElement } from "../../types/browser-ui";
import { useAddressBarFocus } from "./useAddressBarFocus";
import { useBrowserActions } from "./useBrowserActions";
import { useBrowserEffects } from "./useBrowserEffects";
import { useBrowserShortcuts } from "./useBrowserShortcuts";
import { buildCommands } from "./useCommands";
import { useCompactChromePeek } from "./useCompactChromePeek";

export function useBrowserController() {
  const store = useBrowserStore();
  const webviews = useRef(new Map<string, WebviewElement>());
  const {
    compactChromePeeking,
    compactSidebarPeeking,
    compactToolbarPeeking,
    holdCompactToolbar,
    peekCompactChrome,
    peekCompactSidebar,
    peekCompactToolbar,
    releaseCompactToolbar
  } = useCompactChromePeek(store.compactMode);
  const activeWorkspace = getActiveWorkspace(store.state);
  /** ADR-0005: 同 Space 在不同窗口可有不同 activeTab；优先取本窗口 window-scoped 值，
   *  找不到则回退到 canonical Space.activeTabId（与之前行为一致）。 */
  const activeTabId = useMemo(() => {
    const ownId = store.ownWindowId;
    if (ownId != null) {
      const entry = store.windowRegistry.find((w) => w.windowId === ownId);
      const scoped = entry?.spaceFocus[activeWorkspace.id]?.activeTabId;
      if (scoped && activeWorkspace.tabs.some((t) => t.id === scoped)) return scoped;
    }
    return activeWorkspace.activeTabId;
  }, [store.ownWindowId, store.windowRegistry, activeWorkspace.id, activeWorkspace.activeTabId, activeWorkspace.tabs]);
  const activeTab = useMemo(
    () => activeWorkspace.tabs.find((t) => t.id === activeTabId) ?? getActiveTab(activeWorkspace),
    [activeWorkspace, activeTabId]
  );
  const activeWebview = webviews.current.get(activeTab.id);
  const registerWebview = useCallback((tabId: string, webview: WebviewElement) => {
    registerReadyWebview(webviews.current, tabId, webview);
  }, []);
  const removeWebview = useCallback((tabId: string, webview: WebviewElement) => {
    unregisterWebview(webviews.current, tabId, webview);
  }, []);
  const focusAddressBar = useAddressBarFocus(store, peekCompactSidebar);
  const actions = useBrowserActions({
    activeWebview,
    focusAddressBar,
    peekCompactChrome,
    peekCompactSidebar,
    peekCompactToolbar,
    store,
    webviews
  });
  const commands = useMemo(() => buildCommands(store.state, actions, store.setPanel, {
    compactMode: store.compactMode,
    floatingSidebarOpen: store.floatingSidebarOpen,
    floatingToolbarOpen: store.floatingToolbarOpen,
    sidebarCollapsed: store.sidebarCollapsed
  }), [actions, store]);
  const handleShortcut = useBrowserShortcuts({ actions, activeWebview, activeWorkspace, store });

  useEffect(() => {
    useBrowserStore.getState().setAddressValue(activeTab.url);
  }, [activeTab.url]);

  /* ===== M2.6 U-1: 切换 Tab 时同步 MediaSession（暂停前 Tab、接管媒体键）。
   *   通过 useRef 记录上一个活跃 Tab 的 webContentsId，对比当前 activeTab。
   *   这是 Option A：通过 store 响应式订阅，覆盖所有 tab 切换路径（selectTab、
   *   selectAdjacentTab、closeTab、switchWorkspace 等）。
   */
  const previousTabIdRef = useRef<{ tabId?: string; webContentsId?: number }>({});

  useEffect(() => {
    const prev = previousTabIdRef.current;
    const currentWebviewId = activeWebview?.getWebContentsId?.();
    const currentId: number | undefined = typeof currentWebviewId === "number" ? currentWebviewId : undefined;
    if (prev.tabId !== undefined && prev.tabId !== activeTab.id) {
      void actions.syncMediaSessionOnTabSwitch?.({
        fromId: prev.webContentsId,
        toId: currentId
      });
    }
    previousTabIdRef.current = { tabId: activeTab.id, webContentsId: currentId };
  }, [activeTab.id, activeWebview, actions]);

  useEffect(() => {
    if (!activeWebview) return;

    const onFoundInPage = (event: Event) => {
      const result = (event as {
        result?: {
          activeMatchOrdinal?: number;
          finalUpdate?: boolean;
          matches?: number;
        };
      }).result;

      if (!result) return;
      store.setFindResult({
        activeMatchOrdinal: result.activeMatchOrdinal ?? 0,
        finalUpdate: Boolean(result.finalUpdate),
        matches: result.matches ?? 0
      });
    };

    activeWebview.addEventListener("found-in-page", onFoundInPage);
    return () => activeWebview.removeEventListener("found-in-page", onFoundInPage);
  }, [activeWebview, store]);

  useBrowserEffects({
    actions: {
      closeActiveTab: actions.closeActiveTab,
      findInPage: actions.findInPage,
      focusAddressBar: actions.focusAddressBar,
      newTab: actions.newTab,
      newIncognitoWindow: store.newIncognitoWindow,
      newGuestWindow: store.newGuestWindow,
      openUrlInActiveWorkspace: actions.openUrlInActiveWorkspace,
      printActiveTab: actions.printActiveTab,
      resetActiveTabZoom: actions.resetActiveTabZoom,
      restoreLastClosedTab: actions.restoreLastClosedTab,
      runWebviewAction: actions.runWebviewAction,
      selectAdjacentTab: actions.selectAdjacentTab,
      setCommandOpen: (open) => store.setCommandOpen(open),
      setFindOpen: (open) => store.setFindOpen(open),
      setPanel: store.setPanel,
      syncForceHttps: actions.syncForceHttps,
      syncSafeBrowsing: actions.syncSafeBrowsing,
      toggleActiveDevTools: () => store.toggleActiveDevTools(activeWebview),
      toggleActiveTabFavorite: actions.toggleActiveTabFavorite,
      toggleActiveTabMuted: actions.toggleActiveTabMuted,
      toggleActivePictureInPicture: actions.toggleActivePictureInPicture,
      toggleSidebar: store.toggleSidebar,
      zoomIn: () => store.zoomIn(activeWebview),
      zoomOut: () => store.zoomOut(activeWebview)
    },
    findQuery: store.findQuery,
    forceHttps: store.state.settings.forceHttps ?? false,
    safeBrowsing: store.state.settings.safeBrowsingEnabled ?? true,
    ingestDownload: store.ingestDownload,
    ingestPermissionRequest: store.ingestPermissionRequest,
    ingestPasswordSavePrompt: store.ingestPasswordSavePrompt,
    ingestPendingPwaInstallPrompt: store.ingestPendingPwaInstallPrompt,
    ingestInstalledPwaApp: store.ingestInstalledPwaApp,
    reloadInstalledPwaApps: store.reloadInstalledPwaApps,
    reloadInstalledExtensions: store.reloadInstalledExtensions,
    onAutoUpdateStateChange: store.setAutoUpdateState,
    refreshAutoUpdateState: store.refreshAutoUpdateState,
    setOwnWindowId: store.setOwnWindowId,
    syncWindowRegistry: store.syncWindowRegistry,
    loadAutofillBridgePath: store.loadAutofillBridgePath,
    showAutofillPopup: store.showAutofillPopup,
    hideAutofillPopup: store.hideAutofillPopup,
    onShortcut: handleShortcut,
    openUrlInNewTab: (url) => store.openUrlInActiveWorkspace(url),
    sitePermissions: store.state.sitePermissions,
    sleepIdleTabs: store.sleepIdleTabs,
    workspaces: store.state.workspaces
  });

  return {
    actions,
    activeTab,
    activeWorkspace,
    addressValue: store.addressValue,
    commands,
    commandOpen: store.commandOpen,
    commandQuery: store.commandQuery,
    compactChromePeeking,
    compactMode: store.compactMode,
    compactSidebarPeeking,
    compactToolbarPeeking,
    findOpen: store.findOpen,
    findQuery: store.findQuery,
    findResult: store.findResult,
    floatingSidebarOpen: store.floatingSidebarOpen,
    floatingToolbarOpen: store.floatingToolbarOpen,
    glance: store.glance,
    holdCompactToolbar,
    panel: store.panel,
    permissionRequest: store.permissionRequest,
    passwordSavePrompt: store.passwordSavePrompt,
    passwordVaultUnlocked: store.passwordVaultUnlocked,
    safeBrowsingAlert: store.safeBrowsingAlert,
    checkSafeBrowsingForNavigation: store.checkSafeBrowsingForNavigation,
    dismissSafeBrowsingAlert: store.dismissSafeBrowsingAlert,
    registerWebview,
    removeWebview,
    releaseCompactToolbar,
    setAddressValue: store.setAddressValue,
    setCommandOpen: store.setCommandOpen,
    setCommandQuery: store.setCommandQuery,
    setFindOpen: store.setFindOpen,
    setFindQuery: store.setFindQuery,
    setPanel: store.setPanel,
    setSidebarWidth: store.setSidebarWidth,
    sidebarCollapsed: store.sidebarCollapsed,
    sidebarWidth: store.sidebarWidth,
    splitLayout: activeWorkspace.splitLayout,
    state: store.state
  };
}
