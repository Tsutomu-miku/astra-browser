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
  duplicateActiveTab,
  duplicateTab,
  fillSplitView,
  focusSplitPane,
  groupActiveTab,
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
  removeHistoryEntry,
  reorderWorkspace,
  reorderTab,
  type TabDropPlacement,
  type WorkspaceDropPlacement,
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
  toggleTabGroupCollapsed,
  toggleTabMuted,
  toggleTabPinned,
  toggleSplitMode,
  ungroupActiveTab,
  updateTabGroup,
  updateSettings,
  updateTab,
  updateWorkspace,
  upsertDownload
} from "../domain/browser-actions";
import {
  getBrowserPartitions,
  getProfileIdForPartition,
  getWorkspacePartition,
  type BrowserState,
  type BrowserTab,
  type DownloadEntry,
  type Workspace
} from "../domain/browser-core";
import { getPermissionRule } from "../domain/sitePermissions";
import { loadBrowserState, saveBrowserState } from "../platform/persistence/browserStorage";
import type { PermissionRequestEvent } from "../types/electron";
import type { WebviewAction, WebviewElement } from "../types/browser-ui";
import { getActiveProfileId, getActiveUrl } from "./browserStoreSelectors";

export type Panel = "history" | "downloads" | "settings" | "site" | null;
export type SplitLayout = "grid" | "horizontal" | "vertical";

