/**
 * ADR-0008 / W-10: electron-updater 自动更新服务。
 *   - provider = "github"（W-11 release job 会在 GitHub Release 上传 latest-mac.yml / latest.yml）
 *   - 仅在打包后环境 (app.isPackaged === true) 启用；dev 环境跳过，避免污染测试
 *   - 用户检查入口：设置 About 面板 "Check for updates" + 菜单 "Astra → Check for Updates…"
 *   - 更新流程：check → 发现新版本 → 自动后台下载 → 下载完成后通知 UI "Install and restart"
 *   - 更新状态广播到所有窗口（sendToAll），供 Topbar / About 面板展示
 */

const { app } = require("electron");
const { autoUpdater } = require("electron-updater");

const STATE = {
  /** null | 'checking' | 'available' | 'not-available' | 'downloading' | 'ready' | 'error' */
  status: "idle",
  currentVersion: app.getVersion(),
  info: null, // { version, releaseName, releaseNotes }
  error: null, // string message
  progress: null // { percent, bytesPerSecond, transferred, total }
};

let sendToAllFn = null;

function broadcast() {
  if (!sendToAllFn) return;
  sendToAllFn("auto-update:state", {
    ...STATE,
    // electron-updater 在 dev 环境不启用，UI 需要知道
    isEnabled: Boolean(app.isPackaged)
  });
}

function setStatus(next, payload = {}) {
  STATE.status = next;
  Object.assign(STATE, payload);
  if (payload.status === undefined) delete STATE.status; // avoid overwrite
  STATE.status = next;
  broadcast();
}

function initUpdater({ sendToAll, allowPrerelease = false }) {
  sendToAllFn = sendToAll;

  if (!app.isPackaged) {
    console.log("[auto-update] Dev environment — updater disabled.");
    setStatus("idle");
    return;
  }

  autoUpdater.autoDownload = false;      // 用户确认后再下载（但默认 true 更顺，我们用 false 可控）
  autoUpdater.autoInstallOnAppQuit = true;
  autoUpdater.allowPrerelease = allowPrerelease;
  autoUpdater.allowDowngrade = false;
  autoUpdater.fullChangelog = false;

  autoUpdater.on("checking-for-update", () => {
    console.log("[auto-update] Checking for update...");
    setStatus("checking");
  });

  autoUpdater.on("update-available", (info) => {
    console.log(`[auto-update] Update available: ${info.version}`);
    setStatus("available", { info: pickInfo(info) });
    // 立即自动开始后台下载（autoDownload = false 但我们手动 start）
    void autoUpdater.downloadUpdate();
  });

  autoUpdater.on("update-not-available", (info) => {
    console.log(`[auto-update] Already on latest: ${info.version}`);
    setStatus("not-available", { info: pickInfo(info) });
  });

  autoUpdater.on("download-progress", (progress) => {
    setStatus("downloading", {
      progress: {
        percent: Number(progress.percent?.toFixed?.(1) ?? progress.percent),
        bytesPerSecond: progress.bytesPerSecond || 0,
        transferred: progress.transferred || 0,
        total: progress.total || 0
      }
    });
  });

  autoUpdater.on("update-downloaded", (info) => {
    console.log(`[auto-update] ${info.version} downloaded — ready to install.`);
    setStatus("ready", { info: pickInfo(info), progress: null });
  });

  autoUpdater.on("error", (err) => {
    const msg = err && (err.message || String(err));
    console.error("[auto-update] Error:", msg);
    setStatus("error", { error: msg || "Unknown error" });
  });

  // 启动 10s 后静默首次检查（给窗口渲染留出时间）；之后每 2 小时重试。
  setImmediate(broadcast);
  setTimeout(() => void checkForUpdates(true), 10_000);
  setInterval(() => void checkForUpdates(true), 2 * 60 * 60 * 1000);
}

function pickInfo(info) {
  if (!info) return null;
  return {
    version: info.version,
    releaseName: info.releaseName || null,
    releaseNotes:
      typeof info.releaseNotes === "string"
        ? info.releaseNotes
        : Array.isArray(info.releaseNotes)
        ? info.releaseNotes.map((n) => n.note || n).join("\n")
        : null,
    releaseDate: info.releaseDate || null,
    files: Array.isArray(info.files) ? info.files.map((f) => ({ url: f.url, size: f.size })) : []
  };
}

/**
 * 检查更新。silent = true 时不弹 "not-available" 的 busy 状态（用于后台周期检查）。
 */
async function checkForUpdates(silent = false) {
  if (!app.isPackaged) {
    setStatus("idle");
    return { ok: false, reason: "disabled-in-dev" };
  }
  try {
    if (!silent) setStatus("checking");
    const res = await autoUpdater.checkForUpdates();
    return { ok: true, hasUpdate: Boolean(res && res.updateInfo && res.updateInfo.version !== app.getVersion()) };
  } catch (err) {
    const msg = err && (err.message || String(err));
    setStatus("error", { error: msg || "checkForUpdates failed" });
    return { ok: false, error: msg };
  }
}

async function downloadUpdate() {
  if (!app.isPackaged) return { ok: false, reason: "disabled-in-dev" };
  try {
    await autoUpdater.downloadUpdate();
    return { ok: true };
  } catch (err) {
    const msg = err && (err.message || String(err));
    setStatus("error", { error: msg || "downloadUpdate failed" });
    return { ok: false, error: msg };
  }
}

/** 下载完成后，立即退出并安装更新。 */
function installAndRestart() {
  if (!app.isPackaged) return false;
  if (STATE.status !== "ready") return false;
  try {
    autoUpdater.quitAndInstall(true, true);
    return true;
  } catch {
    return false;
  }
}

function getState() {
  return {
    ...STATE,
    isEnabled: Boolean(app.isPackaged)
  };
}

module.exports = {
  initUpdater,
  checkForUpdates,
  downloadUpdate,
  installAndRestart,
  getState
};
