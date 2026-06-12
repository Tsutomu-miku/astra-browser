# Product Requirement Document — Astra Browser

> Rev. 2026-06-12. 架构决策：从 Electron 迁移到 **Chromium 原生 CEF 架构**，彻底摆脱 Electron 的进程模型限制、扩展兼容瓶颈、内存开销与安全边界问题。
> 核心结论：当前 Astra 距离"日常可驱动（daily-driver）浏览器"至少还有 **6–9 个月的 P0/P1 工作量**，主要差距集中在 **CEF 集成、密码自动填充、安全浏览、扩展兼容、窗口会话管理、PWA、翻译/阅读模式** 等浏览器底线能力。

## 1. Product Intent（产品定位）

Astra 是一款面向 **重度知识工作者（开发者、创作者、研究者、运营）** 的桌面浏览器，借鉴 Arc Browser 的 **垂直空间化组织（Spaces + 垂直侧边栏 + Split View + 预览）** 哲学，基于 **Chromium 原生 CEF 架构** 构建，同时补齐 Chrome 级别的 **日常驱动底线能力（密码/历史/下载/设置/权限/安全/扩展/DevTools/PDF/翻译/阅读模式）**。

**不做：** 移动端优先、社交化浏览器、广告流量变现、隐私币经济模型、跨端全家桶式账号体系（早期阶段）。

**设计原则：**

1.  **对象语义统一（Object Identity First）：** 同一个 Tab/Favorite/Group，无论是在侧边栏、命令栏、开始页、上下文菜单还是历史中，行为必须一致。
2.  **Chrome 底线不打折：** 默认情况下，Chrome 稳定版中一个普通用户能做的日常操作，Astra 也必须能做。Arc 风格的差异化只能 **叠加** 在 Chrome 底线之上，不能替代。
3.  **Chromium 原生能力优先：** 优先使用 Chromium 原生能力（Password Manager、Safe Browsing、DevTools Protocol、Extensions、PDF Viewer），而不是在 UI 层重新造轮子。
4.  **安全与隐私默认不妥协：** HTTPS、权限弹窗、下载安全扫描、第三方 Cookie 隔离、一次性权限、后台摄像头/麦克风告警，不做"后续版本再加"。
5.  **键盘优先 / 鼠标可用：** 所有 Chrome 原生键盘行为（快捷键、焦点、上下文菜单键）必须兼容，Arc 风格的命令栏在这之上叠加效率入口。
6.  **单进程模型精简内存：** 利用 CEF 的灵活进程配置，在保证稳定性的前提下尽可能降低内存占用（相比 Electron 减少 30%+）。

---

## 1.1 架构决策：Electron → Chromium CEF

### 1.1.1 为什么离开 Electron

| 问题 | 影响 | 严重度 |
| --- | --- | --- |
| Electron webview 基于 `<webview>` tag，本质是 offscreen BrowserView，进程模型受限 | 内存开销大、扩展 API 不兼容、DevTools 一致性差 | P0 |
| Node.js 主进程 + Chromium renderer 的双运行时模型 | 安全边界复杂、IPC 性能损耗、密码存储需要额外 native 模块 | P0 |
| Chrome Web Store 扩展（MV3）几乎无法原生兼容 | 需要自建兼容层，工程量 ≈ 3 个 Milestone | P0 |
| Password Manager / Safe Browsing 等 Chromium 核心组件未暴露 | 需要自实现或通过原生模块接入，风险高 | P0 |
| Electron 版本追 Chromium 滞后 4–8 周 | 安全补丁延迟、新特性无法及时跟进 | P1 |
| 打包体积大（150MB+） | 用户下载成本高 | P2 |

### 1.1.2 CEF 架构优势

- **原生 Chromium**: 直接使用 Chromium 源码，Password Manager / Safe Browsing / Extensions / DevTools Protocol 全部原生可用
- **进程模型灵活**: 可配置单进程/多进程，按需优化内存
- **C++ 原生层 + JS UI 层**: 原生层负责浏览器核心能力，React UI 层负责 Arc 风格交互，职责清晰
- **直接接入 Chrome Web Store**: Chromium 原生扩展系统支持 MV3
- **更小的打包体积**: 相比 Electron 减少约 30%

### 1.1.3 架构分层

```
┌─────────────────────────────────────────┐
│  UI Layer (React / TypeScript)          │  侧边栏 / 顶栏 / 命令面板 / 分屏 / 预览
│  src/ui/                                │  纯渲染 + 状态管理（Zustand）
├─────────────────────────────────────────┤
│  Browser Core (C++ / CEF)               │  Tab / Window / Navigation / Extensions
│  src/browser/                           │  Password Manager / Safe Browsing / Downloads
│                                         │  DevTools / Permissions / Profiles
├─────────────────────────────────────────┤
│  Chromium CEF (vendor/)                 │  libcef + 资源 + 本地化
└─────────────────────────────────────────┘
```

### 1.1.4 JS ↔ Native 通信机制

- **UI → Native**: CEF V8 扩展（`window.astra`）暴露浏览器 API
- **Native → UI**: CEF 消息路由器（CefMessageRouter）异步事件
- **同步模式**: 状态变更由 Native 侧 push，UI 侧订阅更新（类似 Flux 单向数据流）

### 1.1.5 迁移策略

**Phase 0 — 脚手架**：CEF 工程搭建 + React UI 嵌入 + 基础 Tab 开关
**Phase 1 — 核心功能移植**：Spaces / Split View / 侧边栏 / 命令栏 / 拖拽
**Phase 2 — Chromium 能力接入**：密码 / 下载 / 权限 / 历史 / 设置页 / DevTools
**Phase 3 — 扩展与高级功能**：Chrome 扩展 / 翻译 / 阅读模式 / PWA / 多窗口

## 2. User Profile & Typical Workflows

### 2.1 Persona A — 日常驱动用户（Daily Driver）

-   打开 20–60 个 Tab，分成 3–5 个 Space；
-   每日使用：密码填充、下载、书签管理、历史搜索、设置页面、Chrome 扩展 3–8 个、打印、PDF 阅读、多语言页面翻译；
-   痛点：当前 Astra 因为缺少 1 项就会切回 Chrome 做那件事 → 最终流失。

### 2.2 Persona B — 知识工作者（Knowledge Worker）

-   在 Persona A 之上，重度使用 Split View（2–4 窗）、预览（Peek）、分屏对比、Pinned 看板页、最近关闭恢复、按主题分组；
-   痛点：Chrome 的 Tab 管理太原始，但 Arc 的 AI 功能又过重/贵。

