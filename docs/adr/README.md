# Architecture Decision Records (ADRs)

> 对齐 `docs/PRD.md` §6 Top 10 风险 + §8 立即执行 #3。格式来自 [MADR](https://adr.github.io/madr/) 的简版。
>
> **规则：**
> 1. 文件名遵循 `NNNN-short-kebab.md`，NNNN 单调递增不回填；
> 2. 每个 ADR 必须有 "暂定决策（Proposal → Accepted/Rejected/Deprecated）" 两阶段：Proposal 必须在 M1 第 2 周末前改为终态；
> 3. 关联的 PoC 分支、冒烟脚本、ADR 链接一起放 PRD.md §8 对应条目。

## 目录

| 编号 | 标题 | 状态 | 对应 PRD 条目 |
| --- | --- | --- | --- |
| 0001 | Chrome Web Store (MV3) 扩展兼容层 | Proposed | E-1/E-2/E-3, §6 #2 |
| 0002 | 密码库 & 自动填充架构 | Proposed | P-1/P-2/P-4/P-5/P-8, §6 #3 |
| 0003 | Safe Browsing 接入策略 | Proposed | K-1/K-6/D-3, §6 #4 |

## 规划中（M1 末之前应出）

| 规划编号 | 主题 | 触发条件 |
| --- | --- | --- |
| 0004 | 多窗口 × Space 状态同步模型（windowId ↔ spaceId） | 启动 W-1 开发前 |
| 0005 | 翻译模块选型（Google Translate API vs Marian/Argos 本地） | 启动 V-12 前 |
| 0006 | Electron webview → BrowserView 迁移必要性评估 | M1 末性能测试 |
| 0007 | 代码签名 & macOS Notarization CI 流水线 | 启动 W-11 前 |
