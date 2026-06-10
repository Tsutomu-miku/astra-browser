# ADR 0001 — Chrome Web Store (Manifest V3) 扩展兼容层

| 字段 | 值 |
| --- | --- |
| 状态 | Proposed (PoC @ M1 前 2 周) |
| 负责人 | TBD |
| 决策日期 | 2026-06-27（M1 第 2 周末） |
| 关联 PRD 条目 | E-1, E-2, E-3, §6 风险 #2 |
| 关联临界点 | §4.3 第 15 项 |

## 背景

Persona A（日常驱动用户）平均在 Chrome 上安装 3–8 个扩展；根据 PRD §3.10 对照，**不支持 uBlock Origin、Tampermonkey、Dark Reader、1Password、Bitwarden、React DevTools 这 Top 6 扩展，就会直接触发切回 Chrome**。Electron 官方未提供 Chrome Web Store / MV3 的原生兼容层；Chromium 内置的扩展子系统在 Electron 中被移除或只暴露给 `chrome-extension://` protocol 的基础 hook。

## 候选方案

**方案 A（推荐，PoC 默认方向）：Electron Extensions（electron-extensions 社区维护版 + 手写 MV3 Service Worker 宿主）**

- 复用 Electron 已废弃的 `electron-extensions`（@pengx17/logilib 分支或 electron-browser-shell 移植版）；
- 为 MV3 Service Worker 自建一个短命周期的 BrowserWindow / offscreen window 宿主，封装 `chrome.storage`、`chrome.tabs`、`chrome.declarativeNetRequest`、`chrome.runtime` 四个核心 namespace；
- Declarative Net Request 走 Electron `session.webRequest` 的 onBeforeRequest 重写；
- Side Panel API（V-2）后期走 Astra 侧栏扩展点。

成本：4–6 周 PoC，之后每兼容一个新 namespace 约 0.5–2 周。

**方案 B：Electron 18+ 的 `ses.loadExtension` 官方接口**

- 官方仅保证 MV2；MV3 仅支持最小 manifest（不支持 Service Worker、不支持 DNR）。
- 结论：**只够加载最原始的 MV2 扩展**，不满足本需求。

**方案 C：CEF / Chromium Embedded Framework 替换 Electron 的扩展子系统**

- 工作量等于重写 main/preload/webview 三层。
- 结论：**Milestone 3 之后才值得评估**。

**方案 D：向用户明确"扩展不保证兼容"**

- 把 E-1 降为 P2，在设置页挂"欢迎提交适配 PR"说明。
- 会直接流失 60%+ Persona A 用户（PRD §4.3 临界点 #15 就是扩展兼容）。

## 评估维度

| 维度 | A（社区层 + 手写宿主） | B（loadExtension） | C（CEF） | D（放弃） |
| --- | --- | --- | --- | --- |
| uBlock 兼容 | 80% 可行 | 0% | 100% 可行 | N/A |
| Tampermonkey 兼容 | 60%（userscript 层） | 0% | 100% | N/A |
| 工程人月 | 2.5 + 1.5/季度 | <0.5（但不可用） | 8+ | 0 |
| 对现有代码侵入 | Medium（新增 `src/main/extensions/`） | Low | Very High | 0 |
| 后续维护成本 | Medium（跟 Chromium 大版本） | Low | Very High | 0 |
| 用户流失率（估算） | 5–10% | 40–60% | 0% | 60%+ |

## 暂定决策（待 PoC 验证）

- **走方案 A**，范围锁定 4 个核心 namespace + Top 20 扩展验证清单；
- **M1 第 2 周末前必须跑通 uBlock Origin 与 Dark Reader 的基础功能**（页面拦截、样式注入）；
- 若 PoC 失败 → 切换到方案 A 的 D（降级，E-1 标 P2，临界点 20 项改 19 项并在 PRD 同步声明）。

## 不可决事项 / 升级触发

- Google 对 CWS 的 `update.xml` 下发做反爬封 IP → 升级为 "自建镜像 + 用户登录态"；
- Electron 的 `webRequest` hook 性能在 1000 条 DNR 规则下退化至每请求 > 5ms → 必须切 Chromium native network service bridge；
- 1Password / Bitwarden 依赖未公开的 native messaging hook → 明确公告"不支持密码类扩展，使用内建 P-1 密码库"。

## 可交付物

1. PoC 仓库分支（本仓库 `feature/poc-mv3-extensions`）；
2. `src/main/extensions/mv3-host.ts` 最小宿主；
3. Top 20 扩展冒烟脚本（Playwright，`tests-e2e/extensions/*.spec.ts`）；
4. 本 ADR 的 Status 字段在 M1 第 2 周末改为 Accepted / Rejected。
