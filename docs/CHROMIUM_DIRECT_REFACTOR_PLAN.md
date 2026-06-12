# Chromium Direct Refactor Plan

## 结论

重构目标是 direct Chromium，不是 Electron，也不是 CEF。

Astra 应该作为 Chromium/Chrome framework 上的一层产品化浏览器，而不是在 Chromium 外面再造一套浏览器服务。原则很简单：

- Chromium 已经有的能力，直接复用：Profile、Browser、TabStripModel、WebContents、NavigationController、History、Downloads、Permissions、Password Manager、Autofill、Safe Browsing、Extensions、DevTools、Update、Policy。
- Astra 只实现 Chromium 没有的产品语义：Spaces、竖向 sidebar 信息架构、Favorites 作为 tab folder、Split/Glance、Astra command palette、Astra visual identity。
- UI 走 Chromium desktop 的 `ui/views`/`BrowserView` 体系，而不是 Electron React，也不是 CEF + AppKit 外壳。

## 目标源码布局

在 Chromium checkout 中接入 Astra：

```text
chromium/src/
  astra/
    BUILD.gn
    app/
      astra_browser_main_extra_parts.*
      astra_content_browser_client.*
      astra_main_delegate.*
    browser/
      astra_command_delegate.*
      astra_tab_features.*
      astra_workspace_service.*
    ui/views/
      astra_browser_view.*
      sidebar/astra_sidebar_view.*
  chrome/
    minimal patch points to register Astra parts
  components/
    only add Astra components when the feature is product-specific
```

本仓库保留 `chromium/astra/` 作为 overlay/template；真正构建时由脚本同步到 `chromium/src/astra`，并在 Chromium 源码里打小 patch。

## 不做什么

- 不写 CEF wrapper。
- 不写自研 `DownloadManager`、`PermissionManager`、`HistoryService`、`ExtensionService`。
- 不把 Electron renderer domain 原样搬到 C++。
- 不在平台 UI 里保存 tab/workspace 真相源。
- 不用 CMake 构建浏览器主体；direct Chromium 使用 GN/Ninja。

更多边界见 `AGENTS.md` 和 `docs/ENGINEERING_STANDARDS.md`。任何后续实现若需要突破这些边界，先补 ADR，不要直接写代码。

## Astra 应该拥有的最小产品层

### 1. Workspace service

Chromium/Chrome 没有 Arc-style Spaces，所以 Astra 需要一个 `ProfileKeyedService`：

- workspace 列表、排序、active workspace。
- workspace 与 Chromium `Browser`/`TabStripModel` 的投影关系。
- Favorites/Pinned/Groups 在 Astra sidebar 中的展示分类。
- session restore 时把 workspace 元数据附加到 Chromium tab/session。

### 2. Tab features

每个 `content::WebContents` 挂一个 Astra feature object：

- workspace id。
- Favorite folder membership。
- Split/Glance metadata。
- sidebar presentation cache。

真正导航、历史、加载状态、favicon、mute、zoom 仍然从 Chromium `WebContents`/`NavigationController`/`TabStripModel` 获取。

### 3. Views UI

基于 Chromium `BrowserView`：

- 左侧 Astra sidebar。
- 顶部/地址栏沿用或改造 Chromium toolbar。
- Split/Glance 作为 BrowserView 内容区域布局，而不是额外 webview。
- command palette 可以是 Views bubble/dialog，并复用 Chrome command infra。

### 4. Command bridge

不要另造命令系统。Astra command palette、菜单和快捷键应该调用 Chrome 已有 command/controller：

- new tab/window/incognito。
- close/reload/back/forward/devtools。
- downloads/history/settings/passwords/extensions。
- Astra-only commands 单独扩展 command id。

## Chromium patch points

后续 agent 需要在 Chromium 源码中做小而明确的 patch：

1. `chrome/browser/chrome_browser_main.cc`
   - 注册 `AstraBrowserMainExtraParts`。

2. `chrome/browser/ui/views/frame/browser_view.cc` 或对应 factory
   - 在 Astra branding/build flag 下创建 `AstraBrowserView`。

3. `chrome/browser/ui/browser_command_controller.*`
   - 增加 Astra-only command id 或转发到 `AstraCommandDelegate`。

4. `chrome/browser/profiles/*`
   - 不替换 ProfileManager，只为 Astra workspace metadata 挂 keyed service。

5. `chrome/browser/extensions`, `chrome/browser/password_manager`, `chrome/browser/safe_browsing`
   - 默认复用；只有产品策略/branding 需要 patch。

6. `chrome/app/chrome_main_delegate.*`
   - 如果需要产品级初始化，再接 `AstraMainDelegate`。

## 迁移路线

### Phase 0: 切掉错误方向

- 删除 CEF/CMake/AppKit scaffold。
- 增加 direct Chromium overlay、GN skeleton、bootstrap/build 脚本。
- ADR 改为 direct Chromium。

### Phase 1: Chromium checkout 可构建

- `scripts/chromium-bootstrap.sh` 拉取 Chromium。
- overlay 同步到 `chromium/src/astra`。
- 最小 patch 注册 `AstraBrowserMainExtraParts`。
- `autoninja -C out/astra_Debug chrome` 通过。

### Phase 2: BrowserView shell

- `AstraBrowserView` 能替换默认 BrowserView。
- sidebar view 挂到左侧，但 tab 仍由 Chrome `TabStripModel` 管。
- 不改动历史、下载、权限、扩展等组件。

### Phase 3: Workspace/Sidebar semantics

- `AstraWorkspaceService` 成为 workspace 真相源。
- `AstraTabFeatures` 绑定 WebContents 到 workspace/favorite/split metadata。
- Electron domain 中 P0 tab identity 语义迁移为 Chromium unit/browser tests。

### Phase 4: Astra feature parity

- Split/Glance 基于 WebContents layout 和 Views 实现。
- command palette 接 Chrome command infra。
- 设置/历史/下载/扩展/密码页面优先复用 Chrome WebUI，再做 Astra UI 定制。

### Phase 5: Retire Electron

- Electron 代码只在迁移完成后删除。
- 发布链路切到 Chromium GN/Ninja、Chromium signing/notarization flow。

## Agent 分工

| Agent | 目录 | 职责 |
| --- | --- | --- |
| Chromium build | `scripts/`, `chromium/astra/BUILD.gn` | checkout、GN args、overlay sync、first build |
| Patch points | `chromium/astra/app`, Chromium `chrome/` patch | main parts、content browser client、branding flag |
| UI Views | `chromium/astra/ui/views` | BrowserView/sidebar/split/glance shell |
| Workspace | `chromium/astra/browser/astra_workspace_service.*` | ProfileKeyedService、workspace metadata、session bridge |
| Tab features | `chromium/astra/browser/astra_tab_features.*` | WebContentsUserData、favorite/split/glance metadata |
| Commands | `chromium/astra/browser/astra_command_delegate.*` | Chrome command bridge、Astra-only commands |
| QA | Chromium tests | unit/browser/ui smoke tests |

## 验收标准

- 没有 CEF 依赖。
- 新主线没有 Electron runtime 入口。
- 代码骨架挂在 Chromium framework 概念上，而不是自研浏览器服务。
- 所有非 Astra 差异化能力都明确标注“复用 Chromium”。
- 后续 agent 可以从 `chromium/astra/` 开始在 Chromium checkout 中实现。
