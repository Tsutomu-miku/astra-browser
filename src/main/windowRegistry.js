/**
 * ADR-0005 Accepted / W-1, W-2: 多窗口 × Space 状态同步中心。
 *
 * 【背景】PRD §4.3 第 17 项 & §6 风险 #7：
 *  - 当前 Astra 的 browserStore state 是全局单例，同一个 Space 在两个不同窗口中
 *    selectTab 后会互相覆盖 activeTabId；
 *  - 多窗口布局、每窗口每个 Space 的 Split focus、glance 状态、侧栏折叠状态、
 *    find-in-page 状态等必须按"窗口 × Space"维度隔离；
 *  - Tab/Group/Favorite 等"对象数据"仍然全局共享（只有一份 canonical source），
 *    只有"UI 选择状态"按窗口隔离。
 *
 * 【模型】
 *   WindowRegistry (主进程)  = {
 *     windows: Map<windowId, {
 *       browserWindow: BrowserWindow,
 *       activeSpaceId: string,   // 该窗口当前展示哪个 Space
 *       spaceFocus: Record<spaceId, {
 *         activeTabId: string, splitFocusTabId?: string,
 *         glance?: {title,url}, findOpen?: boolean, panel?: PanelKind
 *       }>
 *     }>
 *   }
 *
 *   每次创建新 BrowserWindow → registerWindow()；
 *   每次 renderer 通过 IPC 调用 domain action → IPC handler 先取 sender 的 windowId，
 *     再根据 windowId + 该窗口的 activeSpaceId 路由到正确的 state 切片。
 *
 *   主进程作为唯一真相源，所有窗口的"窗口本地状态"变更都会通过
 *     `window-state:sync` IPC 广播给所有 renderer，每个 renderer 只更新自己
 *     关心的窗口条目（因为一个 renderer 只属于一个 window）。
 *
 * 【广播协议】
 *   主进程 → 所有 renderer：channel="window-registry:sync"，payload=完整 registry 快照
 *   renderer → 主进程："window-registry:set-active-space" (windowId, spaceId)
 *   renderer → 主进程："window-registry:set-focus" (windowId, spaceId, focusPatch)
 */

const { BrowserWindow, ipcMain } = require("electron");

/**
 * @typedef {{
 *   activeTabId: string,
 *   splitFocusTabId?: string | null,
 *   glance?: {title:string,url:string}|null,
 *   panel?: string|null,
 *   findOpen?: boolean
 * }} SpaceFocusState
 */

/**
 * @typedef {{
 *   windowId: number,
 *   bounds: {x:number,y:number,width:number,height:number},
 *   isMaximized: boolean,
 *   isFullScreen: boolean,
 *   activeSpaceId: string,
 *   spaceFocus: Record<string, SpaceFocusState>
 * }} WindowRegistryEntry
 */

const registry = new Map();
/** @type {Map<string, Array<number>>} reverse map spaceId -> windowIds displaying that space */
const spaceToWindows = new Map();
/** @type {Set<() => void>} change listeners used internally (for persistence, diagnostics) */
const changeListeners = new Set();

function associateSpace(windowId, spaceId) {
  if (!spaceToWindows.has(spaceId)) spaceToWindows.set(spaceId, []);
  const list = spaceToWindows.get(spaceId);
  if (!list.includes(windowId)) list.push(windowId);
}

function disassociateSpace(windowId, spaceId) {
  const list = spaceToWindows.get(spaceId);
  if (!list) return;
  const idx = list.indexOf(windowId);
  if (idx >= 0) list.splice(idx, 1);
  if (list.length === 0) spaceToWindows.delete(spaceId);
}

function onChange() {
  for (const fn of changeListeners) {
    try { fn(); } catch { /* ignore listener errors */ }
  }
  broadcastSync();
}

