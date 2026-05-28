import { useEffect, useMemo, useRef } from "react";

import { getActiveTab, getActiveWorkspace } from "../../domain/browser/selectors";
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
  const { compactChromePeeking, peekCompactChrome } = useCompactChromePeek(store.compactMode);
  const activeWorkspace = getActiveWorkspace(store.state);
  const activeTab = getActiveTab(activeWorkspace);
  const activeWebview = webviews.current.get(activeTab.id);
  const focusAddressBar = useAddressBarFocus(store);
  const actions = useBrowserActions({
    activeWebview,
    focusAddressBar,
    peekCompactChrome,
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