### 2.3 Persona C — 前端开发者（Seed 用户典型种子）

-   在 Persona B 之上，必须能用 DevTools（Elements/Console/Network/Performance/Lighthouse），PWA 安装、源地图调试、Service Worker 检查、网络限速、设备模式；
-   痛点：目前 Electron 应用必须暴露原生 DevTools 入口 + 原始快捷键。

## 3. Feature Inventory — Arc/Chrome 对照表

> 分类与 Agent 调研输出一致；"Astra 现状"列是对当前代码的诚实盘点。优先级规则见 3.4。

### 3.1 导航 / 命令 / 搜索

| # | 能力 | 来源 | 日驱阈值 | Astra 现状 | 优先级 |
| --- | --- | --- | --- | --- | --- |
| N-1 | 地址栏：URL 解析、自动补全、搜索建议、实时答案（单位/计算器） | Chrome | Y | 部分（omnibox 已有，无补全搜索建议） | P1 |
| N-2 | Tab-to-Search（键入站点域名后 Tab 直搜站内） | Chrome | M | 无 | P2 |
| N-3 | @tab / @note / @history 等地址栏上下文搜索增强 | Chrome (131+) | M（>40 标签） | 无 | P2 |
| N-4 | 命令栏（Command Bar）：全局快捷键、搜索、跳转、执行动作（新建、重命名、休眠、切 Space…） | Arc | M（Arc 定位核心，Chrome 可选） | 部分（已有命令栏，覆盖面不全） | P1 |
| N-5 | 侧边栏搜索：跨 Section 过滤（Essentials/Pinned/Favorites/Groups/Tabs） | Arc | Y | 有 | P0 已覆盖 |
| N-6 | 开始页搜索：直接打开 Tab/Favorite、不替换 URL | Arc | M | 部分 | P1 |
| N-7 | Arc Search / AI 搜索（事实类直答，不跳转结果页） | Arc Max | N（付费差异化，远期） | 无 | P2+ |
| N-8 | 自然语言动作（"把这个 Space 里的 GitHub 标签归为一组"） | Arc Max | N | 无 | P2+ |
| N-9 | 内联搜索（Find In Page，含正则、高亮、计数） | Chrome | Y | 部分（无 Electron webview.find 集成） | P1 |

### 3.2 Tab / Space / Group

| # | 能力 | 来源 | 日驱阈值 | Astra 现状 | 优先级 |
| --- | --- | --- | --- | --- | --- |
| T-1 | Tab 新建 / 关闭 / 切换 / 复制 / 固定 / 静音 | Chrome | Y | 有 | P0 |
| T-2 | 多 Tab 快捷关闭：关闭其他 / 关闭右侧 / 关闭左侧 | Chrome | M (>20 标签) | 命令栏+菜单部分覆盖，右键需补全 | P1 |
| T-3 | 标签搜索（全局 Tab Search，Ctrl/Cmd+Shift+A） | Chrome | M (>20 标签) | 命令栏部分覆盖 | P1 |
| T-4 | Tab 悬停卡片 + 缩略预览 | Chrome | M | 无 | P1 |
| T-5 | 最近关闭标签 / 窗口（≥15 条） | Chrome | Y | 有（recently closed 部分，窗口缺失） | P1 |
| T-6 | Tab 自动丢弃 / 后台闲置释放内存 | Chrome | M (Electron 必做 Y) | 有（Memory Saver） | P0 已覆盖 |
| T-7 | 单 Tab RAM/CPU 用量可视 + 任务管理器 | Chrome + Arc ATC | M (Electron 必做 Y) | 无 | P1 |
| T-8 | Spaces：按主题隔离域，独立收藏/历史/快捷方式 | Arc | Y | 有 | P0 |
| T-9 | Spaces 多窗口（一个 Space 开多个窗口） | Chrome | M (>1 显示器用户) | 无 | P1 |
| T-10 | Space 模板（学生/远程/作者/研究预设） | Arc | N | 无 | P2 |
| T-11 | 共享协作 Spaces / Teams 企业管理 | Arc Teams | N | 无 | P2+ |
| T-12 | 垂直侧边栏：Essentials/Pinned/Favorites/Groups/Tabs/RecentlyClosed | Arc | Y | 有 | P0 已覆盖 |
| T-13 | 侧边栏收起（纯图标态）与拖拽宽度 | Arc/Chrome | Y | 有（宽度 hoist 到 body 刚合入） | P1 已覆盖 |
| T-14 | Tab 组：命名 / 配色 / 折叠 / 展开 / 整体关闭 / 整体休眠 | Chrome+Arc | M (>15 标签) | 有 | P1 已覆盖 |
| T-15 | Tab 组自动关闭策略（定时/30 天未活跃） | Chrome 135 | N | 无 | P2 |
| T-16 | 标签组同步（跨桌面登录设备） | Chrome | N（需同步生态后） | 无 | P2 |
| T-17 | AI 自动分组 / 自动整理 Favorites / 自动重命名标签 | Chrome+Arc Max | N | 无 | P2+ |
| T-18 | Tab Stashes（手动+按频率自动藏匿） | Arc | M（重度用户） | 无（部分可由 sleep 覆盖） | P1 |
| T-19 | 标签自定义重命名（Tab 标题不好辨认时手动改） | Arc | M | 有 | P1 已覆盖 |
| T-20 | 实时文件夹（Live Folders：一个入口打开 Split 多站点） | Arc | M | 无 | P2 |
| T-21 | Tab 拖拽语义一致：重排 / 进 Favorite / 进 Pinned / 进 Group / 跨 Space / 到 Split / 到 New Space | Chrome+Arc | Y | 有 | P0 已覆盖 |
| T-22 | 标签睡眠保护（Active/Pinned/Split 不被批量睡眠） | Chrome+Arc ATC | Y | 有 | P0 已覆盖 |
| T-23 | Tab 拖拽到系统窗口 / 其他应用的链接共享 | Arc | M | Electron 原生可用，需验证 | P1 |
| T-24 | 标签组与组内 guide line 视觉连续 | Arc 风格 | Y | 已修复（本 PR 合入） | P1 已覆盖 |

### 3.3 Split View / Peek / Little Arc