/** 把当前 registry 快照广播给所有 renderer。 */
function broadcastSync() {
  const payload = serialize();
  for (const win of BrowserWindow.getAllWindows()) {
    try {
      if (!win.isDestroyed()) win.webContents.send("window-registry:sync", payload);
    } catch { /* ignore */ }
  }
}

/**
 * 注册一个新 BrowserWindow。
 * @param {Electron.BrowserWindow} browserWindow
 * @param {{defaultSpaceId: string, restoredActiveSpaceId?: string, restoredSpaceFocus?: Record<string, SpaceFocusState>}} opts
 */
function registerWindow(browserWindow, { defaultSpaceId, restoredActiveSpaceId, restoredSpaceFocus = {} }) {
  const id = browserWindow.id;
  const activeSpaceId = restoredActiveSpaceId || defaultSpaceId;
  registry.set(id, {
    browserWindow,
    activeSpaceId,
    spaceFocus: { ...restoredSpaceFocus }
  });
  associateSpace(id, activeSpaceId);

  browserWindow.once("closed", () => {
    const entry = registry.get(id);
    if (entry) disassociateSpace(id, entry.activeSpaceId);
    registry.delete(id);
    onChange();
  });

  // bounds changes → re-serialize (needed for session restore persistence)
  for (const evt of ["resize", "move", "maximize", "unmaximize", "enter-full-screen", "leave-full-screen"]) {
    browserWindow.on(evt, () => void schedulePersist());
  }

  onChange();
  return id;
}

/**
 * 切换某个窗口当前展示的 Space（由 renderer 触发）。
 * @param {number} windowId
 * @param {string} spaceId
 * @param {string} [defaultActiveTabId] 当该 Space 从未在此窗口展示过时，使用此默认 activeTab
 */
function setActiveSpace(windowId, spaceId, defaultActiveTabId) {
  const entry = registry.get(windowId);
  if (!entry) return false;
  if (entry.activeSpaceId === spaceId) return true;
  disassociateSpace(windowId, entry.activeSpaceId);
  entry.activeSpaceId = spaceId;
  associateSpace(windowId, spaceId);
  // 若此窗口从未聚焦过这个 Space，用传入的默认 tabId 占位，保证 UI 不崩
  if (!entry.spaceFocus[spaceId] && defaultActiveTabId) {
    entry.spaceFocus[spaceId] = { activeTabId: defaultActiveTabId };
  }
  onChange();
  return true;
}

/**
 * 合并更新某个 Space 的聚焦状态（选 Tab、切 Split focus、打开 Glance/Panel/Find）。
 */
function setSpaceFocus(windowId, spaceId, patch) {
  const entry = registry.get(windowId);
  if (!entry) return false;
  if (!entry.spaceFocus[spaceId]) {
    entry.spaceFocus[spaceId] = { activeTabId: patch?.activeTabId ?? "" };
  }
  entry.spaceFocus[spaceId] = { ...entry.spaceFocus[spaceId], ...patch };
  onChange();
  return true;
}

/** 获取某个窗口当前应该激活的 tabId（优先用 registry 中的 window-scoped 值，fallback 到 canonical Space.activeTabId）。 */
function resolveActiveTabId(windowId, spaceId, canonicalActiveTabId) {
  const entry = registry.get(windowId);
  if (!entry) return canonicalActiveTabId;
  return entry.spaceFocus[spaceId]?.activeTabId ?? canonicalActiveTabId;
}

/** 取展示某 Space 的所有 windowId（用于：关闭 Tab 时通知所有展示该 Space 的窗口）。 */
function getWindowsDisplayingSpace(spaceId) {
  return spaceToWindows.get(spaceId) ?? [];
}

/** 取某个 spaceId 的"权威 activeTabId"。
 * 规则：若只有一个窗口展示该 Space → 直接用该窗口记录的值；
 *       若多个窗口展示 → 取最后一次 setSpaceFocus 的窗口的值（因为该 Space 对象本身
 *         canonical 的 activeTabId 在域层独立维护，registry 只是窗口视图）。
 */
