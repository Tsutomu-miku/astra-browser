import { create } from "zustand";

import {
  addTab,
  assignTabToGroup,
  addWorkspace,
  clearHistory,
  clearWorkspaceBrowsingData,
  closeActiveTab,
  closeTab,
  clearBrowsingData,
  clearSitePermissionRule,
  clearSitePermissionRulesForOrigin,
  duplicateActiveTab,
  duplicateTab,
  fillSplitView,
  focusSplitPane,
  groupActiveTab,
  groupTab,
  closeOtherTabs,
  closeTabsToLeft,
  closeTabsToRight,
  deleteWorkspace,
  moveTabToWorkspace,
  openTabInSplit,
  openUrlInSplit,
  removeTabFromSplit,
  navigateActiveTab,
  openUrlInActiveWorkspace,
  recordHistory,
  removeEssential,
  removeHistoryEntry,
  reorderWorkspace,
  reorderTab,
  removeWorkspaceFavorite,
  resetActiveTabZoom,
  restoreClosedTab,
  restoreLastClosedTab,
  selectAdjacentTab,
  selectTab,
  setActiveTabZoom,
  sleepInactiveTabs,
  sleepTab,
  setSitePermission,
  stepActiveTabZoom,
  switchWorkspace,
  toggleActiveTabFavorite,
  toggleActiveTabEssential,
  toggleActiveTabMuted,
  toggleActiveTabPinned,
  toggleTabEssential,
  toggleTabFavorite,
  toggleTabGroupCollapsed,
  toggleTabMuted,
  toggleTabPinned,
  toggleSplitMode,
  ungroupActiveTab,
  ungroupTab,
  updateTabGroup,
  updateSettings,
  updateTab,
  updateWorkspace,
  updateWorkspaceById,
  upsertDownload
} from "../domain/actions";
import {
  getBrowserPartitions,
  getProfileIdForPartition,
  getWorkspacePartition,
  type BrowserState
} from "../domain/browser";
import { getPermissionRule } from "../domain/permissions/sitePermissions";
import { loadBrowserState, saveBrowserState } from "../platform/persistence/browserStorage";
import type { WebviewElement } from "../types/browser-ui";
import { getActiveProfileId, getActiveUrl } from "./browserStoreSelectors";
import type { BrowserStore } from "./browserStoreTypes";

const initialState = loadBrowserState();