| # | 能力 | 来源 | 日驱阈值 | Astra 现状 | 优先级 |
| --- | --- | --- | --- | --- | --- |
| S-1 | 二窗 Split View（左右/上下，可拖拽比例）—— 独立实体模型 | Arc+Chrome | M（知识工作者 Y） | **已完成（P0）** — SplitTab 一等实体，侧边栏单行展示，独立 active 状态 | P1 已完成 |
| S-2 | 三/四窗 Split（2025 Arc 标准） | Arc 2.0 | N | 无 — 可基于实体模型扩展 | P2 |
| S-3 | 按 Tab Group 一键分屏（2–4 个标签平铺） | Arc | M | 无 | P2 |
| S-4 | Glance / Peek 预览（Alt+hover：查看链接，不打断上下文） | Arc | M（Arc 风格刚需 Y） | 有（Glance 已实现） | P1 |
| S-5 | Peek 可交互（填表、播放媒体、跳转链接） | Arc 2.0 | M | 待评估 Glance 是否支持 | P1 |
| S-6 | Peek Stacks（多个预览折叠成临时研究合集） | Arc 2.0 | N | 无 | P2 |
| S-7 | Little Arc（全局迷你悬浮窗、边缘吸附、可缩放） | Arc | M（效率加分项） | 无 | P2 |
| S-8 | Little Workflows（保存常用 Little 窗，一键唤起） | Arc | N | 无 | P2+ |

### 3.4 笔记 / 白板（Easel / Notes）/ Boost

| # | 能力 | 来源 | 日驱阈值 | Astra 现状 | 优先级 |
| --- | --- | --- | --- | --- | --- |
| B-1 | 每个标签页可关联笔记（Markdown/富文本），侧边打开不打断 | Arc | M（知识管理定位 Y） | 无 | P1 |
| B-2 | Easel 白板：便签/图片/文本/嵌入 Tab，可协作 | Arc | N（差异化大、工作量大） | 无 | P2+ |
| B-3 | Boost：可视化隐藏站点元素、改色、改字体、重排 | Arc | M（广告/布局强迫症用户） | 无 | P2 |
| B-4 | Boost CSS/JS 注入 + 条件触发（时段/子路径） | Arc | N（开发者小众） | 无 | P2 |
| B-5 | AI Boost 生成（自然语言→规则） | Arc Max | N | 无 | P2+ |
| B-6 | Boost 市场（站点预设库） | Arc | N（需审核机制） | 无 | P2+ |

### 3.5 AI 套件（Arc Max 对标）

| # | 能力 | 来源 | 日驱阈值 | Astra 现状 | 优先级 |
| --- | --- | --- | --- | --- | --- |
| A-1 | 页面 / 长文 / PDF / 视频总结（带时间戳跳转） | Chrome+Arc | M（2025 用户期望值上升，卖点） | 无 | P1（可接第三方 API） |
| A-2 | 100+ 语言翻译，保留文档/页面结构 | Chrome+Arc | M（海外产品 Y，国内 M） | 无 | P1 |
| A-3 | 标签 / 下载物 / Favorites / 组自动命名 | Arc Max | N | 无 | P2 |
| A-4 | 自动按主题整理 Favorites / 历史 / Groups | Arc Max | N | 无 | P2 |
| A-5 | 结构化抽取（商品/招聘/表格 → CSV/Sheets） | Arc Max | N | 无 | P2+ |
| A-6 | AI 自动执行多步工作流（填表、登录） | Arc Max | N（需安全/审计模型） | 无 | P2+ |
| A-7 | 上下文感知草稿生成（回复邮件、社媒评论） | Arc Max | N | 无 | P2+ |
| A-8 | 本地 LLM 离线模式（隐私场景） | Arc 2025 | N（需硬件门槛、模型体积） | 无 | P2+ |

### 3.6 密码 / 自动填充 / 支付

| # | 能力 | 来源 | 日驱阈值 | Astra 现状 | 优先级 |
| --- | --- | --- | --- | --- | --- |
| P-1 | 密码保存、填充、搜索、查看、编辑、删除 | Chrome | Y | 无（Electron 层需接 Chromium Password Store） | P0 |
| P-2 | 多份地址 + 智能分字段自动填充 | Chrome | Y | 无 | P0 |
| P-3 | 信用卡/借记卡 + Google Pay/Apple Pay 支付意图确认 | Chrome | Y | 无 | P1 |
| P-4 | 密码健康仪表盘（重复/弱/泄露扫描，一键修复） | Chrome | M | 无 | P1 |
| P-5 | 跨浏览器密码导入（Safari/Edge/1Password） | Chrome | M | 无 | P1 |
| P-6 | Passkey（通行密钥，默认创建替代密码，端到端加密同步） | Chrome 130+ | M（趋势） | 无 | P2 |
| P-7 | AI 非标字段填充（自定义备注、会员 ID、注册问题） | Chrome 135 | N | 无 | P2+ |
| P-8 | 生物识别解锁密码库（Touch ID / Windows Hello） | Chrome | M | 无 | P1 |

### 3.7 安全 / 隐私 / 权限

| # | 能力 | 来源 | 日驱阈值 | Astra 现状 | 优先级 |
| --- | --- | --- | --- | --- | --- |
| K-1 | 强制 HTTPS + 不安全站点拦截警告 | Chrome | Y | Chromium 默认，需显式启用 UI | P0 |
| K-2 | 权限弹窗（摄像头/麦克风/定位/剪贴板/通知/联系人） | Chrome | Y | 已有 preload 通道占位，UI 未接入 | P0 |
| K-3 | 一次性临时权限（10 分钟闲置自动回收） | Chrome 131+ | M（隐私合规趋势） | 无 | P1 |
| K-4 | 90 天未访问站点权限自动重置 + 月度摘要通知 | Chrome 131+ | N（可延后） | P2 |
| K-5 | 站点设置总控（origin 覆盖所有权限，可搜索） | Chrome | Y | 无（需设置页） | P0 |
| K-6 | 默认增强型安全浏览（Enhanced Safe Browsing）：钓鱼/恶意扩展前置扫描 | Chrome | Y（默认必开） | 无（需接 Safe Browsing API） | P0 |
| K-7 | 深伪 / 伪造媒体检测预警 | Chrome 135 | N（需额外模型） | 无 | P2+ |
| K-8 | 扩展权限护栏（新装扩展申请过宽权限时弹窗警告） | Chrome 131 | M | 无 | P2 |
| K-9 | 第三方 Cookie 默认禁用 + Privacy Sandbox 控制面板 | Chrome 135 | M（合规） | Chromium 默认大部分，需 UI 开关 | P1 |
| K-10 | 指纹识别防护（Canvas/AudioContext 指纹阻断） | 各类隐私浏览器 | M | 无 | P1 |
| K-11 | 后台调用摄像头/麦克风实时告警 + 一键撤销权限 | Arc ATC | M（可作为差异化） | 无 | P1 |
| K-12 | 无痕 / 访客模式（两种语义必须明确区分） | Chrome | Y | Electron 可用 partition，UI 未接入 | P0 |

