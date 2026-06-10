# ADR 0008 — 代码签名 & macOS Notarization & Windows EV 签名 CI 流水线

| 字段 | 值 |
| --- | --- |
| 状态 | Proposed（启动 W-11 开发前确定） |
| 负责人 | TBD |
| 决策日期 | 2026-08-15（M2 启动前） |
| 关联 PRD 条目 | W-10, W-11, §6 风险 #8 |
| 关联临界点 | §4.3 第 20 项 |

## 背景

PRD §4.3 临界点第 20 项要求"macOS Notarization + Windows EV 签名 + 自动更新"，这是桌面软件发布的安全底线。§6 风险 #8 明确指出"macOS notarization 和 Windows EV 签名的 CI 集成非常脆弱"。

当前状态：
- `electron-builder` 配置已支持多平台打包；
- CI 已有 release workflow，但未启用代码签名；
- 未申请 Apple Developer ID、Windows EV 证书；
- `electron-updater` 已集成但未测试签名后的更新流程。

本 ADR 明确签名流水线的架构、密钥管理、回滚策略。

## 候选方案

**方案 A（推荐）：GitHub Actions + 云端密钥存储 + 分平台签名 Job**

- 密钥存储：
  - macOS：Apple Developer ID Application 证书 + Notarization API Key（App Store Connect API），存储于 GitHub Actions Secrets；
  - Windows：EV Code Signing Certificate（USB HSM 或 DigiCert 云端密钥），存储于 GitHub Actions Secrets 或 DigiCert KeyLocker；
- CI 流水线：
  - `build` job：全平台构建 + 未签名包验证；
  - `sign-mac` job：macOS 签名 + notarization（使用 `electron-builder` 的 `afterSign` hook + `@electron/notarize`）；
  - `sign-win` job：Windows 签名（使用 `electron-builder` 的 `sign` hook + DigiCert 云端签名或 Azure Key Vault）；
  - `sign-linux` job：Linux GPG 签名（deb/rpm/AppImage）；
  - `release` job：收集所有签名包 → 创建 GitHub Release → 触发 `electron-updater` 的 `latest.yml` 更新。
- 密钥轮换：每 11 个月轮换一次（Apple 证书有效期 5 年，Windows EV 证书有效期 1-3 年），在 CI 中配置过期前 30 天告警。

成本：证书费用 ~$300-700/年，CI 配置 + 调试约 2 周。

**方案 B：本地构建机 + 物理 HSM + 手动签名**

- 密钥存储：
  - macOS：证书导入本地构建机的 Keychain，锁定 Keychain 访问权限；
  - Windows：EV 证书 USB HSM 物理连接到构建机；
- 签名流程：
  - CI 构建未签名包 → 上传到内部存储 → 本地构建机 pull → 签名 → upload 回 release；
- 优点：密钥永不离开物理设备，安全性最高；
- 缺点：CI 非闭环，需要手动触发签名，发布流程慢。

成本：证书费用 ~$300-700/年 + 构建机硬件 ~$2000，CI 配置约 3 周。

**方案 C：使用第三方签名服务（Azure Code Signing / DigiCert One）**

- 密钥完全由第三方服务管理；
- CI 通过 API 调用签名服务，无需在 CI 中存储任何私钥；
- 优点：安全性最高，密钥永不暴露；
- 缺点：费用较高（~$1000-2000/年），API 调用可能增加签名时间。

## 评估维度

| 维度 | A（GitHub Actions + Secrets） | B（本地构建机 + HSM） | C（第三方签名服务） |
| --- | --- | --- | --- |
| 安全性（密钥暴露风险） | 中（GitHub Secrets 有访问控制） | 高（物理隔离） | 最高（第三方托管） |
| CI 自动化程度 | 高（全自动） | 低（需手动触发） | 高（全自动） |
| 签署时间/包 | 2-5 分钟（macOS notarization 最慢） | 3-8 分钟 | 5-15 分钟（API 延迟） |
| 年度成本 | $300-700（证书） | $300-700（证书） + $2000（硬件） | $1300-2700（服务+证书） |
| 工程投入 | 2 周 | 3 周 | 1.5 周 |
| 可维护性 | 高（GitHub 生态成熟） | 低（需维护物理设备） | 中（依赖第三方 API） |
| 合规性（HSM 要求） | 需单独申请 FIPS HSM | 满足（USB HSM） | 满足（服务端 HSM） |

## 暂定决策

- 走 **方案 A**（GitHub Actions + Secrets），但 Windows EV 签名采用云端 HSM 模式（DigiCert KeyLocker 或 Azure Key Vault）而非导出私钥：
  1. macOS：Developer ID 证书 + App Store Connect API Key 存储于 GitHub Secrets，使用 `@electron/notarize` 工具；
  2. Windows：EV 证书使用 DigiCert KeyLocker 云端密钥，CI 通过 DigiCert ONE API 调用签名，不导出私钥；
  3. Linux：GPG 密钥存储于 GitHub Secrets，签名 deb/rpm/AppImage；
  4. 密钥访问控制：仅 release 分支的 tag push 事件可访问签名密钥；
  5. 审计：所有签名操作记录到内部日志，保留 1 年。
- 理由：
  1. 方案 A 在安全性、自动化、成本之间取得最佳平衡；
  2. Windows EV 采用云端 HSM 避免了私钥导出的安全风险；
  3. GitHub Actions Secrets 有 branch protection + environment protection 两层保护；
  4. Electron 生态对方案 A 有成熟的模板和最佳实践。

## PoC 通过标准

- [ ] CI 打 tag `v0.2.0-test` 后，自动生成 macOS .dmg + .zip、Windows .exe、Linux .AppImage + .deb；
- [ ] macOS 包可通过 `spctl -a -t exec -vv Astra.app` 验证（notarization 通过）；
- [ ] Windows 包可通过 `signtool verify /pa /v AstraSetup.exe` 验证（EV 签名有效）；
- [ ] `electron-updater` 可从 v0.1.0 自动更新到 v0.2.0-test，更新过程中校验签名；
- [ ] 非 release 分支的 PR 无法访问签名密钥（CI 验证失败）。

## 不可决事项 / 升级触发

- GitHub 发生 Secrets 泄漏事件 → 立即迁移到方案 B 或 C；
- Apple 变更 Notarization 流程（如强制使用 notarytool v2）→ 2 周内适配；
- Windows EV 证书的云端 HSM 方案不可用 → 降级为方案 B（本地 USB HSM）；
- 公司有 ISO 27001 合规要求 → 迁移到方案 C（第三方签名服务）以满足 HSM 审计要求；
- 签名时间 > 10 分钟/包 且影响发布节奏 → 评估并行签名或缓存策略。
