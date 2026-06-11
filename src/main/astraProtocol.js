/**
 * astra:// protocol handler (E-10 astra://flags MVP, plus astra://newtab).
 *
 * Electron 主进程注册 `astra://` 为 privileged scheme（standard, secure,
 * cors-enabled, support-fetch-api）。然后通过 `protocol.handle` 返回：
 *   - astra://newtab   → 渲染进程 index.html（真实 SPA）
 *   - astra://flags   → 实验开关页（由 astra-flags.html 静态 + JS 读写 localStorage）
 *   - 其它 astra://    → 404
 *
 * flags 的实现方式：渲染进程内一个 React mini-app，通过 IPC `get-flags / set-flags`
 * 读写 main 端的 `astra-flags.json`（或本地 localStorage 作为 MVP fallback）。
 */

const { BrowserWindow, app, protocol, net } = require("electron");
const fs = require("node:fs");
const path = require("node:path");

const DEV_SERVER_URL = process.env.VITE_DEV_SERVER_URL;

/* ===== E-10 astra://flags 默认值 =====
 * - hardware-acceleration: 是否禁用 GPU 进程（重启生效）
 * - service-worker:       是否允许站点注册 Service Worker（默认 true；禁用用于排障）
 * - mv3-extensions:       是否启用 MV3 扩展兼容层（默认 false；M2 尾期开启）
 * - webgl-intervention:   对老旧设备自动降档 WebGL（默认 true）
 * - side-panel-api:       启用 chrome.sidePanel 扩展 API（默认 false）
 */
const DEFAULT_FLAGS = {
  "hardware-acceleration": true,
  "service-worker": true,
  "mv3-extensions": false,
  "webgl-intervention": true,
  "side-panel-api": false
};

function flagsFilePath() {
  return path.join(app.getPath("userData"), "astra-flags.json");
}

function loadFlags() {
  try {
    const raw = fs.readFileSync(flagsFilePath(), "utf-8");
    const parsed = JSON.parse(raw);
    return { ...DEFAULT_FLAGS, ...(parsed && typeof parsed === "object" ? parsed : {}) };
  } catch {
    return { ...DEFAULT_FLAGS };
  }
}

function saveFlags(next) {
  const merged = { ...DEFAULT_FLAGS, ...(next && typeof next === "object" ? next : {}) };
  try {
    fs.mkdirSync(path.dirname(flagsFilePath()), { recursive: true });
    fs.writeFileSync(flagsFilePath(), JSON.stringify(merged, null, 2));
  } catch {
    /* non-fatal */
  }
  return merged;
}

function renderFlagsHtml(flags) {
  const body = Object.entries(DEFAULT_FLAGS).map(([key, def]) => {
    const value = Boolean(flags[key] ?? def);
    const hint = FLAGS_HINT[key] ?? "";
    return `
<div class="flag-row">
  <label>
    <input type="checkbox" data-key="${escapeAttr(key)}" ${value ? "checked" : ""} />
    <strong>${escapeText(key)}</strong>
    <small>${escapeText(hint)}</small>
  </label>
  <div class="flag-default">Default: <code>${def ? "Enabled" : "Disabled"}</code></div>
</div>`;
  }).join("");

  return `<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <title>astra://flags — Experimental Features</title>
  <meta name="viewport" content="width=device-width,initial-scale=1" />
  <style>
    :root { color-scheme: dark; --fg: #dfe6ef; --muted: #8aa0bf; --bg: #111318; --card: #181b23; --accent: #7dd3fc; }
    * { box-sizing: border-box; }
    body { margin: 0; background: var(--bg); color: var(--fg); font: 14px/1.5 -apple-system, system-ui, sans-serif; }
    header { padding: 32px 40px 8px; border-bottom: 1px solid #222733; }
    h1 { margin: 0 0 8px; font-size: 24px; }
    header .muted { color: var(--muted); }
    main { max-width: 920px; padding: 24px 40px 80px; }
    .banner { padding: 12px 16px; background: #241a0e; color: #ffd591; border: 1px solid #3c2a14; border-radius: 8px; margin-bottom: 24px; }
    .flag-row { display: grid; grid-template-columns: 1fr auto; gap: 8px 16px; padding: 16px 0; border-bottom: 1px solid #222733; }
    .flag-row label { display: flex; flex-direction: column; gap: 4px; }
    .flag-row input[type=checkbox] { margin-right: 8px; }
    .flag-row small { color: var(--muted); }
    .flag-default { align-self: start; color: var(--muted); font-size: 12px; }
    footer { position: fixed; bottom: 0; left: 0; right: 0; padding: 12px 40px; background: #0b0d12; border-top: 1px solid #222733; display: flex; gap: 12px; justify-content: flex-end; }
    button { padding: 6px 12px; border-radius: 6px; border: 1px solid #2a2f3d; background: #1b1e27; color: var(--fg); cursor: pointer; }
    button.primary { background: var(--accent); color: #0b1420; border-color: var(--accent); }
    button:disabled { opacity: 0.5; cursor: not-allowed; }
    code { font-family: ui-monospace, monospace; }
  </style>
</head>
<body>
  <header>
    <h1>astra://flags</h1>
    <div class="muted">Experimental features (E-10). Toggle with care — changes require restart.</div>
  </header>
  <main>
    <div class="banner">These features are experimental and may cause browser instability. To reset all to defaults click "Reset all".</div>
    ${body}
  </main>
  <footer>
    <button id="reset" type="button">Reset all to defaults</button>
    <button id="apply" class="primary" type="button">Apply and restart</button>
  </footer>
  <script>
    const initial = ${JSON.stringify(flags)};
    const mutable = { ...initial };
    const setDirty = () => {
      document.getElementById("apply").disabled =
        JSON.stringify(mutable) === JSON.stringify(initial);
    };
    document.querySelectorAll('input[type="checkbox"][data-key]').forEach((input) => {
      input.addEventListener("change", () => { mutable[input.dataset.key] = input.checked; setDirty(); });
    });
    document.getElementById("reset").addEventListener("click", () => {
      Object.keys(mutable).forEach((key) => delete mutable[key]);
      document.querySelectorAll('input[type="checkbox"][data-key]').forEach((input) => {
        const def = JSON.parse(\`${JSON.stringify(DEFAULT_FLAGS)}\`)[input.dataset.key];
        input.checked = Boolean(def);
        mutable[input.dataset.key] = Boolean(def);
      });
      setDirty();
    });
    document.getElementById("apply").addEventListener("click", async () => {
      try {
        // 在 Electron 主进程上下文暴露的 astraShell.flags API（M2.5 后续补 IPC）。
        // MVP 先写入 localStorage 并提示用户重启；正式版本会通过 IPC 持久化并 app.relaunch()。
        localStorage.setItem("astra-flags", JSON.stringify(mutable));
        if (window.astraShell && typeof window.astraShell.applyFlagsAndRestart === "function") {
          await window.astraShell.applyFlagsAndRestart(mutable);
        } else {
          alert("Flags saved. Please restart the browser to apply.");
        }
      } catch {
        alert("Failed to save flags.");
      }
    });
    setDirty();
  </script>
</body>
</html>`;
}