### 3.8 下载 / 历史 / 书签

| # | 能力 | 来源 | 日驱阈值 | Astra 现状 | 优先级 |
| --- | --- | --- | --- | --- | --- |
| D-1 | 下载：任务进度、暂停、取消、续传、重试、打开文件/所在文件夹 | Chrome | Y | 仅有 Electron 默认条，无统一 UI | P0 |
| D-2 | 下载气泡 + 侧边下载中心（按任务/站点分组） | Chrome | Y | 无 | P1 |
| D-3 | 危险下载告警（签名不匹配/恶意文件阻断 + ZIP 解包前扫描） | Chrome | M（合规 + 信任底线） | 无（接 Safe Browsing API 后） | P1 |
| D-4 | 压缩包内文件扫描前打开禁用 / 解封流程 | Chrome 135 | N | 无 | P2 |
| D-5 | 历史：按时间倒序 + 按站点/标题/URL 搜索 + 批量删除 | Chrome | Y | 有（domain history 模块，UI 不完整） | P1 |
| D-6 | 历史：Journeys（按主题自动聚类浏览会话）可命名 / 一键删除整个簇 | Chrome | M（≥1000 历史条目） | 无 | P2 |
| D-7 | AI 语义历史搜索（"上周读的关于蜜蜂保护的文章"） | Chrome 135 | M（卖点） | 无 | P2 |
| D-8 | 历史自动过期（30/60/90 天/自定义窗口） | Chrome+扩展 | M | 无 | P1 |
| D-9 | 书签：文件夹 / 搜索 / 拖拽 / 导入导出 HTML | Chrome | Y | 部分（侧边栏 Favorites，无全功能管理 UI） | P1 |
| D-10 | 书签导入（Chrome/Edge/Safari/Firefox HTML） | Chrome | Y | 无 | P0 |
| D-11 | 书签共享（受密码保护链接，协作者） | Chrome 135 | N | 无 | P2 |
| D-12 | 最近关闭标签跨窗口、跨 Space 的完整恢复链 | Chrome+Arc | Y | 有（单 Space，窗口缺失） | P1 |

### 3.9 侧栏 / 阅读模式 / PDF / 翻译

| # | 能力 | 来源 | 日驱阈值 | Astra 现状 | 优先级 |
| --- | --- | --- | --- | --- | --- |
| V-1 | 侧栏：书签 / 历史 / 下载 三入口固定 | Chrome Side Panel | M（有垂直侧边栏产品形态，所以 Y） | 有（侧边栏直接整合，形态不同） | P1 |
| V-2 | 扩展自定义侧栏（Manifest V3 Side Panel API） | Chrome 114+ | M（扩展生态） | 无 | P2 |
| V-3 | 阅读模式：去广告纯文章视图 / 字号字体主题可调 | Chrome+Safari | Y（内容消费型用户） | 无 | P0 |
| V-4 | 阅读模式：总结 / Q&A / 引用提取 | Chrome+Arc | M | 无（A-1 同步覆盖） | P2 |
| V-5 | 阅读模式：多栏报纸版式 / 无滚动分页 / OpenDyslexic 字体 / TTS 朗读 | Chrome 135 | N（无障碍加分项） | 无 | P2 |
| V-6 | 离线保存阅读模式页，跨设备同步 | Chrome | N（需云同步） | 无 | P2 |
| V-7 | PDF 基本查看（缩放、旋转、全屏、侧边缩略图） | Chromium | Y | Electron 默认已支持 | P0 已覆盖 |
| V-8 | PDF 表单填写 / 签名（画签 + 输入 + 证书签名）/ 文本插入 | Chrome+Edge | Y（办公 Y） | 无 | P1 |
| V-9 | PDF OCR（扫描件自动识别、读屏） | Chrome 134 | M（办公 + 无障碍底线） | 无 | P2 |
| V-10 | PDF 导出可编辑 DOCX/XLSX / 左右对比视图 | Chrome 135 | N | 无 | P2+ |
| V-11 | PDF AI 总结 / 问答 / 100 语言翻译 | Chrome+Arc Max | M | 无（A-1/A-2 同步覆盖） | P1 |
| V-12 | 内建 100+ 语言页面级翻译，保结构 | Chrome | Y（出海 Y） | 无（可接 Google Translate API 或 Chromium 内置翻译） | P0 |
| V-13 | 选词翻译 / 整段翻译气泡 | Chrome | M | 无 | P2 |

### 3.10 扩展 / DevTools / 开发者能力

| # | 能力 | 来源 | 日驱阈值 | Astra 现状 | 优先级 |
| --- | --- | --- | --- | --- | --- |
| E-1 | 兼容 Chrome Web Store（Manifest V3）扩展 | Chrome | Y（大众用户必 Y） | 无（Electron 需要专门集成层） | P0 |
| E-2 | Service Worker、Declarative Net Request、Storage、Tabs API 兼容 | MV3 | Y | 依赖 E-1 | P0 |
| E-3 | 扩展权限护栏 + 卸载入口（与设置页一致） | Chrome | M | 依赖 E-1 | P1 |
| E-4 | 原生 DevTools 入口：Elements/Console/Sources/Network/Performance/Memory/Application/Security/Lighthouse | Chrome | Y（种子用户是开发者） | Electron 原生可用，需统一 F12 / Ctrl+Shift+I + 设置入口 | P0 |
| E-5 | 任务级子面板：Recorder/CSS Overview/Issues/Rendering/Network Conditions/Performance Monitor/Web Vitals | Chrome | M（进阶开发者） | Electron 已带，需测试 | P1 |
| E-6 | Lighthouse：INP 交互归因、90 天 CrUX 趋势、交互快照 | Chrome 134 | M | Electron 已带，需测试 | P1 |
| E-7 | DevTools AI 助手（报错解释+修复片段、Lighthouse 优先级） | Chrome 135+ / Arc | M（卖点） | 无 | P2 |
| E-8 | 嵌套 CSS/CSS Anchor/BFCache 标记/Wasm 调试增强 | Chrome 最新 | N | Electron 已带 | — |
| E-9 | 图形化堆快照 diff、自定义 DevTools 主题、Popup 调试 | Chrome 实验 | N | — | P2+ |
| E-10 | Flags：chrome://astra-flags 实验功能开关 | Chrome | M（开发者种子用户） | 无 | P1 |
| E-11 | 多用户 Profiles（独立书签/密码/扩展/设置，家庭/共用设备） | Chrome | Y | 无（Space ≠ Profile；当前 Space 共享 session partition，但未覆盖"Profile"语义） | P0 |

