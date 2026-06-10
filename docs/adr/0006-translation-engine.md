# ADR 0006 — 翻译模块选型（Google Translate API vs Marian/Argos 本地）

| 字段 | 值 |
| --- | --- |
| 状态 | Proposed（启动 V-12 开发前确定） |
| 负责人 | TBD |
| 决策日期 | 2026-06-27（M1 第 2 周末） |
| 关联 PRD 条目 | V-12, A-2, §6 风险 #5 |
| 关联临界点 | §4.3 第 10 项 |

## 背景

PRD §4.3 临界点第 10 项要求"页面级翻译（V-12）"，这是出海产品的 Y 级需求。§6 风险 #5 明确指出 Chromium 内置翻译组件受 API Key 限制，需要在以下路径中选择：

1. 调用 Google Cloud Translation API（云端，付费）；
2. 嵌入 Marian NMT / Argos Translate 本地模型（离线，免费，体积 ~150MB/语言对）；
3. 接入 Chromium 内置翻译组件的非公开 API（风险高，随版本变动）。

此外 A-2（100+ 语言翻译，保留文档结构）与 V-12 共享同一翻译引擎选型，选型结果影响 AI 套件的整体成本模型。

## 候选方案

**方案 A（推荐，M1 默认方向）：Google Cloud Translation API v3（云端）+ 本地缓存**

- API 调用：`https://translation.googleapis.com/v3/projects/{project}/locations/global:translateText`；
- 内容抓取：通过 `webContents.executeJavaScript` 获取页面主体文本（排除导航/广告），保留 DOM 结构映射；
- 回填：逐段翻译后通过 DOM 替换回填，保留原页面样式；
- 本地缓存：按 `(source_lang, target_lang, url_hash, paragraph_hash)` 缓存 30 天，避免重复翻译；
- 成本估算：每百万字符 ~$20，按日均 10 页 × 每篇 5000 字符 × 1000 活跃用户 ≈ $300/月。

**方案 B：Argos Translate（本地 OpenNMT 模型）+ WASM 或 Node.js 绑定**

- 模型下载：首次使用某语言对时下载 ~150MB 模型文件，存储于用户数据目录；
- 推理：`argos-translate` Node.js 绑定或 WASM 版本在渲染进程本地推理；
- 语言覆盖：支持 50+ 语言对，质量略低于 Google（尤其中文↔小语种）；
- 成本：0 云费用，但需维护模型更新管道（每 3 个月更新一次模型）。

**方案 C：Chromium 内置 `translate_agent` 组件 FFI 调用**

- 通过 `electron-patch` 修改 Chromium 源码，暴露 `TranslateAgent` 的 JS 绑定；
- 复用 Chrome 的翻译质量、DOM 结构保留、语言自动检测；
- 成本：工程复杂度极高，每 Electron 大版本升级需重打 patch。

**方案 D：混合模式（默认 Google API，可切换到本地模型）**

- 默认走方案 A，设置页提供"离线翻译"开关；
- 开启后下载 Argos 模型并切换到本地推理；
- 成本：需同时维护两套 pipeline，模型下载/管理逻辑额外 1 周工作量。

## 评估维度

| 维度 | A（Google API + 缓存） | B（Argos 本地） | C（Chromium FFI） | D（混合模式） |
| --- | --- | --- | --- | --- |
| 翻译质量（中英/中日） | 95%+（业界基线） | 75-85% | 95%+ | 95%+（默认） / 75-85%（离线） |
| 首次翻译延迟 | 300-800ms | 2-5s（WASM） / 800-2000ms（Node） | 100-300ms | 300-800ms（默认） |
| 离线可用性 | 否 | 是 | 是 | 是（需切换） |
| 云成本（1k DAU） | ~$300/月 | $0 | $0 | ~$300/月（默认） |
| 安装包体积增加 | 0 | +150MB/语言对（按需下载） | 0 | 0（默认） / +150MB（离线） |
| 工程人月（PoC + 集成） | 0.5 + 1.0 | 1.0 + 2.0 | 3.0+ | 1.0 + 2.5 |
| DOM 结构保留能力 | 需自实现（中等难度） | 需自实现（同等难度） | 原生支持 | 需自实现（两套） |
| 隐私合规（GDPR） | 需数据处理协议（DPA） | 本地处理，合规友好 | 本地处理 | DPA + 本地可选 |

## 暂定决策

- M1 MVP 走 **方案 A**（Google Cloud Translation API v3），理由：
  1. 翻译质量是用户不可妥协的底线（翻译不准 = 功能不可用）；
  2. 工程投入最小（1.5 人月 vs 3+ 人月），可在 M1 交付可用版本；
  3. 1k DAU 下 $300/月成本可控，用户规模上来后再评估方案 D；
  4. 本地缓存可将重复翻译成本降低 70%+。
- 设置页提供"不发送页面内容到 Google"开关，开启后翻译功能降级为不可用；
- M2 末根据用户规模和成本数据重新评估方案 D。

## PoC 通过标准

- [ ] 打开日文 Wikipedia 页面，点击翻译按钮 1 秒内显示中文翻译；
- [ ] 翻译后页面保留原有的图片、表格、超链接结构；
- [ ] 同一页面第二次翻译延迟 < 50ms（命中缓存）；
- [ ] Wireshark 抓包：仅发送文本内容到 `translation.googleapis.com`，不发送 cookie/localStorage；
- [ ] 关闭网络后访问已缓存的翻译页面仍能显示翻译结果。

## 不可决事项 / 升级触发

- 月翻译成本 > $5000 → 立即启动方案 D 混合模式开发；
- Google API 价格上调 > 50% → 重新评估方案 B；
- Argos 中文模型质量评测 BLEU 分数 > 40（当前 ~28）→ 重新评估方案 B；
- 欧盟用户占比 > 30% 且 GDPR 投诉 > 1 起 → 加速方案 D 落地。
