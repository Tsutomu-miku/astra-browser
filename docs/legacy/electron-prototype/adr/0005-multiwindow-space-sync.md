# ADR 0005 — 多窗口 × Space 状态同步模型（windowId ↔ spaceId）

| 字段 | 值 |
| --- | --- |
| 状态 | Accepted（方案 A，2026-06-11 定案） |
| 负责人 | Seed M2.5 delivery |
| 决策日期 | 2026-06-11（M2.5 交付期） |
| 关联 PRD 条目 | W-1, W-2, T-9, §6 风险 #7 |
| 关联临界点 | §4.3 第 17 项 |

## 背景

PRD §4.2 把"多窗口 + 会话恢复"成熟度打 2 分、§6 风险 #7 列为 Top 10 第 7。当前 Astra 的 Space 状态为单窗口单实例模型（`browserStore` 中 Space 与 activeTabId 直接关联，无 Window ID 维度），导致：

1. 新建窗口后，同一 Space 在两个窗口中的 Tab 选择、休眠、分组状态会互相覆盖；
2. 会话恢复只能恢复最后一个窗口的状态，多窗口场景下数据丢失；
3. 跨窗口拖拽 Tab 后，原窗口的 Space 状态与新窗口不一致。

Electron 层面 `BrowserWindow` 已有独立 `id`，但 domain 层未建立 `windowId ↔ spaceId ↔ activeTabId` 的三元映射关系。

## 候选方案

**方案 A（推荐）：BrowserStore 引入 Window 维度，Space 状态按 Window 隔离**

- `browserStore` 新增 `windows: Record<WindowId, WindowState>`，`WindowState` 含 `activeSpaceId`、`spaceStates: Record<SpaceId, SpaceWindowState>`；
- `SpaceWindowState` 含该 Window 下该 Space 的 `activeTabId`、`paneFocus`、`scrollPosition` 等窗口本地状态；
- 全局 Tab/Group/Favorite 数据仍为单例（同一份对象在多个窗口中可见），仅 UI 状态（active、focus、scroll）按 Window 隔离；
- IPC 层所有 action 新增可选 `windowId` 参数，默认取 `sender` 的窗口 ID。

成本：PoC 1 周 + 集成 2 周，影响约 30 个 domain action。

**方案 B：每个 BrowserWindow 独立一份 browserStore 实例，通过主进程 IPC 同步全局数据**

- 每个渲染进程维护独立 store 实例；
- Tab/Group/Favorite 等全局数据由主进程 centralized 存储，通过 `browserStore:sync` IPC 广播变更；
- 窗口本地状态（active、focus）不广播。

成本：PoC 2 周 + 集成 4 周，需重写现有 store 订阅机制。

**方案 C：限制为"单窗口模式"，仅支持从当前窗口拖拽 Tab 出新窗口**

- 把 W-1 降为 P1，仅实现"把 Tab 拖出到新窗口"的最小功能；
- 新窗口作为独立"单 Space 窗口"存在，不支持跨窗口 Space 切换。

成本：PoC 3 天 + 集成 1 周，但用户体验显著劣于 Chrome。

## 评估维度

| 维度 | A（Store 引入 Window 维度） | B（多 Store 实例 + 主进程同步） | C（限制单窗口模式） |
| --- | --- | --- | --- |
| 对现有代码侵入 | Medium（仅 action 签名 + store 结构） | High（store 架构 + IPC 层全量修改） | Low |
| 数据一致性保证 | 高（全局数据单例，仅 UI 状态隔离） | 中（依赖 IPC 广播时序） | 高（天然隔离） |
| 支持 T-9（多窗口同 Space） | 是 | 是 | 否 |
| 会话恢复完整度 | 高（可恢复多窗口布局） | 高 | 低（仅恢复单窗口） |
| 工程人月（PoC + 集成） | 0.8 + 2.0 | 1.5 + 4.0 | 0.2 + 1.0 |
| 对性能影响 | 可忽略（store 内存增加 < 5%） | Medium（IPC 广播频率） | 可忽略 |

## 暂定决策

- 默认走 **方案 A**，M1 第 2 周前完成 PoC 验证：
  1. `browserStore` 新增 `windows` 维度（带类型定义）；
  2. 2 个核心 action（`selectTab`、`createTab`）接入 `windowId` 路由；
  3. 新建窗口时正确初始化该窗口的 Space 状态；
  4. 会话恢复时按 Window ID 恢复布局。
- 若 PoC 中发现 30+ 个 domain action 的修改量超出预期 → 评估方案 B 的子集（仅 Tab/Group 全局数据同步，其余状态隔离）。

## PoC 通过标准

- [ ] 同时打开 2 个窗口，在窗口 A 中选择 Tab X，窗口 B 的 activeTab 不改变；
- [ ] 在窗口 A 中将 Tab Y 休眠，窗口 B 中该 Tab 也显示为休眠状态（全局数据同步）；
- [ ] 崩溃后重启，2 个窗口的布局、Space、activeTab 完全恢复；
- [ ] 跨窗口拖拽 Tab 后，原窗口和新窗口的 Space 状态正确更新。

## 不可决事项 / 升级触发

- 后续支持"多窗口同步浏览"（一个窗口操作另一个窗口联动）→ 需扩展方案 A 的 `SpaceWindowState` 加入 sync 标记；
- 用户打开 > 10 个窗口时 store 内存占用 > 50MB → 评估方案 B 的按需加载；
- Electron 未来引入多渲染进程共享状态 API → 重新评估方案 B。
