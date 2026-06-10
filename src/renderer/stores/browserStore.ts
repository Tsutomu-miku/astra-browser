/* eslint-disable max-lines */
// This file is a zustand dispatch table: every entry is a one-line
// `update(set, domainAction)` forwarder into the pure domain layer, plus a
// handful of UI state toggles (sidebar, panel, glance, permission request).
// Splitting it into multiple slices would add indirection without clarifying
// ownership, since all entries share the same structural shape and all route
// through the single `update` helper at the bottom of the file.
import { create } from "zustand";

import { SIDEBAR_DEFAULT_WIDTH, clampSidebarWidth } from "../common/layout/sidebarSizing";
import {
  addTab,
  addTabToFavorites,
  assignTabToGroup,
  addWorkspace,
  clearAllPerOriginZoomSettings,
  clearHistory,
  clearPerOriginZoom,
  clearWorkspaceBrowsingData,
  closeActiveTab,
  closeTab,
  closeTabGroup,
  clearBrowsingData,
  clearSitePermissionRule,
  clearSitePermissionRulesForOrigin,
  duplicateActiveTab,
  duplicateTab,
  duplicateTabGroup,
  fillSplitView,
  focusSplitPane,
  groupActiveTab,
  groupTab,
  groupTabsTogether,
  closeOtherTabs,
  closeTabsToLeft,
  closeTabsToRight,
  deleteWorkspace,
  moveTabToWorkspace,
  moveTabGroupToWorkspace,
  moveTabToFavoritePosition,
  moveTabToFolderEnd,
  moveTabToFolderPosition,
  moveWorkspaceFavoriteToWorkspace,
  moveTabToNewWorkspace,
  moveTabGroupToNewWorkspace,
  moveWorkspaceFavoriteToNewWorkspace,
  newTabInGroup,
  restoreClosedTabToNewWorkspace,
  openTabInSplit,
  openUrlInSplit,
  removeTabFromSplit,
  navigateActiveTab,
  openUrlInActiveWorkspace,
  recordHistory,
  removeAddress,
  removeEssential,
  removeHistoryEntry,
  removePassword,
  removePaymentMethod,
  reorderWorkspace,
  reorderTab,
  removeWorkspaceFavorite,
  reorderEssential,
  reorderTabGroup,
  reorderWorkspaceFavorite,
  resetActiveTabZoom,
  restoreClosedTab,
  restoreClosedTabToWorkspace,
  restoreLastClosedTab,
  selectAdjacentTab,
  selectTab,
  setActiveTabZoom,
  setIncognitoMode,
  setPerOriginZoom,
  setWorkspaceSplitLayout,
  sleepIdleTabs,
  sleepInactiveTabs,
  sleepTabGroup,
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
  touchPasswordUsed,
  ungroupActiveTab,
  ungroupTab,
  ungroupTabGroup,
  updateReaderSettings,
  updateTabGroup,
  updateSettings,
  updateTab,
  updateTranslationSettings,
  updateWorkspace,
  updateWorkspaceById,
  upsertAddress,
  upsertDownload,
  removeDownload,
  upsertPassword,
  upsertPaymentMethod,
  /* ===== M2.1 Profile / Extension / Reset ===== */
  addProfile,
  removeProfile,
  switchProfile,
  addExtension,
  removeExtension,
  toggleExtension,
  resetSettings,
  clearAllDownloads
} from "../domain/actions";
import {
  getBrowserPartitions,
  getProfileIdForPartition,
  getWorkspacePartition,
  getZoomForUrl,
  importBookmarksFromHtml,
  type AddressEntry,
  type AutofillDatabase,
  type BrowserState,
  type PasswordEntry,
  type PaymentMethodEntry,
  type ReaderSettings,
  type TranslationSettings
} from "../domain/browser";
import { applyReaderStyles, extractReaderContent } from "../domain/browser/readerMode";
import { getPermissionRule } from "../domain/permissions/sitePermissions";
import { loadBrowserState, saveBrowserState } from "../platform/persistence/browserStorage";
import { loadBrowserUiState, saveBrowserUiState } from "../platform/persistence/browserUiStorage";
import { getActiveProfileId, getActiveUrl } from "./browserStoreSelectors";
import type { BrowserStore } from "./browserStoreTypes";
import { syncMuted, syncZoom } from "./browserStoreWebviewSync";