function getLastFocusedTabId(spaceId, fallback) {
  const ids = getWindowsDisplayingSpace(spaceId);
  if (ids.length === 0) return fallback;
  for (let i = ids.length - 1; i >= 0; i--) {
    const entry = registry.get(ids[i]);
    const tabId = entry?.spaceFocus[spaceId]?.activeTabId;
    if (tabId) return tabId;
  }
  return fallback;
}

/** 为会话恢复/持久化序列化所有窗口。 */
function serialize() {
  const result = [];
  for (const [windowId, entry] of registry) {
    if (entry.browserWindow.isDestroyed()) continue;
    try {
      const b = entry.browserWindow.getBounds();
      result.push({
        windowId,
        bounds: { x: b.x, y: b.y, width: b.width, height: b.height },
        isMaximized: entry.browserWindow.isMaximized(),
        isFullScreen: entry.browserWindow.isFullScreen(),
        activeSpaceId: entry.activeSpaceId,
        spaceFocus: entry.spaceFocus
      });
    } catch { /* ignore */ }
  }
  return result;
}

/* ========== Persistence ========== */

let persistTimer = null;
let persistTarget = null;

function schedulePersist() {
  if (persistTimer) return;
  persistTimer = setTimeout(() => {
    persistTimer = null;
    if (persistTarget) persistTarget(serialize());
  }, 500);
}

/** 传入持久化回调。通常在 main.js 里挂到 app.whenReady 后。 */
function installPersistence(saveFn) {
  persistTarget = saveFn;
  changeListeners.add(schedulePersist);
}

/* ========== IPC Handlers (call once from main.js after registry module loads) ========== */

function installIpc() {
  ipcMain.handle("window-registry:get", (event) => {
    const id = BrowserWindow.fromWebContents(event.sender)?.id;
    return {
      ownWindowId: id ?? null,
      registry: serialize()
    };
  });
  ipcMain.handle("window-registry:set-active-space", (event, { spaceId, defaultActiveTabId }) => {
    const id = BrowserWindow.fromWebContents(event.sender)?.id;
    if (id == null) return { ok: false, reason: "unknown-window" };
    return { ok: setActiveSpace(id, spaceId, defaultActiveTabId) };
  });
  ipcMain.handle("window-registry:set-focus", (event, { spaceId, patch }) => {
    const id = BrowserWindow.fromWebContents(event.sender)?.id;
    if (id == null) return { ok: false, reason: "unknown-window" };
    return { ok: setSpaceFocus(id, spaceId, patch) };
  });
  // 跨窗口 Tab 拖拽：源窗口在 renderer 发起 drag-tab-to-new-window IPC，
  // 主进程创建新 BrowserWindow 并把该 Tab 迁移过去。Domain 层实际迁移
  // Tab 数据；这里只负责创建窗口 + 初始路由。
  ipcMain.handle("window-registry:open-new-window", (event, { spaceId, defaultActiveTabId }) => {
    // main.js 需通过 setCreateWindowFn 注入 BrowserWindow 创建器（因为 BrowserWindow
    // 需要 preload + 尺寸等 app 级配置）。
    const opts = { defaultSpaceId: spaceId, defaultActiveTabId };
    if (!createWindowFn) return { ok: false, reason: "no-creator" };
    const newWin = createWindowFn(opts);
    return { ok: true, windowId: newWin?.id ?? null };
  });
}

let createWindowFn = null;
function setCreateWindowFn(fn) {
  createWindowFn = fn;
}

module.exports = {
  // lifecycle
  registerWindow,
  installIpc,
  installPersistence,
  setCreateWindowFn,
  // mutations
  setActiveSpace,
  setSpaceFocus,
  // queries
  resolveActiveTabId,
  getWindowsDisplayingSpace,
  getLastFocusedTabId,
  serialize,
  broadcastSync
};
