# ADR 0002 — 密码库 & 自动填充架构

| 字段 | 值 |
| --- | --- |
| 状态 | Proposed (PoC @ M1 前 2 周) |
| 负责人 | TBD |
| 决策日期 | 2026-06-27 |
| 关联 PRD 条目 | P-1, P-2, P-4, P-5, P-8 |
| 关联临界点 | §4.3 第 1 项 |

## 背景

PRD §4.2 把"密码/自动填充"成熟度打 0 分、§6 风险 #3 列为 Top 10 第 3。Electron 未暴露 Chromium Password Manager C++ 组件的任何 JS API，且不实现 `<input type=password">` 下的 Autofill Agent，**这意味着表单级的浏览器原生填充完全不存在**，必须自绘。

## 候选方案

**方案 A（推荐，PoC 默认方向）：OS Keyring 存储 + 自绘 Autofill Popup + 内容脚本**

- 存储：`electron-safe-storage` + `keytar`（macOS Keychain / Windows Credential Manager / Linux libsecret），密码原文不入本地 JSON；
- 采集：`<webview>` 注入 `preload/autofill-agent.ts` 内容脚本，监听表单 submit 事件 → 提示"是否保存"；
- 填充：自绘 Popup（BrowserWindow overlay 或 `<div>` overlay，视 DPI 而定），响应用户点击 → 调用 `webContents.executeJavaScript` 回填表单字段；
- 搜索/查看/编辑：设置页自动填充面板（P-1/§3.14 区块 #2）。
- 生物识别解锁：`node-mac-auth`（macOS Touch ID）/ `windows-credentials` 二次确认前展示。

成本：PoC 1.5–2 周；M1 交付 3–4 周；健康仪表盘/导入再迭代 2 周。

**方案 B：嵌入 1Password/Bitwarden CLI 做代理**

- 复用成熟密码库；
- 需要用户额外安装桌面应用 + 授权本机 CLI，安装门槛显著高于 Chrome。
- 结论：**作为 P-5 导入源，不作默认实现。**

**方案 C：Chromium Password Manager 组件 FFI 调用**

- 原生 `password_store` 组件 + V8 绑定；
- 复杂度等价 CEF，Electron 每次升级都要重打 patch。
- 结论：**远期仅在用户规模 > 10 万时评估。**

## 评估维度

| 维度 | A（Keyring + 自绘） | B（1Pwd/Bitwarden） | C（Chromium FFI） |
| --- | --- | --- | --- |
| 零安装用户可用 | 是（系统级） | 否（需额外装） | 是 |
| 跨站点字段识别率 | 70–85%（规则+启发） | 90%+（商业训练） | 90%+ |
| 本地安全性 | 高（系统 Keyring） | 高 | 高 |
| 工程人月（M1+M2） | 4–5 | < 0.5（仅限导入） | 10+ |
| 是否支持生物识别 | 是 | 是 | 是 |
| 与 P-3 支付意图、P-2 地址填充复用 | 高（同 popup + 内容脚本） | 低 | 中 |

## 暂定决策

- 走 **方案 A**，MVP 交付链路：
  1. `electron-safe-storage` + `keytar` 封装 → `src/main/vault.ts`；
  2. IPC 通道 `vault:save`、`vault:list-match`、`vault:fill`、`vault:update`；
  3. webview preload 注入 content-script 采集 + 回填；
  4. 设置页骨架（§3.14 #2）挂入口。
- 填充 Popup 放 webview 外部（BrowserView 透明 overlay），避免被站点 DOM z-index / iframe 隔离。

## PoC 通过标准

- [ ] 登录 GitHub 时，submit 后弹出"是否保存密码"且成功写入 Keyring；
- [ ] 再次进入 GitHub 登录页，出现自绘 popup（非原生），选择条目正确回填 `#login_field` / `#password`；
- [ ] 清空 App 后重新启动，密码能从 Keyring 恢复；
- [ ] `keychain list-internet-passwords`（macOS）或 `cmdkey /list`（Windows）看不到 Astra 写入的条目明文。

## 不可决事项 / 升级触发

- Electron safeStorage 在 Linux 某些 DE （Xfce/i3 无 keyring 守护）不可用 → 降级为 `argon2id(user-password)` 解密的本地文件；
- 自绘 popup 在跨显示器 HiDPI 错位率 > 5% → 切换到 Chromium 的 `AutofillAgent` bridge（方案 C 的轻量子集）；
- 采集/回填误识别率在 Alexa Top 500 上 < 60% → 引入 Google Autofill Predictions API 做辅助字段分类。