const initialState = loadBrowserState();
const initialUiState = loadBrowserUiState();

export const useBrowserStore = create<BrowserStore>((set, get) => ({
  addressValue: "",
  commandOpen: false,
  commandQuery: "",
  compactMode: false,
  findOpen: false,
  findQuery: "",
  findResult: null,
  floatingSidebarOpen: false,
  floatingToolbarOpen: false,
  pageHtmlCache: new Map(),
  panel: null,
  permissionRequest: null,
  sidebarCollapsed: false,
  sidebarWidth: initialUiState.sidebarWidth ?? SIDEBAR_DEFAULT_WIDTH,
  state: initialState,
  glance: null,
  cachePageHtml: (tabId, html) => {
    const cache = get().pageHtmlCache;
    cache.set(tabId, html);
    if (cache.size > 16) {
      const firstKey = cache.keys().next().value;
      if (typeof firstKey === "string") cache.delete(firstKey);
    }
    set({ pageHtmlCache: cache });
  },
  openActiveTabReader: () => {
    const { state, pageHtmlCache } = get();
    const workspace = state.workspaces.find((ws) => ws.id === state.activeWorkspaceId);
    if (!workspace) return;
    const tab = workspace.tabs.find((t) => t.id === workspace.activeTabId);
    if (!tab) return;
    const html = pageHtmlCache.get(tab.id);
    const title = tab.title || "Reader view";
    if (!html) {
      /* 尚未缓存 HTML。页面停止加载后会填充缓存并可再次打开。 */
      set({ glance: { title, url: tab.url } });
      return;
    }
    try {
      const readerContent = extractReaderContent(html);
      const styled = applyReaderStyles({
        content: readerContent,
        fontSize: state.settings.reader.fontSize,
        fontFamily: state.settings.reader.fontFamily,
        lineHeight: state.settings.reader.lineHeight,
        contentWidth: state.settings.reader.contentWidth,
        theme: state.settings.reader.theme
      });
      const readerUrl = `data:text/html;charset=utf-8,${encodeURIComponent(styled)}`;
      set({ glance: { title: title || readerContent.title || "Reader view", url: readerUrl } });
    } catch {
      set({ glance: { title, url: tab.url } });
    }
  },
  addTabToFavorites: (tabId) => update(set, (state) => addTabToFavorites(state, tabId)),
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
  closeTabGroup: (groupId) => update(set, (state) => closeTabGroup(state, groupId)),
  closeTabsToLeft: (tabId) => update(set, (state) => closeTabsToLeft(state, tabId)),
  closeTabsToRight: (tabId) => update(set, (state) => closeTabsToRight(state, tabId)),
  closeTab: (tabId) => update(set, (state) => closeTab(state, tabId)),
  clearSitePermission: (profileId, origin, permission) =>
    update(set, (state) => clearSitePermissionRule(state, profileId, origin, permission)),
  clearSitePermissionsForOrigin: (profileId, origin) =>
    update(set, (state) => clearSitePermissionRulesForOrigin(state, profileId, origin)),
  clearPerOriginZoom: (origin) => update(set, (state) => clearPerOriginZoom(state, origin)),
  clearAllPerOriginZoomSettings: () => update(set, clearAllPerOriginZoomSettings),
  deleteWorkspace: (workspaceId) => update(set, (state) => deleteWorkspace(state, workspaceId)),
  duplicateActiveTab: () => update(set, duplicateActiveTab),
  duplicateTab: (tabId) => update(set, (state) => duplicateTab(state, tabId)),
  duplicateTabGroup: (groupId) => update(set, (state) => duplicateTabGroup(state, groupId)),
  fillSplitView: () => {
    update(set, fillSplitView);
  },
  groupActiveTab: () => update(set, groupActiveTab),
  groupTab: (tabId) => update(set, (state) => groupTab(state, tabId)),
  groupTabsTogether: (sourceTabId, targetTabId) => update(set, (state) => groupTabsTogether(state, sourceTabId, targetTabId)),
  ingestDownload: (download) => {
    update(set, (state) => upsertDownload(state, download));
    set({ panel: "downloads" });
  },
  cancelDownload: (id) => {
    void window.astraShell?.cancelDownload(id);
  },
  removeDownload: (id) => update(set, (state) => removeDownload(state, id)),
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
  moveTabGroupToWorkspace: (groupId, workspaceId) =>
    update(set, (state) => moveTabGroupToWorkspace(state, groupId, workspaceId)),
  moveWorkspaceFavoriteToWorkspace: (tabId, workspaceId) =>
    update(set, (state) => moveWorkspaceFavoriteToWorkspace(state, tabId, workspaceId)),
  moveTabToNewWorkspace: (tabId) => update(set, (state) => moveTabToNewWorkspace(state, tabId)),
  moveTabGroupToNewWorkspace: (groupId) =>
    update(set, (state) => moveTabGroupToNewWorkspace(state, groupId)),
  moveWorkspaceFavoriteToNewWorkspace: (tabId) =>
    update(set, (state) => moveWorkspaceFavoriteToNewWorkspace(state, tabId)),
  restoreClosedTabToNewWorkspace: (closedIndex) =>
    update(set, (state) => restoreClosedTabToNewWorkspace(state, closedIndex)),
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
  moveTabToFolderEnd: (tabId, folder) => update(set, (state) => moveTabToFolderEnd(state, tabId, folder)),
  moveTabToFolderPosition: (tabId, targetTabId, placement) =>
    update(set, (state) => moveTabToFolderPosition(state, tabId, targetTabId, placement)),
  moveTabToFavoritePosition: (tabId, targetTabId, placement) =>
    update(set, (state) => moveTabToFavoritePosition(state, tabId, targetTabId, placement)),
  navigateActiveTab: (url, webview) => update(set, (state) => {
    const next = navigateActiveTab(state, url);
    webview?.loadURL?.(getActiveUrl(next));
    return next;
  }),
  newTab: () => update(set, addTab),
  newTabInGroup: (groupId) => update(set, (state) => newTabInGroup(state, groupId)),
  openUrlInActiveWorkspace: (url, title) => update(set, (state) => openUrlInActiveWorkspace(state, url, title)),
  recordHistory: (tabId, url) => update(set, (state) => recordHistory(state, tabId, url)),
  removeEssential: (url) => update(set, (state) => removeEssential(state, url)),
  removeHistoryEntry: (historyId) => update(set, (state) => removeHistoryEntry(state, historyId)),
  removeWorkspaceFavorite: (tabId) => update(set, (state) => removeWorkspaceFavorite(state, tabId)),
  removeTabFromSplit: (tabId) => update(set, (state) => removeTabFromSplit(state, tabId)),
  replaceBrowserState: (state) => {
    saveBrowserState(state);
    set({ state, addressValue: getActiveUrl(state), permissionRequest: null });
  },
  reorderEssential: (essentialId, targetEssentialId, placement) =>
    update(set, (state) => reorderEssential(state, essentialId, targetEssentialId, placement)),
  reorderWorkspace: (workspaceId, targetWorkspaceId, placement) =>
    update(set, (state) => reorderWorkspace(state, workspaceId, targetWorkspaceId, placement)),
  reorderWorkspaceFavorite: (tabId, targetTabId, placement) =>
    update(set, (state) => reorderWorkspaceFavorite(state, tabId, targetTabId, placement)),
  reorderTabGroup: (groupId, targetGroupId, placement) =>
    update(set, (state) => reorderTabGroup(state, groupId, targetGroupId, placement)),
  reorderTab: (tabId, targetTabId, placement) => update(set, (state) => reorderTab(state, tabId, targetTabId, placement)),
  resetActiveTabZoom: (webview) => {
    const before = useBrowserStore.getState().state;
    const activeUrl = getActiveUrl(before);
    const next = update(set, (state) => syncZoom(resetActiveTabZoom(state), webview));
    // 如果存在 per-origin 规则，重置到 defaultZoomFactor 也同步清掉，避免下次又被规则覆盖。
    const overridden = getZoomForUrl(before.settings.perOriginZoom, activeUrl);
    if (overridden !== undefined) {
      update(set, (s) => clearPerOriginZoom(s, activeUrl));
    }
    return next;
  },
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
  restoreClosedTabToWorkspace: (closedIndex, workspaceId) =>
    update(set, (state) => restoreClosedTabToWorkspace(state, closedIndex, workspaceId)),
  restoreLastClosedTab: () => update(set, restoreLastClosedTab),
  runWebviewAction: (action, webview) => webview?.[action]?.(),
  selectAdjacentTab: (direction) => update(set, (state) => selectAdjacentTab(state, direction)),
  selectTab: (tabId) => update(set, (state) => {
    const selected = selectTab(state, tabId);
    const ws = selected.workspaces.find((x) => x.id === selected.activeWorkspaceId);
    const active = ws?.tabs.find((t) => t.id === ws.activeTabId);
    const overridden = active
      ? getZoomForUrl(selected.settings.perOriginZoom, active.url)
      : undefined;
    if (!active || overridden === undefined) return selected;
    // tab 切换时若有 per-origin 规则，先写 tab.zoomFactor，再同步给 webview
    active.zoomFactor = overridden;
    return selected;
  }),
  sleepIdleTabs: () => update(set, sleepIdleTabs),
  sleepInactiveTabs: () => update(set, sleepInactiveTabs),
  sleepTabGroup: (groupId) => update(set, (state) => sleepTabGroup(state, groupId)),
  sleepTab: (tabId) => update(set, (state) => sleepTab(state, tabId)),
  setActiveTabZoom: (zoomFactor, webview) => {
    const before = useBrowserStore.getState().state;
    const activeUrl = getActiveUrl(before);
    const next = update(set, (state) => syncZoom(setActiveTabZoom(state, zoomFactor), webview));
    // 主动设置 zoom（非快捷键步长）→ 写入 per-origin，下次访问同站点恢复
    const origin = activeUrl ? new URL(activeUrl).origin : null;
    if (origin && (origin.startsWith("http://") || origin.startsWith("https://"))) {
      update(set, (state) => setPerOriginZoom(state, origin, zoomFactor));
    }
    return next;
  },
  setAddressValue: (addressValue) => {
    if (useBrowserStore.getState().addressValue !== addressValue) {
      set({ addressValue });
    }
  },
  setCommandOpen: (commandOpen) => set({ commandOpen }),
  setCommandQuery: (commandQuery) => set({ commandQuery }),
  setFindOpen: (findOpen) => set({ findOpen }),
  setFindQuery: (findQuery) => set({ findQuery }),
  setFindResult: (findResult) => set({ findResult }),
  setPanel: (panel) => set({ panel }),
  setSidebarWidth: (width) => {
    const sidebarWidth = clampSidebarWidth(width);
    saveBrowserUiState({ sidebarWidth });
    set({ sidebarWidth });
  },
  setSplitLayout: (splitLayout) => update(set, (state) => setWorkspaceSplitLayout(state, splitLayout)),
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
  setIncognito: (mode) => update(set, (state) => setIncognitoMode(state, mode)),
  setPerOriginZoom: (origin, zoom) => update(set, (state) => setPerOriginZoom(state, origin, zoom)),
  /* ===== Autofill actions ===== */
  upsertPassword: (entry) => update(set, (state) => upsertPassword(state, entry)),
  removePassword: (id) => update(set, (state) => removePassword(state, id)),
  touchPasswordUsed: (id) => update(set, (state) => touchPasswordUsed(state, id)),
  upsertAddress: (entry) => update(set, (state) => upsertAddress(state, entry)),
  removeAddress: (id) => update(set, (state) => removeAddress(state, id)),
  upsertPaymentMethod: (entry) => update(set, (state) => upsertPaymentMethod(state, entry)),
  removePaymentMethod: (id) => update(set, (state) => removePaymentMethod(state, id)),
  /* ===== Reader / Translation ===== */
  updateReaderSettings: (patch) => update(set, (state) => updateReaderSettings(state, patch)),
  updateTranslationSettings: (patch) => update(set, (state) => updateTranslationSettings(state, patch)),
  /* ===== Bookmarks import ===== */
  importBookmarks: (html, opts) => {
    const result = importBookmarksFromHtml(html, { source: opts.source, maxCount: opts.maxCount });
    let essentialsAdded = 0;
    let favoritesAdded = 0;
    update(set, (state) => {
      // 1) 合并 Essentials（去重）
      const seenEssentialUrls = new Set(state.essentials.map((e) => e.url));
      const newEssentials = result.essentials.filter((e) => !seenEssentialUrls.has(e.url));
      essentialsAdded = newEssentials.length;
      state.essentials = [...state.essentials, ...newEssentials];
      // 2) 其它书签按 folder → 第一个 workspace 的 favorites
      const workspace = state.workspaces.find((ws) => ws.id === state.activeWorkspaceId) ?? state.workspaces[0];
      if (!workspace) return state;
      const allFolderFavs = Object.values(result.favoritesByFolder).flat();
      const existingUrls = new Set(workspace.tabs.map((t) => t.url));
      for (const fav of allFolderFavs) {
        if (existingUrls.has(fav.url)) continue;
        const tab = {
          id: fav.id,
          title: fav.title,
          url: fav.url,
          faviconUrl: undefined,
          groupId: null,
          canGoBack: false,
          canGoForward: false,
          isFavorite: true,
          isMuted: false,
          isPinned: false,
          isLoading: false,
          isSleeping: true,
          lastActiveAt: Date.now(),
          zoomFactor: 1,
          isMediaPlaying: false,
          isCameraOn: false,
          isMicrophoneOn: false,
          hasUnread: false
        } satisfies import("../domain/browser").BrowserTab;
        workspace.tabs.push(tab);
        workspace.favoriteOrder.push(tab.id);
        favoritesAdded++;
      }
      return state;
    });
    return { essentialsAdded, favoritesAdded };
  },
  toggleActiveDevTools: (webview) => {
    // 优先级：当前聚焦的 webview（Tab/Split/Glance）> 主窗口 Renderer。
    // diagnostics.js 里的 F12 会走到主窗口，这里让 UI 层能显式传 webContentsId。
    const webContentsId = typeof webview === "object" && webview && typeof (webview as { getWebContentsId?: () => number }).getWebContentsId === "function"
      ? (webview as { getWebContentsId: () => number }).getWebContentsId()
      : undefined;
    void window.astraShell?.toggleDevTools?.(webContentsId);
  },
  newIncognitoWindow: () => {
    const mode = useBrowserStore.getState().state.settings.incognito;
    if (mode !== "in-memory") {
      // 先自动启用 in-memory 模式（K-12 MVP：打开 incognito 就会同时打开模式）
      update(set, (state) => setIncognitoMode(state, "in-memory"));
    }
    void window.astraShell?.openIncognitoWindow?.();
  },
  newGuestWindow: () => {
    void window.astraShell?.openGuestWindow?.();
  },
  syncForceHttps: (enabled) => {
    // 同步 settings 到主进程（K-1）：BrowserState.settings.forceHttps 变化时，
    // useBrowserEffect 会把最新值传过来，这里直接转发给 main，返回的副作用在 main 端处理。
    void window.astraShell?.syncForceHttps?.(enabled);
  },
  ungroupActiveTab: () => update(set, ungroupActiveTab),
  ungroupTab: (tabId) => update(set, (state) => ungroupTab(state, tabId)),
  ungroupTabGroup: (groupId) => update(set, (state) => ungroupTabGroup(state, groupId)),
  updateSettings: (patch) => update(set, (state) => updateSettings(state, patch)),
  updateTabGroup: (groupId, patch) => update(set, (state) => updateTabGroup(state, groupId, patch)),
  updateTab: (tabId, patch) => update(set, (state) => updateTab(state, tabId, patch)),
  updateWorkspaceById: (workspaceId, patch) => update(set, (state) => updateWorkspaceById(state, workspaceId, patch)),
  updateWorkspace: (patch) => update(set, (state) => updateWorkspace(state, patch)),
  /* ===== M2.1 Profiles / Extensions / Reset ===== */
  addProfile: (name, color) => update(set, (state) => addProfile(state, name, color)),
  removeProfile: (id) => update(set, (state) => removeProfile(state, id)),
  switchProfile: (id) => update(set, (state) => switchProfile(state, id)),
  switchActiveProfile: (profileId) => update(set, (state) => {
    state.settings.activeProfileId = profileId;
    return state;
  }),
  addExtension: (ext) => update(set, (state) => addExtension(state, ext)),
  removeExtension: (id) => update(set, (state) => removeExtension(state, id)),
  toggleExtensionEnabled: (id, enabled) => update(set, (state) => toggleExtension(state, id, enabled)),
  resetSettings: () => update(set, resetSettings),
  clearAllDownloads: () => update(set, clearAllDownloads),
  openUserDataFolder: async (kind = "userData") => {
    try {
      const paths = await window.astraShell?.getUserDataPaths?.();
      const target = paths?.[kind] ?? "";
      if (target) void window.astraShell?.openPath?.(target);
    } catch {
      /* ignore — dev mode IPC may not be available */
    }
  },
  restartBrowser: () => {
    void window.astraShell?.relaunch?.();
  },
  zoomIn: (webview) => update(set, (state) => {
    const after = stepActiveTabZoom(state, 1);
    // 同步写入 per-origin 以便下次访问恢复
    const activeUrl = getActiveUrl(after);
    try {
      const origin = activeUrl && new URL(activeUrl).origin;
      if (origin && (origin.startsWith("http://") || origin.startsWith("https://"))) {
        const activeTab = after.workspaces.find((ws) => ws.id === after.activeWorkspaceId)
          ?.tabs.find((tab) => tab.id === after.workspaces.find((ws) => ws.id === after.activeWorkspaceId)?.activeTabId);
        if (activeTab) {
          return setPerOriginZoom(syncZoom(after, webview), origin, activeTab.zoomFactor);
        }
      }
    } catch {
      /* ignore malformed URLs */
    }
    return syncZoom(after, webview);
  }),
  zoomOut: (webview) => update(set, (state) => {
    const after = stepActiveTabZoom(state, -1);
    const activeUrl = getActiveUrl(after);
    try {
      const origin = activeUrl && new URL(activeUrl).origin;
      if (origin && (origin.startsWith("http://") || origin.startsWith("https://"))) {
        const ws = after.workspaces.find((x) => x.id === after.activeWorkspaceId);
        const activeTab = ws?.tabs.find((t) => t.id === ws.activeTabId);
        if (activeTab) {
          return setPerOriginZoom(syncZoom(after, webview), origin, activeTab.zoomFactor);
        }
      }
    } catch {
      /* ignore */
    }
    return syncZoom(after, webview);
  })
}));

function update(
  set: (partial: Partial<BrowserStore>) => void,
  updater: (state: BrowserState) => BrowserState
): BrowserState {
  const current = useBrowserStore.getState().state;
  const next = updater(current);
  if (next === current) return current;

  saveBrowserState(next);
  set({ state: next, addressValue: getActiveUrl(next) });
  return next;
}
