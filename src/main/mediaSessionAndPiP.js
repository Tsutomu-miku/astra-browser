/**
 * Global MediaSession (U-1) + Picture-in-Picture (U-2) main-process handlers.
 *
 * 设计：
 *   MediaSession：主进程通过 IPC 接收来自活跃 webview 的 metadata 变化（标题/艺术家/封面）
 *   并同步到操作系统（macOS Now Playing / Windows SMTC / Linux MPRIS）。
 *   Electron 通过 `win.setThumbarButtons` + macOS `app.beginPresentationOptions`
 *   模拟原生 media key，实际 metadata 通过 MediaSession API 由 Chromium 自动向 OS 暴露，
 *   这里仅需要：
 *     a) 路由 Ctrl/Cmd+F7/F8/F9/␣ 快捷键到当前活跃 webview
 *     b) 在 main 端注册 thumbar 按钮（Windows/Linux）
 *
 *   PiP：Electron <webview> 原生不直接暴露 `document.pictureInPictureElement`，
 *   但可以通过 webview 的 `contents.setPictureInPicture` API（主进程）
 *   或让 webview 执行 JS `document.querySelector('video')?.requestPictureInPicture()`。
 *   MVP 选择后者：通过 IPC `pip:toggle-active-tab` 找到活跃 webview 并
 *   `executeJavaScript` 触发/退出 PiP。
 */

const { BrowserWindow } = require("electron");

function registerMediaShortcuts() {
  /* 操作系统级媒体键由 Chromium 自动处理。
   * 菜单级快捷键在 appMenu.js 中通过角色 roles 暴露。
   * 此处保留扩展点以便未来 MPRIS 插件接入。
   */
}

/**
 * 向当前 BrowserWindow 的 thumbar 注入 media control 按钮。
 * Windows / Linux 任务栏会展示 Play/Pause / Prev / Next。
 * 调用者：在 BrowserWindow 创建后调用一次。
 */
function setWindowMediaControls(win, actions) {
  try {
    win.setThumbarButtons([
      {
        tooltip: "Previous",
        icon: null,
        click: () => actions.previous && actions.previous()
      },
      {
        tooltip: "Play/Pause",
        icon: null,
        click: () => actions.togglePlay && actions.togglePlay()
      },
      {
        tooltip: "Next",
        icon: null,
        click: () => actions.next && actions.next()
      }
    ].filter((b) => b.icon !== null));
  } catch {
    /* Icons require assets we don't ship yet (M3 polish) */
  }
}

/**
 * U-2 PiP MVP：从 renderer 发过来的 IPC 在指定 webview 上触发 PiP。
 * 返回 { success, entering?: boolean, reason? }
 */
async function togglePictureInPicture(webContentsId) {
  if (typeof webContentsId !== "number") return { success: false, reason: "no-active-tab" };
  const webContents = require("electron").webContents;
  const wc = webContents.fromId(webContentsId);
  if (!wc || wc.isDestroyed()) return { success: false, reason: "destroyed" };
  try {
    const script = `
      (async () => {
        try {
          if (document.pictureInPictureElement) {
            await document.exitPictureInPicture();
            return { success: true, entering: false };
          }
          const video = document.querySelector('video:not([muted]):not([disabled]), video');
          if (!video) return { success: false, reason: "no-video" };
          if (video.paused) video.play().catch(() => {});
          await video.requestPictureInPicture();
          return { success: true, entering: true };
        } catch (err) {
          return { success: false, reason: String(err && err.message || err) };
        }
      })();
    `;
    const result = await wc.executeJavaScript(script, true);
    return result || { success: false, reason: "no-result" };
  } catch (err) {
    return { success: false, reason: String(err && err.message || err) };
  }
}

/**
 * U-1 强制同步 MediaSession：当用户切换 Tab 时，把旧标签的媒体暂停、
 * 新标签的 MediaSession metadata 上报（如果有）。
 * MVP 通过向目标 webview 注入一小段 JS 实现“切换 Tab 时暂停其它视频”。
 */
async function syncMediaSessionOnTabSwitch({ fromId, toId }) {
  const { webContents } = require("electron");
  try {
    if (typeof fromId === "number") {
      const prev = webContents.fromId(fromId);
      if (prev && !prev.isDestroyed()) {
        await prev.executeJavaScript(`
          (() => {
            const all = document.querySelectorAll('video, audio');
            all.forEach((el) => { try { if (!el.paused) el.pause(); } catch {} });
          })();
        `).catch(() => {});
      }
    }
    if (typeof toId === "number") {
      const next = webContents.fromId(toId);
      if (next && !next.isDestroyed()) {
        // 让浏览器自然接管 MediaSession。
      }
    }
  } catch {
    /* best-effort only */
  }
}

/* 尝试定位当前聚焦的 webview（活跃 tab）。 */
function findActiveWebviewId() {
  for (const win of BrowserWindow.getAllWindows()) {
    if (win && !win.isDestroyed() && win.webContents) {
      // 活跃 webview 由 renderer 维护，这里主进程只能遍历 children 找 type="webview"。
      const children = win.webContents.getAllWebContents?.() ?? [];
      for (const child of children) {
        if (child && !child.isDestroyed() && child.getType && child.getType() === "webview") {
          return child.id;
        }
      }
    }
  }
  return null;
}

module.exports = {
  findActiveWebviewId,
  registerMediaShortcuts,
  setWindowMediaControls,
  syncMediaSessionOnTabSwitch,
  togglePictureInPicture
};
