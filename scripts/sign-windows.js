/**
 * ADR-0008 / W-11: electron-builder Windows sign 钩子。
 *
 * 优先级策略（均走 DigiCert KeyLocker / Azure Code Signing 云端 HSM）：
 *   1. 若 DIGICERT_KEYLOCKER_API_KEY + CLIENT_CERT_FILE_ONELINER + SIGNER_CERT_SM2 都设置，
 *      使用 DigiCert ONE KeyLocker + Sign Manager 客户端（smctl）——EV 级云端签名；
 *   2. 若 WINDOWS_CERT_BASE64 + WINDOWS_CERT_PASSWORD 设置，回退到普通 OV 证书 pfx 导入；
 *   3. 否则返回 null，electron-builder 走默认逻辑（未签名），PR 分支静默通过。
 *
 * 函数签名：(configuration: any) => string[] | null，electron-builder 自定义 sign hook。
 */

/* eslint-disable no-console */

const fs = require("node:fs");
const os = require("node:os");
const path = require("node:path");
const crypto = require("node:crypto");
const childProcess = require("node:child_process");

function writeTempFile(suffix, content) {
  const name = `astra-code-sign-${crypto.randomBytes(8).toString("hex")}${suffix}`;
  const p = path.join(os.tmpdir(), name);
  fs.writeFileSync(p, content, { mode: 0o600 });
  return p;
}

function cleanup(paths) {
  for (const p of paths) {
    try { fs.unlinkSync(p); } catch { /* ignore */ }
  }
}

module.exports = async function signWindows(configuration) {
  const { DIGICERT_KEYLOCKER_API_KEY, DIGICERT_CLIENT_CERT_ONELINER, DIGICERT_SIGNER_CERT_SM2, WINDOWS_CERT_BASE64, WINDOWS_CERT_PASSWORD, GITHUB_EVENT_NAME, CSC_FOR_PULL_REQUEST } = process.env;

  if (GITHUB_EVENT_NAME === "pull_request" || CSC_FOR_PULL_REQUEST) {
    console.log("[sign-win] Skipping Windows signing for pull request branch.");
    return null;
  }

  const files = configuration.path;
  const targets = Array.isArray(files) ? files : [files];
  const tempFiles = [];

  try {
    if (DIGICERT_KEYLOCKER_API_KEY && DIGICERT_CLIENT_CERT_ONELINER && DIGICERT_SIGNER_CERT_SM2) {
      const clientCertPath = writeTempFile(".crt.pem", DIGICERT_CLIENT_CERT_ONELINER.replace(/\|/g, "\n"));
      const signerCertPath = writeTempFile(".sm2.pem", DIGICERT_SIGNER_CERT_SM2.replace(/\|/g, "\n"));
      tempFiles.push(clientCertPath, signerCertPath);

      const smctl = process.env.SMCTL_PATH || "smctl";
      for (const target of targets) {
        console.log(`[sign-win] DigiCert KeyLocker EV signing: ${target}`);
        childProcess.execFileSync(smctl, [
          "sign",
          "--keypair-alias", process.env.DIGICERT_KEYPAIR_ALIAS || "EV_CodeSigning",
          "--client-cert", clientCertPath,
          "--signer-cert", signerCertPath,
          "--input", target,
          "--output", target,
          "--digest", process.env.DIGICERT_DIGEST || "sha256",
          "--timestamp-server", process.env.DIGICERT_TIMESTAMP_SERVER || "http://timestamp.digicert.com",
          "--overwrite",
          "--no-progress"
        ], { env: { ...process.env, SM_API_KEY: DIGICERT_KEYLOCKER_API_KEY }, stdio: "inherit" });
      }
      return targets;
    }

    if (WINDOWS_CERT_BASE64 && WINDOWS_CERT_PASSWORD) {
      const pfxPath = writeTempFile(".pfx", Buffer.from(WINDOWS_CERT_BASE64, "base64"));
      tempFiles.push(pfxPath);
      const signtool = process.env.SIGNTOOL_PATH || "signtool";
      for (const target of targets) {
        console.log(`[sign-win] OV pfx signing: ${target}`);
        childProcess.execFileSync(signtool, [
          "sign",
          "/fd", "SHA256",
          "/f", pfxPath,
          "/p", WINDOWS_CERT_PASSWORD,
          "/tr", "http://timestamp.digicert.com",
          "/td", "SHA256",
          "/v",
          target
        ], { stdio: "inherit" });
      }
      return targets;
    }

    console.log("[sign-win] Skipping — no Windows signing secrets configured.");
    return null;
  } finally {
    cleanup(tempFiles);
  }
};