### 3.11 媒体 / 无障碍 / 主题

| # | 能力 | 来源 | 日驱阈值 | Astra 现状 | 优先级 |
| --- | --- | --- | --- | --- | --- |
| U-1 | 全局媒体会话控件（播放/暂停/切歌/前进后退/音量/投屏） | Chrome | Y | 无 | P1 |
| U-2 | 画中画（PiP，视频悬浮任一窗口上） | Chrome+Arc | M（看网课/会议） | 无 | P1 |
| U-3 | 多路 PiP（最多 3 路） | Arc | N | 无 | P2 |
| U-4 | 标签静音 / 取消静音（图标 + 快捷键）/ 媒体设备切换 | Chrome | Y | 有（状态徽章已有；快捷键需确认） | P0 已覆盖 |
| U-5 | Cast 投屏（Chromecast/Android TV/Google TV） | Chrome | M（有 TV 设备的家） | 无 | P2 |
| U-6 | 屏幕阅读器兼容（VoiceOver/NVDA/JAWS + 原生 ChromeVox） | Chrome | Y（合规） | Electron 默认兼容，需加 ARIA 审查 | P1 |
| U-7 | 页面缩放（10%–500%，站点级记忆） | Chrome | Y | 无（设置需接入） | P0 |
| U-8 | 强制页面颜色 / 高对比度主题 | Chrome 无障碍 | M | 无 | P1 |
| U-9 | F7 光标浏览（Caret Browsing） | Chrome | M | Chromium 默认，需启用 UI | P1 |
| U-10 | 大光标 / 焦点高亮 / 悬停文本放大 | Chrome 无障碍 | N（加分项） | 无 | P2 |
| U-11 | 实时字幕（Live Caption：任意音频实时字幕） | Chrome | M（无障碍底线） | 无 | P2 |
| U-12 | 亮/暗主题随系统自动切换 | Arc/Chrome | Y | 部分（需接入系统主题） | P1 |
| U-13 | 全局 Chrome 色（主色、面板、Sidebar 独立调色） | Arc | M（差异化） | 部分（Space 色已做） | P1 |
| U-14 | 每 Space 独立配色 + 图标 | Arc | Y（Arc 特色） | 有 | P1 已覆盖 |

### 3.12 账户 / 同步 / 跨设备 / 企业

| # | 能力 | 来源 | 日驱阈值 | Astra 现状 | 优先级 |
| --- | --- | --- | --- | --- | --- |
| C-1 | 本地 Profile 多人切换（3.10 E-11 同一条能力链） | Chrome | Y | 无 | P0 |
| C-2 | 登录账号后跨设备同步：书签/密码/历史/设置/扩展/打开的标签/地址栏数据 | Chrome | M（多设备用户>10% 再上，云成本高） | 无（需自建或接第三方） | P2 |
| C-3 | 跨设备接力（手机/平板打开的标签接续到桌面） | Chrome | M | 依赖 C-2 | P2 |
| C-4 | 跨设备 Tab Stash 同步（移动端打开同 Space 自动暂停桌面后台） | Arc ATC | N | — | P2+ |
| C-5 | 新设备生物识别授权（Touch ID / Windows Hello 批准密码同步，免 Google 密码） | Chrome 133 | M（安全） | 依赖 C-2/P-8 | P2 |
| C-6 | Arc VPN / 内置 VPN | Arc | N（运营成本高） | 无 | P2+ |
| C-7 | Kids Mode / 家长控制 | Arc | N（需要内容审核团队） | 无 | P2+ |
| C-8 | 企业 Teams 管理后台（共享 Space、权限、审计日志） | Arc Teams | N | 无 | P2+ |

### 3.13 窗口 / 安装 / 打包 / 平台集成

| # | 能力 | 来源 | 日驱阈值 | Astra 现状 | 优先级 |
| --- | --- | --- | --- | --- | --- |
| W-1 | 多窗口支持（新建窗口、窗口间拖 Tab/Group） | Chrome | Y | 部分（Electron 可做，UI 未接） | P0 |
| W-2 | 会话恢复：崩溃/重启自动恢复窗口 + Tab/Group/Space 完整状态 | Chrome | Y（信任底线） | 部分（本地 localStorage，跨窗口缺失） | P0 |
| W-3 | PWA 安装（独立窗口、离线、通知、徽章、链接捕获） | Chrome | M（日常驱动 Y） | 无 | P1 |
| W-4 | 原生文件关联（.html/.pdf/.mhtml）、URL Scheme（astra://） | 所有浏览器 | M | 需打包配置 | P1 |
| W-5 | 系统级分享扩展（第三方 app 的链接可用 Astra 打开） | Arc | N（平台小优化） | 无 | P2 |
| W-6 | 默认浏览器设置 + 每次启动检查 | Chrome/Edge | M | 无 | P1 |
| W-7 | 打印：系统对话框 / 另存 PDF / 页眉页脚 / 边距 / 缩放 / 背景图形 | Chrome | Y（办公 Y） | Electron 默认，需暴露设置入口 | P1 |
| W-8 | 页面另存为（HTML 完整 / 单文件 MHTML / 仅 HTML） | Chrome | Y | 无 | P1 |
| W-9 | 菜单栏 / 任务栏全局快捷入口（Little Arc 类功能的启动键） | Arc | M | 无 | P2 |
| W-10 | 自动更新（Squirrel / MSIX / Sparkle） | 所有桌面产品 | Y | 已有 release 流程，自动更新需确认 | P0 |
| W-11 | 代码签名 + 公证（macOS notarization / Windows EV） | 所有桌面产品 | Y（安全底线，避免 SmartScreen 拦截） | 需 CI 接入 | P0 |
| W-12 | QR 码分享（页面生成二维码，手机扫码打开） | Chrome | M（跨设备便捷功能） | 无 | P2 |
| W-13 | 发送到你的设备（跨设备推送链接） | Chrome | M（依赖同步生态） | 依赖 C-2 | P2 |
| W-14 | 节能模式（电池 20% 或拔插时降帧率/后台任务） | Chrome+Arc ATC | M（笔记本用户 Y） | 无 | P1 |

### 3.14 设置页（Settings）——浏览器底线完整性

设置页不只是一个页面，它是 **上述 3.1–3.13 全部能力的统一索引入口**。对标 Chrome settings 页面结构，Astra 设置页至少需要：