export const useBrowserStore = create<BrowserStore>((set) => ({
  addressValue: "",
  commandOpen: false,
  commandQuery: "",
  compactMode: false,
  findOpen: false,
  findQuery: "",
  floatingSidebarOpen: false,
  floatingToolbarOpen: false,
  panel: null,
  permissionRequest: null,
  sidebarCollapsed: false,
  splitLayout: "horizontal",
  state: initialState,
  glance: null,
  addWorkspace: () => update(set, addWorkspace),
  assignTabToGroup: (tabId, groupId) => update(set, (state) => assignTabToGroup(state, tabId, groupId)),
  clearBrowsingData: () => {
    const next = update(set, clearBrowsingData);
    void window.astraShell?.clearBrowsingData(getBrowserPartitions(next));
    set({ permissionRequest: null });
  },
  clearHistory: () => update(set, clearHistory),
  clearWorkspaceBrowsingData: (workspaceId) => {
    const current = useBrowserStore.getState().state;
    const workspace = current.workspaces.find((candidate) => candidate.id === workspaceId);
    const next = update(set, (state) => clearWorkspaceBrowsingData(state, workspaceId));
    if (workspace) {
      void window.astraShell?.clearBrowsingData([getWorkspacePartition(workspace)]);
    }
    set({ permissionRequest: null });
    return next;
  },
  closeActiveTab: () => update(set, closeActiveTab),
  closeOtherTabs: (tabId) => update(set, (state) => closeOtherTabs(state, tabId)),
  closeTabsToLeft: (tabId) => update(set, (state) => closeTabsToLeft(state, tabId)),
  closeTabsToRight: (tabId) => update(set, (state) => closeTabsToRight(state, tabId)),
  closeTab: (tabId) => update(set, (state) => closeTab(state, tabId)),
  clearSitePermission: (profileId, origin, permission) =>
    update(set, (state) => clearSitePermissionRule(state, profileId, origin, permission)),
  clearSitePermissionsForOrigin: (profileId, origin) =>
    update(set, (state) => clearSitePermissionRulesForOrigin(state, profileId, origin)),
  deleteWorkspace: (workspaceId) => update(set, (state) => deleteWorkspace(state, workspaceId)),
  duplicateActiveTab: () => update(set, duplicateActiveTab),
  duplicateTab: (tabId) => update(set, (state) => duplicateTab(state, tabId)),
  fillSplitView: () => {
    update(set, fillSplitView);
    set({ splitLayout: "grid" });
  },
  groupActiveTab: () => update(set, groupActiveTab),
  groupTab: (tabId) => update(set, (state) => groupTab(state, tabId)),
  ingestDownload: (download) => {
    update(set, (state) => upsertDownload(state, download));
    set({ panel: "downloads" });
  },
  ingestPermissionRequest: (request) => {
    const state = useBrowserStore.getState().state;
    const profileId = getProfileIdForPartition(state, request.partition) ?? getActiveProfileId(state);
    const rule = getPermissionRule(state.sitePermissions, profileId, request.origin, request.permission);
    if (rule) {
      window.astraShell?.resolvePermissionRequest(request.id, rule.decision === "allow");
      return;
    }
    set({ permissionRequest: { ...request, profileId } });
  },
  moveTabToWorkspace: (tabId, workspaceId) => update(set, (state) => moveTabToWorkspace(state, tabId, workspaceId)),
  focusSplitPane: (tabId) => update(set, (state) => focusSplitPane(state, tabId)),
  closeGlance: () => set({ glance: null }),
  openGlance: (url, title) => set({ glance: { title: title || url, url } }),
  openGlanceInSplit: () => {
    const glance = useBrowserStore.getState().glance;
    if (!glance) return;
    update(set, (state) => openUrlInSplit(state, glance.url, glance.title));
    set({ glance: null });
  },
  openTabInSplit: (tabId) => update(set, (state) => openTabInSplit(state, tabId)),
  openUrlInSplit: (url, title) => update(set, (state) => openUrlInSplit(state, url, title)),
  navigateActiveTab: (url, webview) => update(set, (state) => {
    const next = navigateActiveTab(state, url);
    webview?.loadURL?.(getActiveUrl(next));
    return next;
  }),
  newTab: () => update(set, addTab),
  openUrlInActiveWorkspace: (url, title) => update(set, (state) => openUrlInActiveWorkspace(state, url, title)),
  recordHistory: (tabId, url) => update(set, (state) => recordHistory(state, tabId, url)),
  removeEssential: (url) => update(set, (state) => removeEssential(state, url)),
  removeHistoryEntry: (historyId) => update(set, (state) => removeHistoryEntry(state, historyId)),
  removeWorkspaceFavorite: (url) => update(set, (state) => removeWorkspaceFavorite(state, url)),
  removeTabFromSplit: (tabId) => update(set, (state) => removeTabFromSplit(state, tabId)),
  replaceBrowserState: (state) => {
    saveBrowserState(state);
    set({ state, addressValue: getActiveUrl(state), permissionRequest: null });
  },
  reorderWorkspace: (workspaceId, targetWorkspaceId, placement) =>
    update(set, (state) => reorderWorkspace(state, workspaceId, targetWorkspaceId, placement)),
  reorderTab: (tabId, targetTabId, placement) => update(set, (state) => reorderTab(state, tabId, targetTabId, placement)),
  resetActiveTabZoom: (webview) => update(set, (state) => syncZoom(resetActiveTabZoom(state), webview)),
  resolvePermissionRequest: (decision) => {
    const request = useBrowserStore.getState().permissionRequest;
    if (!request) return;
    window.astraShell?.resolvePermissionRequest(request.id, decision === "allow");
    update(set, (state) => setSitePermission(state, {
      decision,
      profileId: request.profileId ?? getActiveProfileId(state),
      origin: request.origin,
      permission: request.permission
    }));
    set({ permissionRequest: null });
  },
  restoreClosedTab: (closedIndex) => update(set, (state) => restoreClosedTab(state, closedIndex)),
  restoreLastClosedTab: () => update(set, restoreLastClosedTab),
  runWebviewAction: (action, webview) => webview?.[action]?.(),
  selectAdjacentTab: (direction) => update(set, (state) => selectAdjacentTab(state, direction)),
  selectTab: (tabId) => update(set, (state) => selectTab(state, tabId)),
  sleepInactiveTabs: () => update(set, sleepInactiveTabs),
  sleepTab: (tabId) => update(set, (state) => sleepTab(state, tabId)),
  setActiveTabZoom: (zoomFactor, webview) => update(set, (state) => syncZoom(setActiveTabZoom(state, zoomFactor), webview)),
  setAddressValue: (addressValue) => {
    if (useBrowserStore.getState().addressValue !== addressValue) {
      set({ addressValue });
    }
  },
  setCommandOpen: (commandOpen) => set({ commandOpen }),
  setCommandQuery: (commandQuery) => set({ commandQuery }),
  setFindOpen: (findOpen) => set({ findOpen }),
  setFindQuery: (findQuery) => set({ findQuery }),
  setPanel: (panel) => set({ panel }),
  setSplitLayout: (splitLayout) => set({ splitLayout }),
  setSitePermission: (profileId, origin, permission, decision) =>
    update(set, (state) => setSitePermission(state, { profileId, origin, permission, decision })),
  switchWorkspace: (workspaceId) => update(set, (state) => switchWorkspace(state, workspaceId)),
  toggleActiveTabFavorite: () => update(set, toggleActiveTabFavorite),
  toggleActiveTabEssential: () => update(set, toggleActiveTabEssential),
  toggleActiveTabMuted: (webview) => update(set, (state) => syncMuted(toggleActiveTabMuted(state), webview)),
  toggleActiveTabPinned: () => update(set, toggleActiveTabPinned),
  toggleCompactMode: () => set((state) => ({
    compactMode: !state.compactMode,
    floatingSidebarOpen: false,
    floatingToolbarOpen: false,
    sidebarCollapsed: !state.compactMode ? true : state.sidebarCollapsed
  })),
  toggleFloatingSidebar: () => set((state) => ({
    compactMode: true,
    floatingSidebarOpen: state.compactMode ? !state.floatingSidebarOpen : true,
    sidebarCollapsed: true
  })),
  toggleFloatingToolbar: () => set((state) => ({
    compactMode: true,
    floatingToolbarOpen: state.compactMode ? !state.floatingToolbarOpen : true,
    sidebarCollapsed: state.compactMode ? state.sidebarCollapsed : true
  })),
  toggleTabGroupCollapsed: (groupId) => update(set, (state) => toggleTabGroupCollapsed(state, groupId)),
  toggleTabEssential: (tabId) => update(set, (state) => toggleTabEssential(state, tabId)),
  toggleTabFavorite: (tabId) => update(set, (state) => toggleTabFavorite(state, tabId)),
  toggleTabMuted: (tabId, webview) => update(set, (state) => syncMuted(toggleTabMuted(state, tabId), webview, tabId)),
  toggleTabPinned: (tabId) => update(set, (state) => toggleTabPinned(state, tabId)),
  toggleSidebar: () => set((state) => state.compactMode
    ? { floatingSidebarOpen: !state.floatingSidebarOpen, sidebarCollapsed: true }
    : { sidebarCollapsed: !state.sidebarCollapsed }),
  toggleSplitMode: () => update(set, toggleSplitMode),
  ungroupActiveTab: () => update(set, ungroupActiveTab),
  ungroupTab: (tabId) => update(set, (state) => ungroupTab(state, tabId)),
  updateSettings: (patch) => update(set, (state) => updateSettings(state, patch)),
  updateTabGroup: (groupId, patch) => update(set, (state) => updateTabGroup(state, groupId, patch)),
  updateTab: (tabId, patch) => update(set, (state) => updateTab(state, tabId, patch)),
  updateWorkspaceById: (workspaceId, patch) => update(set, (state) => updateWorkspaceById(state, workspaceId, patch)),
  updateWorkspace: (patch) => update(set, (state) => updateWorkspace(state, patch)),
  zoomIn: (webview) => update(set, (state) => syncZoom(stepActiveTabZoom(state, 1), webview)),
  zoomOut: (webview) => update(set, (state) => syncZoom(stepActiveTabZoom(state, -1), webview))
}));

