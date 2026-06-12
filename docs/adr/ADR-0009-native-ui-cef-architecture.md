# ADR-0009: 浏览器架构 — 原生 UI + Chromium CEF

- Status: Accepted
- Date: 2026-06-12
- Deciders: 架构决策
- Related: ADR-0005 (Multi-window × Space sync)

## Context

Astra Browser 最初基于 Electron 架构：
- 整个浏览器 UI（侧边栏、顶栏、地址栏等）用 React + HTML/CSS 渲染
- 网页内容用 Electron `<webview>` 组件渲染
- 主进程是 Node.js，渲染进程是 Chromium renderer

随着产品发展，Electron 架构暴露出越来越多的根本性问题：

1. **扩展兼容性差**：Chrome Web Store 扩展（MV3）无法原生运行，需要自建兼容层
2. **密码管理缺失**：Chromium Password Manager 是 C++ 核心组件，Electron 未暴露 API
3. **安全浏览缺失**：Safe Browsing 需要接入 Chromium 原生模块
4. **DevTools 不一致**：Electron webview 的 DevTools 与 Chrome 原生有差异
5. **内存开销大**：Node.js + Chromium 双运行时，每个 webview 额外开销
6. **原生体验差**：HTML/CSS 渲染的浏览器 chrome 在滚动、动画、系统集成上不如原生

## Decision

迁移到 **原生 UI + Chromium CEF** 架构：

- **浏览器 Chrome**：macOS 用 AppKit / Objective-C++ 原生渲染
- **网页内容**：Chromium Embedded Framework (CEF) 渲染
- **状态模型**：C++ 原生数据模型作为唯一真相源，UI 通过 observer 订阅更新
- **多进程**：CEF 标准多进程架构（browser + renderer + GPU + utility）

### Architecture

```
┌──────────────────────────────────────────────────┐
│  Native UI Layer (AppKit / Objective-C++)         │
│  • Sidebar (NSTableView)                          │
│  • Top Bar / Address Bar                          │
│  • Window / Split View                            │
├──────────────────────────────────────────────────┤
│  Browser Core (C++ / CEF)                         │
│  • Tab / Workspace state management               │
│  • CEF App / Client lifecycle                     │
│  • Navigation / Loading                           │
├──────────────────────────────────────────────────┤
│  Chromium CEF (vendor)                            │
│  • Blink rendering engine                         │
│  • V8 JavaScript engine                           │
│  • Network stack                                  │
│  • Extensions system (optional)                   │
└──────────────────────────────────────────────────┘
```

## Rationale

### Why native UI?

1. **性能**：原生控件比 HTML/CSS 更快、更流畅，特别是滚动和动画
2. **系统集成**：更好的 macOS 集成（HIG、Services、Accessibility、Touch Bar）
3. **内存**：少了一个 V8 实例 + DOM 树来渲染浏览器 chrome
4. **Arc 风格一致**：Arc Browser 本身就是原生 UI

### Why CEF?

1. **完整 Chromium**：真正的 Blink + V8，不是 webview
2. **扩展系统**：理论上可以接入 Chromium extensions 子系统
3. **DevTools**：完整的 Chrome DevTools Protocol
4. **成熟稳定**：被 Spotify、Steam、Slack（旧版）等产品使用

### 为什么不用其他方案？

- **Electron**：扩展性/密码/安全浏览等底线能力做不了
- **WebKit/WKWebView**：不兼容 Chrome 扩展，渲染引擎差异大
- **直接用 Chromium 源码**：维护成本太高，CEF 已经做了封装
- **Qt WebEngine**：额外依赖 Qt 框架，且扩展支持有限

## Consequences

### Positive

- 可以原生接入 Chromium Password Manager、Safe Browsing、Extensions 等核心组件
- 更好的性能和更低的内存占用
- 原生级别的用户体验
- 为后续多窗口、PWA、DevTools 等功能打好基础

### Negative

- 需要维护 C++ + Objective-C++ 代码，技术栈更复杂
- UI 开发速度比 React 慢
- Windows 版本需要另外写 WinUI / MFC 实现
- 相比 Electron 生态小、文档少
- 团队需要学习 CEF 多进程编程模型

### Risks

1. **CEF 扩展集成工作量大**：CEF 默认不包含 Chrome 扩展系统，需要自己集成 Chromium extensions 模块
2. **多平台 UI 工作量**：每个平台需要单独实现 UI 层
3. **CEF 版本跟进**：需要定期跟进 CEF/Chromium 版本更新
4. **调试复杂度**：C++ + Objective-C++ + CEF 多进程调试比 JS 复杂

## Migration Path

分阶段迁移：

**Phase 0 — 脚手架**：CEF 工程搭建 + 原生 UI 基础框架 + 基础 Tab 开关
**Phase 1 — 核心功能移植**：侧边栏、分屏、拖拽、Glance 等 Arc 风格功能
**Phase 2 — Chromium 能力接入**：密码、下载、权限、历史、设置、DevTools
**Phase 3 — 差异化功能**：扩展、翻译、阅读模式、PWA、多窗口

## References

- [CEF 官方文档](https://chromiumembedded.github.io/cef/)
- [CEF 构建下载](https://cef-builds.spotifycdn.com/)
- [Arc Browser 技术栈分析](https://arc.net/)