const FLAGS_HINT = {
  "hardware-acceleration": "Disable to use CPU rendering only (useful for GPU bug workarounds). Restart required.",
  "service-worker": "Disable to prevent sites from installing service workers (security / debugging).",
  "mv3-extensions": "Enable the Chrome MV3 extension compatibility layer (currently experimental).",
  "webgl-intervention": "Automatically disable WebGL on low-end devices to avoid GPU crashes.",
  "side-panel-api": "Enable the chrome.sidePanel Extension API (E-3, requires MV3 compat layer)."
};

function escapeAttr(s) { return String(s).replace(/"/g, "&quot;"); }
function escapeText(s) {
  return String(s)
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;");
}

function installAstraProtocol() {
  const scheme = "astra";

  // registerSchemesAsPrivileged must be called BEFORE app.whenReady.
  // isProtocolHandled internally reads the default session, which isn't
  // available until app.ready — wrap it with a readiness guard.
  protocol.registerSchemesAsPrivileged([
    {
      scheme,
      privileges: {
        standard: true,
        secure: true,
        supportFetchAPI: true,
        corsEnabled: true,
        bypassCSP: false,
        stream: true
      }
    }
  ]);

  // Must be registered before app ready; the handle() call runs when ready.
  app.on("ready", () => {
    const flags = loadFlags();

    protocol.handle(scheme, async (request) => {
      try {
        const url = new URL(request.url);
        if (url.hostname === "app") {
          // 渲染进程的 SPA：由 BrowserWindow.loadURL / loadFile 走 astra://app 也可以。
          const pathname = url.pathname === "/" ? "/index.html" : url.pathname;
          if (DEV_SERVER_URL) {
            return net.fetch(`${DEV_SERVER_URL}${pathname}${url.search}`);
          }
          return net.fetch(`file://${path.join(__dirname, "../../dist/renderer")}${pathname}`);
        }
        if (url.hostname === "newtab") {
          if (DEV_SERVER_URL) return net.fetch(DEV_SERVER_URL);
          return net.fetch(`file://${path.join(__dirname, "../../dist/renderer/index.html")}`);
        }
        if (url.hostname === "flags") {
          const headers = new Headers({ "Content-Type": "text/html; charset=utf-8" });
          return new Response(renderFlagsHtml(flags), { status: 200, headers });
        }
        return new Response("Not found", { status: 404 });
      } catch (error) {
        return new Response(`Server error: ${error && error.message}`, { status: 500 });
      }
    });
  });
}

module.exports = {
  DEFAULT_FLAGS,
  FLAGS_HINT,
  flagsFilePath,
  installAstraProtocol,
  loadFlags,
  saveFlags
};
