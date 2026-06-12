# ADR-0008 签名 & Notarization 密钥配置指南

本文件说明 Release CI 需要在仓库 **Settings → Secrets and variables → Actions** 中配置的
全部密钥。未配置密钥时，release workflow 仍然会产出**未签名包**用于 QA；配置后自动切换到
签名 + notarization 流程。

## 必须配置的 Secrets

### macOS（签名 + Notarization）

| Secret 名 | 说明 | 获取方式 |
| --- | --- | --- |
| `CSC_LINK` | macOS Developer ID Application 证书（`.p12` Base64） | `base64 -i DevID_App.p12 \| tr -d '\n'` |
| `CSC_KEY_PASSWORD` | `.p12` 导出密码 | 导出时自行设置 |
| `APPLE_ID` | 用于 Notarization 的 Apple ID（邮箱） | Apple Developer Account 主邮箱 |
| `APPLE_APP_SPECIFIC_PASSWORD` | Apple ID App 专用密码（非登录密码） | https://appleid.apple.com → Security → App-Specific Passwords |
| `APPLE_TEAM_ID` | Apple Team ID（10 字符） | https://developer.apple.com/account/ → Membership |

### Windows（EV 签名 — DigiCert KeyLocker 推荐）

| Secret 名 | 说明 |
| --- | --- |
| `DIGICERT_KEYLOCKER_API_KEY` | DigiCert ONE KeyLocker API Key |
| `DIGICERT_CLIENT_CERT_ONELINER` | DigiCert ONE 客户端证书（PEM 格式，所有换行 `\n` 替换为 `\|`） |
| `DIGICERT_SIGNER_CERT_SM2` | DigiCert 签名证书（SM2 可选，若用 RSA 留空） |
| `DIGICERT_KEYPAIR_ALIAS` | KeyLocker 密钥对别名，默认 `EV_CodeSigning` |
| `DIGICERT_TIMESTAMP_SERVER` | 时间戳服务器，默认 `http://timestamp.digicert.com` |

**或回退 OV 签名（非 EV）：**

| Secret 名 | 说明 |
| --- | --- |
| `WINDOWS_CERT_BASE64` | `.pfx` 证书 Base64 |
| `WINDOWS_CERT_PASSWORD` | `.pfx` 密码 |

### Linux（GPG 签名 — 可选）

| Secret 名 | 说明 |
| --- | --- |
| `GPG_PRIVATE_KEY_ONELINER` | `gpg --armor --export-secret-keys <keyid>` 全部换行替换为 `\|` |
| `GPG_PASSPHRASE` | 密钥口令 |

## 发布流程

1. 所有签名/Notarization 操作只在 `v*` tag push 触发的 release job 里运行；
2. PR 分支的 release workflow（若手动触发）会被显式跳过，不访问任何密钥；
3. 密钥被访问前，CI 环境必须通过 `environment: release` 保护；
4. 所有签名操作审计日志写入 `scripts/sign-windows.js` 与 `scripts/notarize.js`，CI 控制台可见；
5. 非 release 分支 **禁止读取** 以上 Secrets，由 GitHub Branch protection + Environment protection rule 双重保证。

## 过期告警

- Developer ID 证书有效期 5 年、Windows EV 证书 1–3 年，GitHub 仓库 `dependabot.yml` 可扩展（本项目在 CI release job 末尾加了证书过期前 30 天的 shell 告警检查，见 `release.yml` `verify-signatures` step）。

## 降级 / 升级触发（ADR §"不可决事项"）

- GitHub Secrets 泄漏 → 迁移到本地构建机 HSM（方案 B）；
- Apple 切换到 notarytool v3 → `scripts/notarize.js` 中 `tool` 参数升级；
- DigiCert KeyLocker 不可用 → 回退 OV pfx 模式（需 `WINDOWS_CERT_BASE64`）。