| 区块 | 条目数（估算） | 优先级 |
| --- | --- | --- |
| 你与 Astra（Profile / 同步 / 登录） | 8–12 | P0 |
| 自动填充（密码 / 地址 / 付款方式） | 15–20 | P0 |
| 隐私与安全（Safe Browsing / Cookie / 权限 / 历史清理 / 浏览数据 / 安全扫描） | 20–30 | P0 |
| 外观（主题 / 色 / 字号 / 缩放 / 侧栏 / 是否显示书签栏） | 10–15 | P1 |
| 搜索引擎（默认引擎 / 站点 Tab-to-Search / 建议开关） | 6–8 | P1 |
| 默认浏览器（系统级设置） | 2–3 | P1 |
| 启动时行为（恢复会话 / 打开配置页 / 指定页） | 3–5 | P0 |
| 站点设置（K-5：所有 origin 级权限总控，约 20+ 权限 × 列表页） | 50+ 屏幕 | P0 |
| 性能（内存节省 / 节能 / 自动休眠策略 / 单 Tab 资源明细） | 10–15 | P1 |
| 无障碍（U-6～U-11） | 10–15 | P1 |
| 扩展（E-1～E-3） | 8–12 | P1 |
| 语言 + 翻译（V-12） | 8–10 | P1 |
| 下载（D-1～D-4） | 6–8 | P0 |
| 打印设置 | 5–8 | P1 |
| 系统（硬件加速 / 代理 / 后台运行 / 打开文件 / 证书管理） | 8–10 | P1 |
| 重置与清理 | 4–6 | P1 |
| 关于 Astra（版本、更新、开源、协议） | 4–6 | P1 |

估算：**16 个主区块 × 每个 6–20 屏幕，合计 180–230 个可交互设置项**，这是"能用浏览器"与"好用浏览器"之间最大的单一工程量。

---

## 4. 差距量化（当前 Astra vs. 日常驱动）

### 4.1 覆盖计数

基于 3.1–3.13 的 **约 160 条能力**：

-   **P0 必做（Y 级，缺了就是坏浏览器）：约 35 条**，当前已覆盖约 15 条（≈ 43%），还缺约 20 条（密码/地址/权限/设置页骨架/下载/Safe Browsing/多窗口/会话恢复/扩展/V-7 阅读模式/V-12 翻译/Profile/无痕/强制 HTTPS/自动更新/签名/二维码…）
-   **P1 重要（缺了不致命，但知识工作者会切回 Chrome）：约 75 条**，当前已覆盖约 20 条（≈ 27%），还缺约 55 条
-   **P2 差异化（Arc Max/Chrome 最新功能）：约 50 条**，当前已覆盖 ≈ 0，全部待做

### 4.2 能力成熟度评分（0–10，10=Chrome 稳定版）

**注：** 以下评分基于 **Electron 版本现状**。迁移到 CEF 后，密码/权限/扩展/DevTools 等 Chromium 原生能力的成熟度将大幅提升（+3～+5 分），但 Shell/工程化成熟度会暂时下降。

| 维度 | Electron 现状 | CEF 迁移后预计 | 备注 |
| --- | --- | --- | --- |
| Shell + 持久化 + Spaces | 6 | 4→7 | CEF 迁移后短期降，长期原生能力升 |
| Tab 生命周期（开关切重休分） | 7 | 7→8 | 核心逻辑可复用；CEF 原生更稳定 |
| 侧边栏 UI + 拖拽语义 | 8.5 | 8.5 | UI 层迁移可复用，Split View 实体模型已完成 |
| Split View + Glance 预览 | 6.5 | 7 | **Split View 独立实体模型已完成**（S-1）；缺三/四窗、可交互 Peek |
| 命令栏 / 地址栏 / 搜索 | 5 | 5 | 缺搜索建议、站内搜索、地址栏动作按钮 |
| 密码 / 自动填充 | 0.5 | 7→8 | CEF 可直接接入 Chromium Password Manager |
| 权限 / 安全浏览 / 隐私 | 1 | 7→8 | CEF 原生权限系统 + Safe Browsing |
| 设置页 | 0.5 | 3→5 | 需重新构建 UI，但 Chromium 原生能力可用 |
| 历史 / 下载 / 书签 UI | 3 | 6→7 | CEF 原生历史/下载服务，只需 UI 层适配 |
| 扩展（CWS）兼容 | 0 | 5→7 | CEF 需集成 Chromium extensions 模块 |
| DevTools 入口 | 4 | 8→9 | CEF 原生 DevTools Protocol，一致性远好于 Electron |
| 媒体 / 无障碍 / 主题 | 3 | 5→6 | CEF 原生媒体/无障碍更好 |
| 翻译 / PDF / 阅读模式 | 0.5 | 6→7 | Chromium 内置翻译 + PDF Viewer 原生可用 |
| 多窗口 + PWA + 会话恢复 | 2 | 5→6 | CEF 原生多窗口 + PWA 安装 |
| 打包/签名/自动更新 | 5 | 3→5 | CEF 打包更复杂，但长期可控 |
| AI 套件（可选卖点） | 0 | 0 | 后续差异化 |
| 内存占用（相对 Electron） | — | +30% 优化 | CEF 单运行时 + 灵活进程模型 |

### 4.3 "切回 Chrome 的临界点"反推优先级（最重要的 20 项）

以下 **20 项完成度达到可用级别，Astra 才能让种子用户"作为默认浏览器坚持一周以上"**：

1.  **密码保存 + 填充 + 搜索 + 编辑**（P-1）— CEF 原生可用
2.  **设置页骨架 + 站点设置总控 + 自动填充入口**（3.14 + K-5）
3.  **统一权限系统 UI（摄像头/麦克风/定位/剪贴板/通知/联系人）**（K-2）— CEF 原生可用
4.  **无痕模式 + 访客模式（两种语义独立）**（K-12）— CEF 原生可用
5.  **增强型安全浏览（默认）+ HTTPS 强制 + 错误拦截页**（K-1/K-6）— CEF 原生可用
6.  **下载中心 UI（进度/暂停/取消/打开/打开所在位置）**（D-1）— CEF 原生可用
7.  **危险下载阻断 + Safe Browsing 接入**（D-3）— CEF 原生可用
8.  **完整历史视图（搜索/按站点/批量删除/Journeys 至少 MVP）**（D-5/D-6）— CEF 原生可用
9.  **书签导入（Chrome/Edge/HTML）**（D-10）
10. **页面级翻译（Chromium 内置翻译）**（V-12）— CEF 原生可用
11. **阅读模式 MVP（去广告 + 字号字体主题）**（V-3）
12. **PDF 表单填写 MVP**（V-8）— CEF 原生可用
13. **站点级缩放（记忆）+ 页面缩放入口**（U-7）
14. **F12 / Ctrl+Shift+I 原生 DevTools 统一入口**（E-4）— CEF 原生可用
15. **Chrome Web Store 扩展兼容 MVP**（E-1/E-2）— 工程量最大
16. **全局媒体控件 + 画中画**（U-1/U-2）— CEF 原生可用
17. **多窗口 + 拖 Tab 跨窗口 + 完整会话恢复（崩溃/重启）**（W-1/W-2）
18. **PWA 安装**（W-3）— CEF 原生可用
19. **打印 UI + 另存为 PDF 入口**（W-7）— CEF 原生可用
20. **macOS Notarization + Windows EV 签名 + 自动更新**（W-10/W-11）

