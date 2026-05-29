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
  const activeTab = getActiveTab(activeWorkspace);
  const activeWebview = webviews.current.get(activeTab.id);
  const registerWebview = useCallback((tabId: string, webview: WebviewElement) => {
    registerReadyWebview(webviews.current, tabId, webview);
  }, []);
  const removeWebview = useCallback((tabId: string, webview: WebviewElement) => {
    unregisterWebview(webviews.current, tabId, webview);
  }, []);
  const focusAddressBar = useAddressBarFocus(store);
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
    ingestDownload: store.ingestDownload,
    ingestPermissionRequest: store.ingestPermissionRequest,
    onShortcut: handleShortcut,
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
    splitLayout: store.splitLayout,
    state: store.state
  };
}
