/* eslint-disable max-lines */
// This file is a zustand dispatch table: every entry is a one-line
// `update(set, domainAction)` forwarder into the pure domain layer, plus a
// handful of UI state toggles (sidebar, panel, glance, permission request).
// Splitting it into multiple slices would add indirection without clarifying
// ownership, since all entries share the same structural shape and all route
// through the single `update` helper at the bottom of the file.
import { create, type StoreApi, type UseBoundStore } from "zustand";

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
  selectSplitTab,
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
  swapSplitPanes,
  toggleSplitPaneSide,
  setSplitPaneSide,
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
import {
  decryptCardDetails,
  detectCardBrand,
  encryptCardDetails,
  isValidExpiry,
  isValidPan,
  lastFourOf,
  type EncryptedCardPayload
} from "../domain/browser/paymentCardUtils";
import {
  createPasswordEntry,
  decryptSecret,
  isVaultUnlocked,
  lockVault,
  passwordMatchesOrigin,
  unlockVault
} from "../domain/browser/passwordVault";
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
  passwordSavePrompt: null,
  passwordVaultUnlocked: isVaultUnlocked(),
  safeBrowsingAlert: null,
  sidebarCollapsed: false,
  sidebarWidth: initialUiState.sidebarWidth ?? SIDEBAR_DEFAULT_WIDTH,
  state: initialState,
  glance: null,
  autoUpdateState: null,
  ownWindowId: null,
  windowRegistry: [],
  autofillPrompt: null,
  autofillBridgePath: null,
  paymentRequestPrompt: null,
  saveCreditcardPrompt: null,
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
  pauseDownload: async (id) => {
    try {
      const ok = await window.astraShell?.pauseDownload?.(id);
      if (!ok) {
        // Best-effort: if pause failed, mark state locally on next ingest to be corrected
        update(set, (state) => upsertDownload(state, {
          ...(state.downloads.find((d) => d.id === id) ?? {
            id,
            filename: "",
            totalBytes: 0,
            receivedBytes: 0,
            savePath: "",
            state: "progressing",
            startedAt: 0
          }),
          state: "paused"
        }));
      }
      return Boolean(ok);
    } catch {
      return false;
    }
  },
  resumeDownload: async (id) => {
    try {
      const ok = await window.astraShell?.resumeDownload?.(id);
      if (!ok) {
        update(set, (state) => upsertDownload(state, {
          ...(state.downloads.find((d) => d.id === id) ?? {
            id,
            filename: "",
            totalBytes: 0,
            receivedBytes: 0,
            savePath: "",
            state: "paused",
            startedAt: 0
          }),
          state: "progressing"
        }));
      }
      return Boolean(ok);
    } catch {
      return false;
    }
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
  selectAdjacentTab: (direction) => {
    const selected = update(set, (state) => selectAdjacentTab(state, direction));
    const ws = selected.workspaces.find((x) => x.id === selected.activeWorkspaceId);
    if (ws?.activeTabId) {
      void window.astraShell?.windowRegistry?.setFocus?.({
        spaceId: ws.id,
        patch: { activeTabId: ws.activeTabId }
      });
    }
    return selected;
  },
  selectTab: (tabId) => {
    const selected = update(set, (state) => {
      const sel = selectTab(state, tabId);
      const ws = sel.workspaces.find((x) => x.id === sel.activeWorkspaceId);
      const active = ws?.tabs.find((t) => t.id === ws.activeTabId);
      const overridden = active
        ? getZoomForUrl(sel.settings.perOriginZoom, active.url)
        : undefined;
      if (!active || overridden === undefined) return sel;
      active.zoomFactor = overridden;
      return sel;
    });
    const ws = selected.workspaces.find((x) => x.id === selected.activeWorkspaceId);
    if (ws?.activeTabId) {
      void window.astraShell?.windowRegistry?.setFocus?.({
        spaceId: ws.id,
        patch: { activeTabId: ws.activeTabId }
      });
    }
  },
  selectSplitTab: (splitId) => {
    update(set, (state) => selectSplitTab(state, splitId));
  },
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
  switchWorkspace: (workspaceId) => {
    const next = update(set, (state) => switchWorkspace(state, workspaceId));
    void window.astraShell?.windowRegistry?.setActiveSpace?.({
      spaceId: workspaceId,
      defaultActiveTabId: next.workspaces.find((w) => w.id === workspaceId)?.activeTabId ?? undefined
    });
    return next;
  },
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
  swapSplitPanes: (splitId) => update(set, (state) => swapSplitPanes(state, splitId)),
  toggleSplitPaneSide: () => update(set, toggleSplitPaneSide),
  setSplitPaneSide: (side: "left" | "right") => update(set, (state) => setSplitPaneSide(state, side)),
  setIncognito: (mode) => update(set, (state) => setIncognitoMode(state, mode)),
  setPerOriginZoom: (origin, zoom) => update(set, (state) => setPerOriginZoom(state, origin, zoom)),
  /* ===== Autofill actions ===== */
  upsertPassword: (entry) => update(set, (state) => upsertPassword(state, entry)),
  removePassword: (id) => update(set, (state) => removePassword(state, id)),
  touchPasswordUsed: (id) => update(set, (state) => touchPasswordUsed(state, id)),
  ingestPasswordSavePrompt: (prompt) => {
    const origin = prompt?.origin;
    if (!origin || !prompt?.username) return;
    // de-duplicate: a prompt with same (origin, username) is an update
    const current = get().passwordSavePrompt;
    const isDuplicate =
      current && passwordMatchesOrigin({ origin: current.origin, id: "" } as never, origin) && current.username === prompt.username;
    set({
      passwordSavePrompt: isDuplicate
        ? { ...current, password: prompt.password || current.password }
        : {
          id: prompt.id || `${Date.now()}-${Math.random().toString(16).slice(2)}`,
          origin,
          username: prompt.username,
          password: prompt.password ?? ""
        }
    });
  },
  rejectPasswordSavePrompt: (id) => {
    const current = get().passwordSavePrompt;
    if (!current || current.id === id) set({ passwordSavePrompt: null });
  },
  acceptPasswordSavePrompt: async (id) => {
    const current = get().passwordSavePrompt;
    if (!current || current.id !== id) return;
    try {
      if (!isVaultUnlocked()) await unlockVault();
      const entry = await createPasswordEntry({
        origin: current.origin,
        username: current.username,
        password: current.password,
        notes: ""
      });
      set({ passwordVaultUnlocked: true });
      update(set, (state) => upsertPassword(state, entry));
    } finally {
      set({ passwordSavePrompt: null });
    }
  },
  unlockPasswordVault: async (passphrase) => {
    try {
      await unlockVault(passphrase ?? "");
      set({ passwordVaultUnlocked: true });
    } catch (err) {
      set({ passwordVaultUnlocked: isVaultUnlocked() });
      throw err;
    }
  },
  lockPasswordVault: () => {
    lockVault();
    set({ passwordVaultUnlocked: false });
  },
  decryptPassword: async (id) => {
    const entry = get().state.settings.autofill.passwords.find((p) => p.id === id);
    if (!entry) return null;
    try {
      if (!isVaultUnlocked()) await unlockVault();
      set({ passwordVaultUnlocked: true });
      return await decryptSecret(entry.encryptedPassword);
    } catch {
      return null;
    }
  },
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
  syncSafeBrowsing: (settings) => {
    void window.astraShell?.safeBrowsing?.syncSettings?.(settings);
  },
  reportSafeBrowsingDecision: (_decision) => {
    /* 一次性 proceed 由闭包内 proceedCallback 直接记录，这里 no-op（统计埋点在 M2 尾期补） */
  },
  checkSafeBrowsingForNavigation: async (url) => {
    const result = await window.astraShell?.safeBrowsing?.checkNavigation?.(url);
    if (!result || result.allowed) return { blocked: false };
    const alert = {
      url: result.url ?? url,
      reason: result.reason ?? "unsafe",
      severity: result.severity
    };
    set({
      safeBrowsingAlert: {
        ...alert,
        proceedCallback: () => {
          useBrowserStore.getState().dismissSafeBrowsingAlert();
        }
      }
    });
    return { blocked: true, alert };
  },
  dismissSafeBrowsingAlert: () => {
    set({ safeBrowsingAlert: null });
  },
  toggleActivePictureInPicture: async (activeWebviewId) => {
    const id = typeof activeWebviewId === "number" ? activeWebviewId : undefined;
    try {
      const result = await window.astraShell?.togglePictureInPicture?.(id);
      return result ?? { success: false, reason: "no-ipc" };
    } catch (err) {
      return { success: false, reason: err instanceof Error ? err.message : String(err) };
    }
  },
  syncMediaSessionOnTabSwitch: (payload) => {
    void window.astraShell?.syncMediaSession?.(payload);
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
  /* M2.5 E-1/E-2 MV3 PoC: renderer registry 和真实 mv3 后端保持双向同步，
   * 同时 update set 两份，真实后端以 main 进程为主。
   */
  installMv3ExtensionFromFolder: async (folderPath) => {
    const result = await window.astraShell?.mv3Extensions?.installFromFolder?.(folderPath);
    if (result?.ok) await useBrowserStore.getState().reloadInstalledExtensions();
    return result ?? { ok: false, reason: "no-ipc" };
  },
  uninstallMv3Extension: async (id) => {
    const result = await window.astraShell?.mv3Extensions?.uninstall?.(id);
    if (result?.ok) await useBrowserStore.getState().reloadInstalledExtensions();
    return result ?? { ok: false, reason: "no-ipc" };
  },
  setMv3ExtensionEnabled: async (id, enabled) => {
    const backendOk = await window.astraShell?.mv3Extensions?.setEnabled?.(id, enabled);
    update(set, (state) => toggleExtension(state, id, enabled));
    await useBrowserStore.getState().reloadInstalledExtensions();
    return Boolean(backendOk);
  },
  reloadInstalledExtensions: async () => {
    const list = await window.astraShell?.mv3Extensions?.list?.();
    if (!Array.isArray(list)) return;
    update(set, (state) => {
      // 合并：已存在的扩展在真实后端里更新 enabled；未知的扩展以真实后端为准。
      const seen = new Set<string>();
      for (const real of list) {
        seen.add(real.id);
        const existing = state.extensions.find((e) => e.id === real.id);
        if (existing) {
          existing.enabled = real.enabled;
          existing.name = real.name || existing.name;
          existing.version = real.version || existing.version;
          existing.description = real.description || existing.description;
        } else {
          state.extensions.push({
            id: real.id,
            name: real.name,
            version: real.version,
            description: real.description,
            enabled: real.enabled,
            installedAt: Date.now()
          });
        }
      }
      return state;
    });
  },
  pickFolderAndInstallMv3Extension: async () => {
    const res = await window.astraShell?.mv3Extensions?.pickFolderAndInstall?.();
    if (res && !res.canceled && res.ok) {
      await useBrowserStore.getState().reloadInstalledExtensions();
    }
    return res ?? { ok: false, reason: "no-ipc" };
  },
  resetSettings: () => update(set, resetSettings),
  clearAllDownloads: () => update(set, clearAllDownloads),
  retryDownload: (url) => {
    const clean = typeof url === "string" && (url.startsWith("http://") || url.startsWith("https://") || url.startsWith("blob:") || url.startsWith("data:")) ? url : null;
    if (!clean) return;
    window.open(clean, "_blank", "noopener");
  },
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
  /* ===== M2.4 W-3 PWA install ===== */
  ingestPendingPwaInstallPrompt: (prompt) => update(set, (state) => {
    const existing = state.pendingPwaInstallPrompts.find((p) => p.origin === prompt.origin);
    if (!existing) {
      state.pendingPwaInstallPrompts.push({ ...prompt });
    } else {
      Object.assign(existing, prompt);
    }
    return state;
  }),
  dismissPendingPwaInstallPrompt: (origin) => update(set, (state) => {
    state.pendingPwaInstallPrompts = state.pendingPwaInstallPrompts.filter((p) => p.origin !== origin);
    return state;
  }),
  confirmPwaInstall: async (origin) => {
    const result = await window.astraShell?.pwaConfirmInstall?.(origin);
    update(set, (state) => {
      state.pendingPwaInstallPrompts = state.pendingPwaInstallPrompts.filter((p) => p.origin !== origin);
      return state;
    });
    return result ?? { accepted: false, reason: "no-ipc" };
  },
  ingestInstalledPwaApp: (app) => update(set, (state) => {
    const existing = state.installedPwaApps.find((p) => p.origin === app.origin);
    if (!existing) {
      state.installedPwaApps.push({ ...app });
    } else {
      Object.assign(existing, app);
    }
    return state;
  }),
  reloadInstalledPwaApps: async () => {
    const apps = await window.astraShell?.pwaListInstalled?.();
    if (Array.isArray(apps)) {
      update(set, (state) => {
        state.installedPwaApps = [...apps];
        return state;
      });
    }
  },
  launchInstalledPwa: (origin) => window.astraShell?.pwaLaunch?.(origin) ?? Promise.resolve({ ok: false, reason: "no-ipc" }),
  uninstallPwa: async (origin) => {
    const result = await window.astraShell?.pwaUninstall?.(origin);
    if (result?.ok) {
      update(set, (state) => {
        state.installedPwaApps = state.installedPwaApps.filter((a) => a.origin !== origin);
        return state;
      });
    }
    return result ?? { ok: false, reason: "no-ipc" };
  },
  /* ===== M2.5 W-10 auto-update ===== */
  setAutoUpdateState: (state) => set({ autoUpdateState: state }),
  refreshAutoUpdateState: async () => {
    const s = await window.astraShell?.autoUpdate?.getState?.();
    if (s) set({ autoUpdateState: s });
  },
  checkForUpdates: () => window.astraShell?.autoUpdate?.check?.() ?? Promise.resolve({ ok: false, reason: "no-ipc" }),
  downloadUpdate: () => window.astraShell?.autoUpdate?.download?.() ?? Promise.resolve({ ok: false, reason: "no-ipc" }),
  installUpdateAndRestart: () => window.astraShell?.autoUpdate?.installAndRestart?.() ?? Promise.resolve(false),
  /* ===== ADR-0005 / W-1 multi-window × Space registry ===== */
  setOwnWindowId: (id) => set({ ownWindowId: id }),
  syncWindowRegistry: (snapshot) => set({ windowRegistry: snapshot }),
  switchActiveSpaceForWindow: async (spaceId) => {
    // 1) 域层切换（global activeWorkspaceId 仍然更新，保持向后兼容）
    const next = update(set, (state) => switchWorkspace(state, spaceId));
    // 2) 告诉主进程：此窗口现在展示该 Space
    const ws = next.workspaces.find((w) => w.id === spaceId);
    await window.astraShell?.windowRegistry?.setActiveSpace?.({
      spaceId,
      defaultActiveTabId: ws?.activeTabId ?? undefined
    });
  },
  setActiveTabForWindow: async (spaceId, tabId) => {
    // 1) 域层：更新 canonical Space.activeTabId（所有窗口共享）
    update(set, (state) => {
      const idx = state.workspaces.findIndex((w) => w.id === spaceId);
      if (idx === -1) return state;
      const ws = { ...state.workspaces[idx], activeTabId: tabId };
      const workspaces = [...state.workspaces];
      workspaces[idx] = ws;
      return { ...state, workspaces };
    });
    // 2) 告诉主进程：此窗口此 Space 的 focus tab = tabId
    await window.astraShell?.windowRegistry?.setFocus?.({
      spaceId,
      patch: { activeTabId: tabId }
    });
  },
  openTabInNewWindow: async (spaceId, tabId) =>
    window.astraShell?.windowRegistry?.openNewWindow?.({
      spaceId,
      defaultActiveTabId: tabId
    }) ?? Promise.resolve({ ok: false, reason: "no-ipc" }),
  /* ===== ADR-0005 / P-2 autofill ===== */
  loadAutofillBridgePath: async () => {
    const p = await window.astraShell?.autofill?.getBridgePath?.();
    if (p) set({ autofillBridgePath: p });
  },
  showAutofillPopup: (event) => {
    if (!event?.detail) return;
    const { focusedBucket, fields } = event.detail;
    const s = get().state.settings.autofill;
    const matchPool = focusedBucket === "creditcard" ? (s.paymentMethods ?? []) : (s.addresses ?? []);
    /** 按 focusedBucket 生成 candidates：遍历所有已保存的地址/卡，
     *  生成 {id,label,subtitle,values} 映射。values 的 key = FIELD_TYPES.type（见 autofillContentScripts）。 */
    const matches: Array<{
      id: string; label: string; subtitle?: string; values: Record<string, string>;
    }> = matchPool.map((entry: any) => {
      if (focusedBucket === "address") {
        // AddressEntry → 映射到 FIELD_TYPES 定义的字段 key
        const e = entry as AddressEntry;
        const values: Record<string, string> = {};
        values.addressName = e.recipient || "";
        const split = splitFullName(e.recipient || "");
        values.firstName = split.first;
        values.lastName = split.last;
        values.street = e.address1 || "";
        values.street2 = e.address2 || "";
        values.city = e.city || "";
        values.state = e.region || "";
        values.zip = e.postalCode || "";
        values.country = e.country || "";
        values.phone = e.phone || "";
        values.email = e.email || "";
        return {
          id: e.id,
          label: e.label || e.recipient || e.address1,
          subtitle: [e.city, e.region, e.postalCode].filter(Boolean).join(", "),
          values
        };
      }
      // creditcard: 不暴露明文卡，仅给出 LastFour + 品牌 + 过期信息；真实填充值需要解密
      // 这里 MVP 把 cardLastFour / expiry / holderName 映射（encryption 层在 P-3 中展开）。
      const e = entry as PaymentMethodEntry;
      const values: Record<string, string> = {
        ccName: e.cardholderName || "",
        ccNumber: e.cardLastFour || "",
        ccExpMonth: String(e.expiryMonth ?? ""),
        ccExpYear: String(e.expiryYear ?? ""),
        ccExp: [
          String(e.expiryMonth ?? "").padStart(2, "0"),
          String(e.expiryYear ?? "").slice(-2)
        ].filter(Boolean).join("/")
      };
      return {
        id: e.id,
        label: e.label || `${e.cardholderName || "Card"} ••${e.cardLastFour || ""}`,
        subtitle: `••${e.cardLastFour || ""}${values.ccExp ? ` · ${values.ccExp}` : ""}`,
        values
      };
    });
    // 只保留：该 form 中至少存在一个 type 可以填（不展示完全不匹配的条目）
    const presentTypes = new Set((fields || []).map((f) => f.type));
    const filtered = matches.filter((m) =>
      Object.entries(m.values).some(([k, v]) => presentTypes.has(k) && v));
    set({
      autofillPrompt: {
        webContentsId: event.webContentsId,
        detail: event.detail,
        matches: filtered,
        offerSaveAddress: focusedBucket === "address" && presentTypes.size > 0
      }
    });
  },
  hideAutofillPopup: (wcId) => {
    const cur = get().autofillPrompt;
    if (!cur) return;
    if (wcId != null && cur.webContentsId != null && wcId !== cur.webContentsId) return;
    set({ autofillPrompt: null });
  },
  acceptAutofillMatch: async (matchId) => {
    const cur = get().autofillPrompt;
    if (!cur || typeof cur.webContentsId !== "number") return;
    const m = cur.matches.find((x) => x.id === matchId);
    if (!m) return;
    let values = m.values;
    // CC 场景：真实 PAN + CSC 需要解密。填充前检查 vault；如果当前只有
    // placeholder ccNumber=XXXX，尝试从 PaymentMethodEntry 重新解密。
    if (cur.detail.focusedBucket === "creditcard") {
      const savedEntry = (get().state.settings.autofill.paymentMethods ?? [])
        .find((p) => p.id === m.id);
      if (savedEntry?.encryptedCardDetails) {
        try {
          if (!isVaultUnlocked()) await unlockVault();
          const decrypted = await decryptCardDetails(savedEntry.encryptedCardDetails);
          if (decrypted) {
            const mm = savedEntry.expiryMonth;
            const yy = savedEntry.expiryYear;
            values = {
              ...values,
              ccName: savedEntry.cardholderName,
              ccNumber: decrypted.pan,
              ccCsc: decrypted.csc ?? "",
              ccExpMonth: mm != null ? String(mm) : "",
              ccExpYear: yy != null ? String(yy) : "",
              ccExp: [
                mm != null ? String(mm).padStart(2, "0") : "",
                yy != null ? String(yy).slice(-2) : ""
              ].filter(Boolean).join("/"),
              ccType: decrypted.brand
            };
          }
        } catch {
          /* 解密失败 → 回退到 match 里的占位值（LastFour），
           * 保证不会崩，且用户可手动修正。 */
        }
      }
    }
    await window.astraShell?.autofill?.fillForm?.({
      webContentsId: cur.webContentsId,
      values
    });
    set({ autofillPrompt: null });
  },
  saveCurrentFormAsAddress: (partial) => {
    const id = partial?.id || `addr-${Date.now().toString(36)}`;
    const entry: AddressEntry = {
      id,
      label: partial?.label || `${partial?.recipient || ""} ${partial?.address1 || ""}`.trim() || `Address ${id.slice(-4)}`,
      recipient: partial?.recipient || "",
      address1: partial?.address1 || "",
      address2: partial?.address2 || "",
      city: partial?.city || "",
      region: partial?.region || "",
      postalCode: partial?.postalCode || "",
      country: partial?.country || "",
      phone: partial?.phone || "",
      email: partial?.email || "",
      createdAt: partial?.createdAt || Date.now()
    };
    update(set, (state) => upsertAddress(state, entry));
    set({ autofillPrompt: null });
  },
  /* ===== ADR-0005 / P-3 PaymentRequest + 保存新卡 prompt ===== */
  createPaymentMethodFromDraft: async (draft) => {
    const holder = (draft.cardholderName || "").trim();
    if (!holder) return { ok: false as const, reason: "请填写持卡人姓名。" };
    const panCheck = isValidPan(draft.pan || "");
    if (!panCheck.valid) {
      return { ok: false as const, reason: "卡号无效：" + (panCheck.reason || "校验失败。") };
    }
    const mm = draft.expiryMonth;
    const yy = draft.expiryYear;
    if ((mm != null || yy != null) && !isValidExpiry(mm, yy)) {
      return { ok: false as const, reason: "有效期无效（必须为未来月份，格式 MM / YYYY）。" };
    }
    try {
      if (!isVaultUnlocked()) await unlockVault();
      const brand = panCheck.brand;
      const encrypted = await encryptCardDetails({
        pan: (draft.pan || "").replace(/\D/g, ""),
        csc: draft.csc || undefined,
        brand
      });
      const lastFour = lastFourOf(draft.pan || "");
      const entry: PaymentMethodEntry = {
        id: `card-${Date.now().toString(36)}-${Math.random().toString(36).slice(2, 6)}`,
        label: (draft.label || "").trim() || `${holder} ••${lastFour}`,
        cardholderName: holder,
        cardLastFour: lastFour,
        encryptedCardDetails: encrypted,
        expiryMonth: mm,
        expiryYear: yy,
        createdAt: Date.now(),
        updatedAt: Date.now()
      };
      return { ok: true as const, entry };
    } catch (err) {
      return {
        ok: false as const,
        reason: err instanceof Error ? err.message : String(err)
      };
    }
  },
  showPaymentRequestPrompt: (event) => {
    if (!event?.detail) return;
    const saved = get().state.settings.autofill.paymentMethods ?? [];
    const candidateCards = saved.map((p) => ({
      id: p.id,
      label: p.label || `${p.cardholderName || "Card"} ••${p.cardLastFour || ""}`,
      subtitle: `••${p.cardLastFour || ""}${p.expiryMonth ? ` · ${String(p.expiryMonth).padStart(2, "0")}/${String(p.expiryYear ?? "").slice(-2)}` : ""}`,
      cardholderName: p.cardholderName,
      cardLastFour: p.cardLastFour,
      expiryMonth: p.expiryMonth,
      expiryYear: p.expiryYear
    }));
    set({
      paymentRequestPrompt: {
        webContentsId: event.webContentsId,
        detail: event.detail,
        candidateCards
      }
    });
  },
  acceptPaymentRequestCard: async (matchId) => {
    const cur = get().paymentRequestPrompt;
    if (!cur || typeof cur.webContentsId !== "number") return;
    const saved = (get().state.settings.autofill.paymentMethods ?? [])
      .find((p) => p.id === matchId);
    let response: import("../types/electron").AutofillPaymentResponse = {
      correlationId: cur.detail.correlationId,
      canceled: true
    };
    if (saved) {
      try {
        if (!isVaultUnlocked()) await unlockVault();
        const decrypted = saved.encryptedCardDetails
          ? await decryptCardDetails(saved.encryptedCardDetails)
          : null;
        response = {
          correlationId: cur.detail.correlationId,
          canceled: false,
          paymentResponse: {
            cardholderName: saved.cardholderName,
            cardNumber: decrypted?.pan || "",
            expiryMonth: String(saved.expiryMonth ?? "").padStart(2, "0"),
            expiryYear: String(saved.expiryYear ?? ""),
            cardSecurityCode: decrypted?.csc || ""
          }
        };
      } catch { /* canceled=true → 调用方会 fallback 原生 PR */ }
    }
    await window.astraShell?.autofill?.sendPaymentResponse?.({
      webContentsId: cur.webContentsId,
      response
    });
    set({ paymentRequestPrompt: null });
  },
  rejectPaymentRequest: () => {
    const cur = get().paymentRequestPrompt;
    if (!cur || typeof cur.webContentsId !== "number") {
      set({ paymentRequestPrompt: null });
      return;
    }
    const response: import("../types/electron").AutofillPaymentResponse = {
      correlationId: cur.detail.correlationId,
      canceled: true
    };
    void window.astraShell?.autofill?.sendPaymentResponse?.({
      webContentsId: cur.webContentsId,
      response
    }).catch(() => null);
    set({ paymentRequestPrompt: null });
  },
  showSaveCreditcardPrompt: (event) => {
    if (!event?.detail?.number) return;
    // 去重：同一 lastFour + cardholderName 的已保存卡，不重复 prompt
    const saved = get().state.settings.autofill.paymentMethods ?? [];
    const lastFour = lastFourOf(event.detail.number);
    const holder = (event.detail.cardholderName || "").trim();
    if (saved.some((p) =>
      p.cardLastFour === lastFour &&
      (holder === "" || p.cardholderName.toLowerCase() === holder.toLowerCase())
    )) {
      return;
    }
    set({ saveCreditcardPrompt: event });
  },
  acceptSaveCreditcard: async (opts = {}) => {
    const cur = get().saveCreditcardPrompt;
    if (!cur) return null;
    const d = cur.detail;
    // 解析过期日：优先级 expiryMonth/expiryYear 独立字段 > expiryRaw (MM/YY)
    let mm: number | undefined;
    let yy: number | undefined;
    if (d.expiryMonth && d.expiryYear) {
      mm = Number(d.expiryMonth) || undefined;
      yy = yyRaw2Year(d.expiryYear);
    } else if (d.expiryRaw) {
      const m = d.expiryRaw.match(/(\d{1,2})\s*[/-]\s*(\d{2,4})/);
      if (m) {
        mm = Number(m[1]) || undefined;
        yy = yyRaw2Year(m[2]);
      }
    }
    const result = await get().createPaymentMethodFromDraft({
      cardholderName: d.cardholderName,
      pan: d.number,
      csc: d.cvv || undefined,
      expiryMonth: mm,
      expiryYear: yy,
      label: opts.label
    });
    if (!result.ok) return result.reason;
    update(set, (state) => upsertPaymentMethod(state, result.entry));
    set({ saveCreditcardPrompt: null });
    return null;
  },
  rejectSaveCreditcard: () => {
    set({ saveCreditcardPrompt: null });
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

/** P-2 地址自动填充：把 "张三" / "John Smith" 等全名拆成 first/last。
 *  中文：last = 首字，first = 其余；英文：按空格切，最后 token 为 last。
 */
function splitFullName(name: string): { first: string; last: string } {
  const s = (name || "").trim();
  if (!s) return { first: "", last: "" };
  // 包含 CJK 字符 → 中文模式
  if (/[\u4e00-\u9fff\u3400-\u4dbf]/.test(s)) {
    if (s.length === 1) return { first: "", last: s };
    if (s.length === 2) return { first: s[1], last: s[0] };
    // 三字及以上：单姓 + 双名。（复姓 P-3 处理，按单姓是主流启发式）
    return { first: s.slice(1), last: s[0] };
  }
  const parts = s.split(/\s+/);
  if (parts.length === 1) return { first: parts[0], last: "" };
  return { first: parts.slice(0, -1).join(" "), last: parts[parts.length - 1] };
}

/** P-3: 统一 YY → YYYY（2 位补 20xx，4 位直接取），失败返回 undefined。 */
function yyRaw2Year(raw: string | number | null | undefined): number | undefined {
  if (raw == null || raw === "") return undefined;
  const s = String(raw).replace(/\D/g, "");
  if (s.length === 2) return 2000 + Number(s);
  if (s.length === 4) return Number(s);
  const n = Number(s);
  return Number.isFinite(n) && n > 2000 ? n : undefined;
}