---

## 5. 产品路标（Roadmap）更新

> 与 `docs/ROADMAP.md` 的 P0/P1/P2 保持一致，并扩展到更细的时间分带。ROADMAP 是执行板（包含任务级细分），PRD 是时间分带（Why & Scope）。

### 5.1 架构迁移总览

**Phase 0 — CEF 脚手架**（当前分支：`chromium-native`）：建立 CEF + React UI 集成，验证技术可行性。

**Phase 1 — 核心功能移植**：将 Electron 版本的 UI / Domain 逻辑迁移到 CEF 架构。

**Phase 2 — Chromium 能力接入**：密码 / 下载 / 权限 / 历史 / 设置 / DevTools 等 Chrome 底线能力。

**Phase 3 — 差异化功能完善**：扩展 / 翻译 / 阅读模式 / PWA / 多窗口。

### 5.2 时间分带

#### Milestone 0 — CEF 脚手架（当前 → 7 月底，≈ 6 周）

**目标：** CEF 工程搭建完成，React UI 嵌入 CefBrowserHost，基础 Tab 开关可用，侧边栏 MVP 跑通。

交付：
- CEF 工程脚手架（CMake + 依赖管理）
- React UI 嵌入 CefBrowserHost（窗口模式）
- JS ↔ Native 通信机制（V8 扩展 + 消息路由器）
- 基础 Tab：新建 / 关闭 / 切换 / 导航
- 侧边栏 MVP：Spaces 切换 + Tab 列表 + 地址栏导航
- 状态管理：Native 为真相源，UI 层订阅更新
- 构建系统：macOS arm64/x64 + Windows x64
- 测试框架：CEF 集成测试 + Vitest 单元测试

#### Milestone 1 — 核心功能移植（7 月底 → 9 月中，≈ 6 周）

**目标：** Arc 风格核心交互全部移植到 CEF 架构，Electron 版本可废弃。

交付：
- 完整侧边栏：Essentials / Pinned / Favorites / Groups / Tabs / Recently Closed
- Split View 独立实体模型（S-1，已在 Electron 版验证，迁移）
- 拖拽语义：重排 / 进 Favorite / 进 Pinned / 进 Group / 跨 Space / 到 Split
- Tab 组：命名 / 配色 / 折叠 / 展开 / 整体关闭 / 整体休眠
- Glance 预览（S-4）
- 命令栏（Command Bar）：搜索 / 跳转 / 执行动作
- 开始页（astra://newtab）
- Tab 自动休眠 / 内存节省（T-6）
- 站点级缩放记忆（U-7 MVP）
- F12 / Ctrl+Shift+I DevTools（E-4，CEF 原生）
- 无痕模式（K-12，CEF 原生 off-the-record）

#### Milestone 2 — 日驱底线 Batch A（9 月中 → 11 月中，≈ 8 周）

**目标：** 把"最大缺口四项"中的 2.5 项做到可用：密码、权限、设置页、翻译/阅读模式/下载 MVP。

交付（对照 §4.3 临界点 1–20）：
- 1 P-1 密码库（Chromium Password Manager 原生接入 + UI）
- 2 设置页（自动填充/权限/外观/搜索/默认/启动/站点设置 8 个主面板可交互）
- 3 K-2 权限系统 UI 接入（摄像头/麦克风/定位/通知/剪贴板 5 种 MVP）
- 6 D-1 下载中心 MVP UI（Chromium 原生下载系统接入）
- 8 D-5 完整历史视图 MVP（Chromium 历史服务接入）
- 9 D-10 书签导入 MVP（Chrome/Edge/HTML）
- 10 V-12 页面级翻译 MVP（Chromium 内置翻译）
- 11 V-3 阅读模式 MVP
- 12 V-8 PDF 表单填写 MVP（Chromium 原生 PDF Viewer）
- 17 W-1/W-2 多窗口 + 会话恢复 MVP

#### Milestone 3 — 日驱底线 Batch B（11 月中 → 2027 春节，≈ 12 周）

**目标：** 临界点 20 项全部达到"可用"（非完美）；Astra 能稳定坚持为默认浏览器，开发者用户日常工作不切 Chrome。

交付：
- 4 K-12 无痕 + 访客模式完整
- 5 K-1/K-6 HTTPS 强制 + Safe Browsing 接入
- 7 D-3 危险下载阻断（Safe Browsing）
- 13 U-7 站点级缩放完善
- 14 E-4 DevTools 完善
- 15 E-1/E-2 Chrome Web Store 兼容 MVP（Top 20 扩展验证）
- 16 U-1/U-2 全局媒体控件 + PiP
- 18 W-3 PWA 安装 MVP
- 19 W-7 打印 UI + 另存 PDF
- 20 W-10/W-11 自动更新 + macOS Notarization + Windows EV

#### Milestone 4 — 差异化 Arc 风格功能（2027 春节之后）

**目标：** 用户选 Astra 而不是 Chrome 的"非底线理由"。

交付（从 P1/P2 池按 ROI 挑）：
- S-2/S-3 多窗 Split + 按 Group 分屏
- S-5 Peek 可交互预览
- B-1 Tab 关联笔记
- B-3 Boost 可视化隐藏元素 / 改色 MVP
- A-1 页面 / 长文 / PDF 总结
- T-18 Tab Stashes
- U-12/U-13 完整主题系统
- K-11 后台权限实时告警
- U-3 多路 PiP
- W-14 节能模式 + 资源中心
- E-10 Flags 实验开关
- T-9 多窗口 × Space 语义整合

#### Milestone 5 — 平台 / 生态

交付：
- 跨设备同步
- Passkey
- Journey 语义历史
- AI 套件
- Easel 协作白板
- Little Arc
- Profile/企业 Teams 管理
- Kids / VPN
- 移动端