function update(
  set: (partial: Partial<BrowserStore>) => void,
  updater: (state: BrowserState) => BrowserState
): BrowserState {
  const next = updater(useBrowserStore.getState().state);
  saveBrowserState(next);
  set({ state: next, addressValue: getActiveUrl(next) });
  return next;
}

function syncZoom(state: BrowserState, webview?: WebviewElement): BrowserState {
  const workspace = state.workspaces.find((candidate) => candidate.id === state.activeWorkspaceId) ?? state.workspaces[0];
  const tab = workspace.tabs.find((candidate) => candidate.id === workspace.activeTabId) ?? workspace.tabs[0];
  webview?.setZoomFactor?.(tab.zoomFactor);
  return state;
}

function syncMuted(state: BrowserState, webview?: WebviewElement, tabId?: string): BrowserState {
  const workspace = state.workspaces.find((candidate) => candidate.id === state.activeWorkspaceId) ?? state.workspaces[0];
  const tab = tabId
    ? state.workspaces.flatMap((candidate) => candidate.tabs).find((candidate) => candidate.id === tabId)
    : workspace.tabs.find((candidate) => candidate.id === workspace.activeTabId) ?? workspace.tabs[0];
  if (tab) webview?.setAudioMuted?.(tab.isMuted);
  return state;
}
