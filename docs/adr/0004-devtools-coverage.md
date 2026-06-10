# ADR-0004: DevTools Coverage (E-4)

Status: Accepted
Date: 2026-06-10
Author: M2.5 delivery

## Context
Chromium/Arc 语义里 DevTools 至少需要覆盖 5 层：
1. Electron 主进程（通过 `--inspect` CLI 启动，外部 Chrome DevTools 连接）
2. BrowserWindow 主渲染进程
3. 每个 `<webview>` Tab
4. Split View 中每个 pane（实际也是 `<webview>`，走 3）
5. Glance 面板（`data:` URL 形式，渲染在主 renderer iframe 中，走 2）

此外用户的肌肉记忆是：
- Cmd/Ctrl+Shift+I / F12 → 当前活跃内容的 DevTools
- 地址栏按钮 / App Menu → 打开应用级 DevTools（主渲染器）

## Decision
三层架构，全部经过 `ipcMain.handle("toggle-devtools", webContentsId)` 统一路由：

- **主渲染器 DevTools**：`window.astraShell.toggleDevTools()` 不传 webContentsId，在 ipcHandlers 里用 `BrowserWindow.fromWebContents(sender)` 定位到窗口。
- **活跃 Tab/Split DevTools**：renderer 端 `useBrowserActions.toggleActiveDevTools(activeWebview)` 把 webview 自身的 `getWebContentsId()` 传过去，ipcHandlers 用 `webContents.fromId(id)` 直接打开对应 DevTools。
- **Glance DevTools**：Glance 的 HTML 通过 `data:` URL 注入到主窗口的 iframe 里，共用主渲染进程，所以走主渲染器 DevTools 即可。
- **主进程 DevTools**：不在 UI 中提供按钮；开发模式通过 `ELECTRON_OBSERVE=5858` / `--inspect=5858` 环境变量启用，ADR 在 README 中说明。

## Rationale
- 统一入口避免用户被多个 "toggle devtools" 菜单混淆。
- 通过 `webContentsId` 路由比 `BrowserWindow.getFocusedWindow().webContents` 更可靠（Split View 和 Glance 焦点会落在主窗口，但用户真正想 inspect 的是活跃 tab 的 webview）。
- `astra://flags` 实验开关页（E-10）可以复用相同的 DevTools 入口，而无需新建面板。

## Consequences
- 所有未来新增的 WebContents（PWA 窗口、Guest 窗口、无痕窗口）在 `createWindow()` 路径上自动被 diagnostics.js 接管，无需单独接入。
- `WebviewElement` 类型需稳定暴露 `getWebContentsId?: () => number` — 已在 `browser-ui.ts` 定义。