---

## 6. 工程 / 架构风险 Top 10（必须显式管理）

1.  **CEF 集成复杂度**：Chromium Embedded Framework 虽然提供了完整的 Chromium 能力，但 C++ 原生开发调试成本高于 Electron Node.js。CEF 的生命周期管理（CefClient / CefBrowser / CefRenderProcessHandler）需要深入理解 Chromium 多进程架构。
2.  **UI 层与 Native 层状态同步**：React UI 状态（Zustand）与 Chromium 原生状态（CefBrowser / CefFrame）是两个状态源，如何保证一致性是核心架构挑战。采用 Native 为唯一真相源，UI 层只读订阅模式。
3.  **扩展兼容（E-1）**：CEF 默认不包含 Chrome 扩展系统，需要自己集成 Chromium extensions 子模块。工程量大但价值高，需要评估直接用 Chromium 原生 extensions 模块 vs 自建兼容层。
4.  **密码库（P-1）**：Chromium Password Manager 是 C++ 核心组件，CEF 可直接接入。但需要处理密钥链集成（macOS Keychain / Windows DPAPI）和加密方案设计。
5.  **Safe Browsing API（K-6/D-3）**：Google 的 API 有费用/配额/合规条款，需评估接入或自建风险 URL 数据库方案。CEF 可直接使用 Chromium 内置 Safe Browsing 模块。
6.  **DevTools 一致性（E-4）**：CEF 原生支持 DevTools Protocol，比 Electron webview 更接近 Chrome 原生体验。需要验证扩展 DevTools、Service Worker 调试等高级功能。
7.  **多窗口 × Space 状态同步（W-1/W-2）**：单进程模型下多窗口状态同步需要 IPC 设计。需设计 Space↔Window 映射，会影响几乎所有 domain action。
8.  **代码签名 & 公证（W-11）**：macOS notarization 和 Windows EV 签名的 CI 集成非常脆（Apple 审核/时间戳/证书过期），需要独立工程周。
9.  **无障碍（U-6）**：CEF + React 组合对屏幕阅读器基本兼容，但若不做系统性 ARIA 审计，会出现大量 tab 顺序、aria-label、role 的零散 bug。
10. **测试基础设施**：需要建立 CEF 集成测试（CEF Test Framework + Playwright/CDP），覆盖 Tab 生命周期、拖拽、跨窗口、权限、下载、翻译、DevTools、会话恢复。

---

### 6.1 Chromium CEF vs Electron 对比决策记录（ADR-0009）

| 维度 | Electron | Chromium CEF | 决策 |
| --- | --- | --- | --- |
| 开发效率 | 高（Node.js 生态） | 中（C++ 原生） | CEF — 长期收益大于短期成本 |
| 内存占用 | 高（双运行时） | 中-低（单运行时 + 灵活进程模型） | CEF — 浏览器核心竞争力 |
| 扩展兼容 | 差（需自建兼容层） | 好（可接入 Chromium extensions） | CEF — P0 级需求 |
| 密码/安全浏览 | 差（需自实现） | 好（原生组件可用） | CEF — 底线能力 |
| 打包体积 | 大（150MB+） | 中（100MB 左右） | CEF — 优化空间大 |
| DevTools | 可用但不一致 | 原生一致 | CEF — 开发者用户重要 |
| 多平台支持 | 好 | 好 | 持平 |
| 生态/文档 | 丰富 | 较少但够用 | Electron 略优 |

**决策结论：** 迁移到 Chromium CEF 架构。虽然短期开发成本上升，但浏览器核心竞争力（扩展、密码、安全浏览、内存）均为 P0 级需求，Electron 架构无法满足。

---

## 7. 验收口径（"离能用还远"的量化版本）

为了避免"我觉得差不多 / 用户还是会切回 Chrome"的主观争论，定义 **Astra v1.0 Ready（默认浏览器可用）** 的硬性指标：

-   [ ] §4.3 20 个临界点条目全部达到"可用"定义
-   [ ] 设置页 §3.14 16 个主区块中 **13 个以上有可交互 UI**
-   [ ] 3.1–3.13 中 **P0（≈35 条）100% 通过；P1（≈75 条）70% 通过**
-   [ ] 种子用户 **≥ 30 人，"连续 7 天使用 Astra 作为默认浏览器不主动切 Chrome"比例 ≥ 60%**（NPS 抽样）
-   [ ] Electron 集成 Playwright 测试 ≥ 150 条，覆盖：Tab 生命周期、拖拽、跨窗口、权限、下载、翻译、DevTools、会话恢复
-   [ ] 每日构建（CI）macOS arm64/x64 + Windows x64/arm64 + Linux AppImage/deb/rpm 全部生成且 Notarization/EV 通过
-   [ ] 无障碍（U-6/U-7/U-9）走查通过
-   [ ] 密码、权限、扩展三大设置面板具备安全审计级别的日志和撤销路径

**以当前 2026-06-10 基线估算：**
-   v1.0 Ready 最早窗口：**2026 年 10 月底**（3 人全时工程，无重大架构返工）
-   较保守窗口：**2027 年春节**（允许扩展兼容返工一次 + 密码方案改动一次）

---

## 8. 与 ROADMAP.md 的映射

本 PRD 只做优先级和范围校准，**执行板继续用 `docs/ROADMAP.md`**：
-   §5 Milestone 0 = ROADMAP Batch 1（P0 Stabilization）+ 少量 P1 设置骨架/缩放/DevTools
-   §5 Milestone 1 = 新增 ROADMAP Batch 5（Day-driver Gaps A）
-   §5 Milestone 2 = 新增 ROADMAP Batch 6（Day-driver Gaps B + Extensions + Auto-update）
-   §5 Milestone 3 ≈ ROADMAP Batch 2/3/4 的更详细拆细
-   §5 Milestone 4 = ROADMAP Batch P2 Expansion（同步/AI/Teams/移动端）

**下一步动作（立即执行）：**
1.  把本 PRD 合入仓库根 `docs/PRD.md`；
2.  ROADMAP.md 新增 §"Milestones 0-4 与 PRD 5.x 映射表"一段（≤ 50 行，不重复详细内容）；
3.  Milestone 1 前 2 周做 E-1 扩展兼容、P-1 密码库、K-6 Safe Browsing 三项 PoC，各写一份 2 页 ADR（架构决策记录）存档到 `docs/adr/`；
4.  Milestone 0 增加一条：建立 Electron 集成测试脚手架（Playwright）。
