// LEGACY SCRIPT — Electron era
//
// ADR-0008 / W-11: electron-builder afterSign 钩子 — macOS notarization。
//
// Part of the legacy Electron build pipeline.
// The direct-Chromium build will use its own notarization workflow
// (e.g. `codesign` + `notarytool` invoked from build scripts).
//
// 只在以下条件都满足时执行：
//   1. 目标平台是 mac
//   2. 环境变量 APPLE_ID / APPLE_APP_SPECIFIC_PASSWORD / APPLE_TEAM_ID 都已设置
//   3. CSC_FOR_PULL_REQUEST 未被设置（防止 PR 分支触发）
//
// 本地构建 & 未配置密钥的 CI 分支会静默跳过，不报错。

/* eslint-disable no-console */

module.exports = async function notarizeMac(context) {
  const { electronPlatformName, appOutDir, packager } = context;
  if (electronPlatformName !== "darwin") return;

  const appBundleId = packager.appInfo.id;
  const appName = packager.appInfo.productFilename;
  const appPath = `${appOutDir}/${appName}.app`;

  const {
    APPLE_ID,
    APPLE_APP_SPECIFIC_PASSWORD,
    APPLE_TEAM_ID,
    APPLE_ID_PASSWORD, // 备用
    GITHUB_EVENT_NAME,
    CSC_FOR_PULL_REQUEST
  } = process.env;

  // Pull request 分支始终跳过（密钥不可访问）；event === "pull_request" 或显式标记
  if (GITHUB_EVENT_NAME === "pull_request" || CSC_FOR_PULL_REQUEST) {
    console.log("[notarize] Skipping macOS notarization for pull request branch.");
    return;
  }

  const appleId = APPLE_ID;
  const appleIdPassword = APPLE_APP_SPECIFIC_PASSWORD || APPLE_ID_PASSWORD;
  const teamId = APPLE_TEAM_ID;

  if (!appleId || !appleIdPassword || !teamId) {
    console.log(
      "[notarize] Skipping — missing one or more of APPLE_ID / APPLE_APP_SPECIFIC_PASSWORD / APPLE_TEAM_ID secrets."
    );
    return;
  }

  // 延迟 require：devDependencies 只有在 CI release job 安装后才存在，
  // 本地非 mac 构建直接 require 会失败。
  const { notarize } = require("@electron/notarize");

  console.log(`[notarize] Starting Apple notarization for ${appBundleId} at ${appPath}...`);
  await notarize({
    appPath,
    appleId,
    appleIdPassword,
    teamId,
    tool: "notarytool" // Apple 推荐；xcrun notarytool，兼容 Xcode 16+
  });
  console.log(`[notarize] Done for ${appBundleId}.`);
};
