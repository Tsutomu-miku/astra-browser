# ADR 0007 — Electron webview → BrowserView 迁移必要性评估

| 字段 | 值 |
| --- | --- |
| 状态 | Proposed（M1 末性能测试后确定） |
| 负责人 | TBD |
| 决策日期 | 2026-08-15（M1 末） |
| 关联 PRD 条目 | T-6, T-7, W-14, §6 风险 #1 |
| 关联临界点 | 隐含（内存/性能是所有功能的基础） |

## 背景

PRD §6 风险 #1 指出"Electron Webview 的进程模型比原生 Chrome 更吃内存，Sleep/Freeze 策略必须比 Chrome 激进 2×"。当前 Astra 使用 `<webview>` 标签渲染所有 Tab 内容，这是 Electron 提供的"旧版"web 内容渲染方案；Electron 同时提供 `BrowserView`（主进程创建、附加到 BrowserWindow）作为替代方案。

两种方案的核心差异：
- `<webview>`：基于 `out-of-process iframes`，渲染进程与主渲染进程共享部分资源，生命周期由 React 管理；
- `BrowserView`：独立的 `BrowserWindow` 子集，完全独立的渲染进程，生命周期由主进程管理，附加到窗口的特定坐标区域。

M1 末需要进行系统性性能测试，根据测试结果决定是否在 M2 启动迁移。

## 候选方案

**方案 A：维持 `<webview>`，强化内存管理策略**

- 不做架构迁移；
- 将 Memory Saver 阈值从 5 分钟降低到 2 分钟；
- 非活动 Tab 数量 > 10 时自动休眠最早的 Tab；
- Split View 中非焦点 pane 在 1 分钟后进入"冻结"状态（保留进程但停止 JavaScript 计时器）；
- 引入 `--disable-features=IsolateOrigins,site-per-process` 等 Chromium 开关减少进程数量。

成本：0 迁移成本，仅需调整 domain 层的 sleep 策略（约 1 周）。

**方案 B：全量迁移到 `BrowserView`，主进程统一管理 Web 内容生命周期**

- 所有 Tab/Split/Glance 的 Web 内容从 `<webview>` 改为 `BrowserView`；
- 主进程新增 `webContentManager.ts` 统一管理所有 `BrowserView` 的创建、附加、布局、销毁；
- 渲染进程仅通过 IPC 向主进程请求"将 viewId=123 附加到 x=200,y=0,w=800,h=600"；
- Split View 的多 pane 通过多个 `BrowserView` 叠加实现；
- 解决 `<webview>` 的内存泄漏、进程复用、DevTools 一致性等问题。

成本：PoC 2 周 + 全量迁移 8-12 周，影响所有 Tab/Split/Glance 相关代码（≈ 30% 的渲染器代码）。

**方案 C：混合模式，仅特定场景使用 `BrowserView`**

- 普通 Tab 继续使用 `<webview>`；
- 以下场景使用 `BrowserView`：
  - Split View 的非焦点 pane（减少与主渲染进程的资源竞争）；
  - PWA 窗口（需要独立的窗口生命周期）；
  - 无痕/Guest 窗口（需要更强的进程隔离）；
- 通过性能监控数据决定后续是否扩大 `BrowserView` 使用范围。

成本：PoC 1 周 + 集成 3-4 周，中等侵入性。

## 评估维度（M1 末性能测试后打分）

| 维度 | 测试方法 | A（维持 webview）阈值 | B（迁移 BrowserView）阈值 |
| --- | --- | --- | --- |
| 20 个 Tab 内存占用 | Activity Monitor / 任务管理器 | < 1.5× Chrome 同期内存 | > 1.8× Chrome 同期内存（触发迁移） |
| 10 分钟闲置后内存释放 | 打开 20 个 Tab → 闲置 10 分钟 → 对比内存 | 释放 > 40% | 释放 < 25%（触发迁移） |
| Tab 切换延迟 | 从 Tab 1 切到 Tab 20，测量 webview 挂载时间 | < 100ms | > 200ms（触发迁移） |
| Split View 4 窗流畅度 | 同时播放 4 个 YouTube 视频，测量 FPS | > 55 FPS | < 45 FPS（触发迁移） |
| DevTools 一致性 | 对比 Chrome DevTools 10 项常用功能 | 0 项差异 | ≥ 3 项功能异常（触发迁移） |
| 内存泄漏率 | 连续打开/关闭 100 个 Tab，比较初始和最终内存 | 增长 < 10% | 增长 > 30%（触发迁移） |

## 暂定决策（基于当前信息的预判断）

- **默认走方案 A**（维持 `<webview>` + 强化 sleep 策略），理由：
  1. 迁移成本极高（8-12 周 = 半个 M2 的工作量）；
  2. 当前 `<webview>` 在 10-15 Tab 场景下内存表现可接受（~1.2× Chrome）；
  3. Electron 42 已修复多个 `<webview>` 内存泄漏 bug；
  4. 强化 sleep 策略可将内存占用再降低 30-40%。
- **M1 末必须完成上表 6 项性能测试**，如果 3 项以上超过 B 的阈值 → 重新评估方案 C 或 B；
- 无论是否迁移，M1 末必须产出性能测试报告存档于 `docs/performance/`。

## PoC 通过标准（若触发迁移）

- [ ] 新建 Tab 使用 `BrowserView` 而非 `<webview>`，功能与现有 Tab 一致；
- [ ] Split View 多 pane 使用多个 `BrowserView` 叠加，拖拽分隔线时视图同步更新；
- [ ] Tab 切换延迟 < 80ms（优于当前 `<webview>`）；
- [ ] 20 个 Tab 内存占用比当前方案降低 > 25%；
- [ ] DevTools 打开的元素检查、网络面板、断点调试与 Chrome 100% 一致。

## 不可决事项 / 升级触发

- Electron 官方宣布 `<webview>` 弃用 → 立即启动方案 B；
- 某个关键扩展（uBlock、Tampermonkey）在 `<webview>` 下无法正常工作 → 评估方案 C；
- M2 用户调研中"卡顿"反馈 > 15% → 重新运行性能测试并评估；
- 出现可复现的 `<webview>` 内存泄漏 bug（泄漏率 > 5MB/小时）→ 紧急评估方案 C。