interface BrowserStore {
  addressValue: string;
  commandOpen: boolean;
  commandQuery: string;
  compactMode: boolean;
  findOpen: boolean;
  findQuery: string;
  floatingSidebarOpen: boolean;
  floatingToolbarOpen: boolean;
  panel: Panel;
  permissionRequest: PermissionRequestEvent | null;
  sidebarCollapsed: boolean;
  splitLayout: SplitLayout;
  state: BrowserState;
  glance: { title: string; url: string } | null;
  addWorkspace: () => void;
  assignTabToGroup: (tabId: string, groupId: string) => void;
  clearBrowsingData: () => void;
  clearHistory: () => void;
  clearWorkspaceBrowsingData: (workspaceId: string) => void;
  closeActiveTab: () => void;
  closeOtherTabs: () => void;
  closeTabsToLeft: () => void;
  closeTabsToRight: () => void;
  closeTab: (tabId: string) => void;
  clearSitePermission: (profileId: string, origin: string, permission: string) => void;
  deleteWorkspace: (workspaceId: string) => void;
  duplicateActiveTab: () => void;
  duplicateTab: (tabId: string) => void;
  fillSplitView: () => void;
  focusSplitPane: (tabId: string) => void;
  groupActiveTab: () => void;
  ingestDownload: (download: DownloadEntry) => void;
  ingestPermissionRequest: (request: PermissionRequestEvent) => void;
  moveTabToWorkspace: (tabId: string, workspaceId: string) => void;
  closeGlance: () => void;
  openGlance: (url: string, title?: string) => void;
  openGlanceInSplit: () => void;
  openTabInSplit: (tabId: string) => void;
  openUrlInSplit: (url: string, title?: string) => void;
  navigateActiveTab: (url: string, webview?: WebviewElement) => void;
  newTab: () => void;
  openUrlInActiveWorkspace: (url: string, title?: string) => void;
  recordHistory: (tabId: string, url: string) => void;
  removeHistoryEntry: (historyId: string) => void;
  removeTabFromSplit: (tabId: string) => void;
  replaceBrowserState: (state: BrowserState) => void;
  reorderTab: (tabId: string, targetTabId: string, placement: TabDropPlacement) => void;
  reorderWorkspace: (workspaceId: string, targetWorkspaceId: string, placement: WorkspaceDropPlacement) => void;
  runWebviewAction: (action: WebviewAction, webview?: WebviewElement) => void;
  selectAdjacentTab: (direction: 1 | -1) => void;
  selectTab: (tabId: string) => void;
  sleepInactiveTabs: () => void;
  sleepTab: (tabId: string) => void;
  resetActiveTabZoom: (webview?: WebviewElement) => void;
  resolvePermissionRequest: (decision: "allow" | "block") => void;
  restoreClosedTab: (closedIndex: number) => void;
  restoreLastClosedTab: () => void;
  setActiveTabZoom: (zoomFactor: number, webview?: WebviewElement) => void;
  setAddressValue: (value: string) => void;
  setCommandOpen: (open: boolean) => void;
  setCommandQuery: (query: string) => void;
  setFindOpen: (open: boolean) => void;
  setFindQuery: (query: string) => void;
  setPanel: (panel: Panel) => void;
  setSplitLayout: (layout: SplitLayout) => void;
  setSitePermission: (profileId: string, origin: string, permission: string, decision: "allow" | "block") => void;
  switchWorkspace: (workspaceId: string) => void;
  toggleActiveTabFavorite: () => void;
  toggleActiveTabEssential: () => void;
  toggleActiveTabMuted: (webview?: WebviewElement) => void;
  toggleActiveTabPinned: () => void;
  toggleCompactMode: () => void;
  toggleFloatingSidebar: () => void;
  toggleFloatingToolbar: () => void;
  toggleTabGroupCollapsed: (groupId: string) => void;
  toggleTabMuted: (tabId: string, webview?: WebviewElement) => void;
  toggleTabPinned: (tabId: string) => void;
  toggleSidebar: () => void;
  toggleSplitMode: () => void;
  ungroupActiveTab: () => void;
  zoomIn: (webview?: WebviewElement) => void;
  zoomOut: (webview?: WebviewElement) => void;
  updateSettings: (patch: Partial<BrowserState["settings"]>) => void;
  updateTabGroup: (groupId: string, patch: Partial<Pick<Workspace["tabGroups"][number], "name" | "color">>) => void;
  updateTab: (tabId: string, patch: Partial<BrowserTab>) => void;
  updateWorkspace: (patch: Partial<Pick<Workspace, "name" | "accent" | "homepage" | "profileName">>) => void;
}

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
  closeOtherTabs: () => update(set, closeOtherTabs),
  closeTabsToLeft: () => update(set, closeTabsToLeft),
  closeTabsToRight: () => update(set, closeTabsToRight),
  closeTab: (tabId) => update(set, (state) => closeTab(state, tabId)),
  clearSitePermission: (profileId, origin, permission) =>
    update(set, (state) => clearSitePermissionRule(state, profileId, origin, permission)),
  deleteWorkspace: (workspaceId) => update(set, (state) => deleteWorkspace(state, workspaceId)),
  duplicateActiveTab: () => update(set, duplicateActiveTab),
  duplicateTab: (tabId) => update(set, (state) => duplicateTab(state, tabId)),
  fillSplitView: () => {
    update(set, fillSplitView);
    set({ splitLayout: "grid" });
  },
  groupActiveTab: () => update(set, groupActiveTab),
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
  removeHistoryEntry: (historyId) => update(set, (state) => removeHistoryEntry(state, historyId)),
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
  toggleTabMuted: (tabId, webview) => update(set, (state) => syncMuted(toggleTabMuted(state, tabId), webview, tabId)),
  toggleTabPinned: (tabId) => update(set, (state) => toggleTabPinned(state, tabId)),
  toggleSidebar: () => set((state) => state.compactMode
    ? { floatingSidebarOpen: !state.floatingSidebarOpen, sidebarCollapsed: true }
    : { sidebarCollapsed: !state.sidebarCollapsed }),
  toggleSplitMode: () => update(set, toggleSplitMode),
  ungroupActiveTab: () => update(set, ungroupActiveTab),
  updateSettings: (patch) => update(set, (state) => updateSettings(state, patch)),
  updateTabGroup: (groupId, patch) => update(set, (state) => updateTabGroup(state, groupId, patch)),
  updateTab: (tabId, patch) => update(set, (state) => updateTab(state, tabId, patch)),
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
