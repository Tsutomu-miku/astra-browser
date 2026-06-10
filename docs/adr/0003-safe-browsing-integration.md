# ADR 0003 — Safe Browsing 接入策略

| 字段 | 值 |
| --- | --- |
| 状态 | Proposed (PoC @ M1 前 2 周) |
| 负责人 | zhangmian.02 |
| 决策日期 | 2026-06-27 |
| PoC 分支 | `feature/poc-safe-browsing` |
| 关联 PRD 条目 | K-1, K-6, D-3 |
| 关联临界点 | §4.3 第 5/7 项 |

## 背景

Chrome 的 Enhanced Safe Browsing（ESB）包含三层：URL 实时查询、二进制哈希预下载比对、扩展 CWS 声誉校验。PRD §4.3 临界点 5（强制 HTTPS + ESB 默认开启）、7（危险下载阻断）都是 Y 级，不做即被归类为"不安全浏览器"。Electron 自身不带 SB 层，必须在应用层实现。

## 候选方案

**方案 A（推荐）：Google Safe Browsing v5 Update API（本地哈希数据库 + 实时命中回源）**

- `google/safebrowsing` Go 参考实现打包成 Node N-API addon，或直接 HTTP 调 `sb-ssl.google.com/safebrowsing/uploads/*`；
- 本地保留 4MB 哈希前缀数据库，URL 命中前缀 → 发 full hash 到 Google（带 7 位前缀，保护隐私）；
- 下载文件 → SHA-256 查 ThreatType = `MALWARE` / `UNWANTED_SOFTWARE` / `SOCIAL_ENGINEERING`；
- HTTPS 强制走 Chromium 内置 `HTTPS-Upgrades` + 回退到 `HSTS` 列表 + 自补 `upgrade-insecure-requests: 1` CSP。

成本：PoC 1 周 + 集成 0.5 周。

**方案 B：第三方替代源（URLHaus / PhishTank / VirusTotal）**

- URLHaus、PhishTank 更新频率与覆盖面逊于 Google SB；
- VirusTotal 有商用配额门槛。
- 结论：**作为方案 A 失败时的 fallback，不做默认。**

**方案 C：Chromium 内置 SB 组件编译接入**

- 跟 Electron 升级强耦合，每季度重打 patch；
- 方案 A 已能覆盖 90% 的 daily-driver 场景。
- 结论：**P2 级，M3 之后再评估。**

## 评估维度

| 维度 | A（Google SB v5） | B（VT + URLHaus） | C（Chromium SB） |
| --- | --- | --- | --- |
| 覆盖率（Phish/Malware） | 95%+（业界基线） | 60–75% | 98% |
| 延迟（URL 命中） | 15–30ms 前缀 + 150ms 回源 | 300–800ms | 10–50ms |
| 合规条款 | 需 Google API Key + 品牌指引 | VT 商用 > $10k/年 | 无需（Blink 内置） |
| 工程人月（PoC + 集成） | 0.8 + 0.5 | 0.5 + 0.3 | 4+ |
| 对用户隐私 | 只发 32-bit 前缀 | 发 URL 明文 | 本地哈希 |

## 暂定决策

- 走 **方案 A**，三层开关（可由设置页独立控制）：
  1. 增强保护（ESB，默认开启）— 查 URL + 下载哈希 + 扩展哈希；
  2. 标准保护（默认 fallback，离线 DB + 不发统计）；
  3. 关闭（强警示 + 每次启动确认）。
- K-1 强制 HTTPS：
  1. 对所有顶级导航默认 `upgrade-insecure-requests`；
  2. Chromium 内置 HSTS preload 列表维持默认；
  3. `https-upgrade-fallback` 命中不安全时渲染自定义错误页（astra://interstitial/insecure），不允许直接点"继续"。

## 合规与商业条款

- Google SB API Key → 走 Astra 公共 GCP 项目；
- 用户数据：**绝不能上报 URL 明文**，仅发 32-bit SHA256 前缀；
- 下载统计：在设置页提供"关闭上传威胁命中"的开关（即降级到标准保护）；
- 品牌：在 `astra://settings/privacy` 底部标注"数据通过 Google Safe Browsing API 处理，查看 [Google 隐私条款]"。

## PoC 通过标准

- [ ] 访问 `testsafebrowsing.appspot.com/s/phishing.html` 命中并展示 interstitial；
- [ ] 下载 `https://testsafebrowsing.appspot.com/dl/malware.exe` 被阻断且显示 `DANGEROUS`；
- [ ] 关闭网络 2 小时内，离线数据库仍能阻止 Top 10 测试样本；
- [ ] Wireshark 抓包：只发送 `safebrowsing.google.com` 的 32-bit 前缀，不发送 URL 明文。

## 升级触发

- Google SB v6 发布或 v5 EOL → 2 周内完成升级；
- 月均查询量超出免费配额（预计 10k QPS 档）→ 申请商业配额或引入方案 B 混合。
