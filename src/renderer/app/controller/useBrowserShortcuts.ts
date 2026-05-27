import { useCallback } from "react";

import { getNumberShortcutTarget } from "../../common/shortcuts/numberShortcutTargets";
import type { ShortcutIntent } from "../../common/shortcuts/keyboardShortcuts";
import type { Workspace } from "../../domain/browser";
import { useBrowserStore } from "../../stores/browserStore";
import type { BrowserStore, SplitLayout } from "../../stores/browserStoreTypes";
import type { WebviewElement } from "../../types/browser-ui";
import type { BrowserActions } from "./useBrowserActions";

interface BrowserShortcutsOptions {
  actions: BrowserActions;
  activeWebview: WebviewElement | undefined;
  activeWorkspace: Workspace;
  store: BrowserStore;
}

export function useBrowserShortcuts({
  actions,
  activeWebview,
  activeWorkspace,
  store
}: BrowserShortcutsOptions) {
  return useCallback((intent: ShortcutIntent) => {
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
      actions.focusAddressBar();
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
      activateSplitLayout(store, "horizontal");
    } else if (intent.type === "toggleSplitVertical") {
      activateSplitLayout(store, "vertical");
    } else if (intent.type === "unsplitAll") {
      if (store.state.splitMode) store.toggleSplitMode();
    } else {
      shortcutActions[intent.type]?.();
    }
  }, [actions, activeWebview, activeWorkspace, store]);
}

function activateSplitLayout(store: BrowserStore, layout: SplitLayout) {
  store.setSplitLayout(layout);
  if (!store.state.splitMode) {
    store.toggleSplitMode();
  }
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
