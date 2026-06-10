const { BrowserWindow, app } = require("electron");
const path = require("node:path");

/**
 * PWA install MVP (W-3):
 *   - Catches `beforeinstallprompt` on every webContents (webview included) and
 *     defers it so the renderer can surface an "Install this site" affordance.
 *   - Stores one deferred prompt per origin; renderer confirms via
 *     `pwa:confirm-install` → we call prompt() + launch a standalone window if
 *     the user accepts.
 *   - Exposes `pwa:launch` for the installed-apps list; launch creates a
 *     standalone minimal BrowserWindow pointing at the saved start_url.
 *
 * Installed apps (id, origin, name, startUrl, icon) are kept in a small
 * in-memory Map for MVP. Persistence through main-process settings storage is
 * scheduled for M2.5 when the "Installed apps" settings panel ships.
 */

/** @type {Map<string, { prompt: () => Promise<{ outcome: string }>; platforms: string[] }>} */
const pendingByOrigin = new Map();

/** @type {Map<string, { id: string; origin: string; name: string; startUrl: string; icon?: string }>} */
const installedApps = new Map();

function originFor(url) {
  try {
    return new URL(url).origin;
  } catch {
    return "";
  }
}

function installPwaListeners({ sendToAll }) {
  app.on("web-contents-created", (_event, contents) => {
    contents.on("beforeinstallprompt", (event) => {
      // Chromium emits this after it has verified a valid manifest + service
      // worker; we cancel the automatic mini-infobar so Astra's own UI owns it.
      event.preventDefault();
      const origin = originFor(contents.getURL?.() ?? "");
      if (!origin) return;

      // We preserve a deferred callable. Electron's Event doesn't clone over
      // IPC, so we wrap it and reply with a plain payload.
      pendingByOrigin.set(origin, {
        prompt: () => event.prompt?.(),
        platforms: Array.isArray(event.platforms) ? event.platforms : []
      });

      const payload = {
        origin,
        platforms: Array.isArray(event.platforms) ? event.platforms : [],
        title: contents.getTitle?.() || origin,
        url: contents.getURL?.() || ""
      };
      sendToAll("pwa:install-prompt-available", payload);
    });

    contents.on("app-installed", (_e, details) => {
      const origin = originFor(details?.manifestUrl ?? contents.getURL?.() ?? "");
      if (!origin) return;
      const record = {
        id: origin,
        origin,
        name: details?.name || contents.getTitle?.() || origin,
        startUrl: details?.startUrl || origin + "/",
        icon: details?.icon || undefined
      };
      installedApps.set(origin, record);
      pendingByOrigin.delete(origin);
      sendToAll("pwa:app-installed", record);
    });
  });
}

async function confirmInstall(origin) {
  const pending = pendingByOrigin.get(origin);
  if (!pending) return { accepted: false, reason: "no-pending-prompt" };
  try {
    const result = typeof pending.prompt === "function" ? await pending.prompt() : { outcome: "dismissed" };
    return { accepted: result?.outcome === "accepted", outcome: result?.outcome };
  } catch (err) {
    return { accepted: false, reason: err instanceof Error ? err.message : String(err) };
  } finally {
    pendingByOrigin.delete(origin);
  }
}

function launchInstalledApp(originOrId) {
  const appRecord = installedApps.get(originOrId);
  if (!appRecord) return { ok: false, reason: "not-installed" };

  const win = new BrowserWindow({
    width: 1280,
    height: 800,
    minWidth: 420,
    minHeight: 320,
    title: appRecord.name,
    backgroundColor: "#ffffff",
    autoHideMenuBar: true,
    titleBarStyle: "default",
    webPreferences: {
      preload: path.join(__dirname, "preload.js"),
      contextIsolation: true,
      sandbox: false,
      plugins: true,
      // PWA windows get their own partition so cookie jars, storage, and service
      // workers are scoped per origin (isolating them from regular browsing).
      partition: `persist:astra-pwa-${appRecord.id}`,
      webviewTag: false
    }
  });

  win.setMenuBarVisibility(false);
  win.loadURL(appRecord.startUrl).catch(() => {
    win.loadURL(appRecord.origin);
  });
  require("./diagnostics").installWindowDiagnostics(win);

  win.once("closed", () => {
    try {
      // Best-effort: clear in-memory cached data when the user closes the
      // standalone window. Persistent cookies/localStorage survive because
      // the partition is persist: prefixed (intentional for offline PWA).
      win.webContents.session.clearCache().catch(() => {});
    } catch { /* ignore */ }
  });

  return { ok: true, name: appRecord.name, startUrl: appRecord.startUrl };
}

function uninstallApp(origin) {
  const existing = installedApps.get(origin);
  if (!existing) return { ok: false, reason: "not-installed" };
  installedApps.delete(origin);
  return { ok: true, origin };
}

function listInstalledApps() {
  return Array.from(installedApps.values());
}

function isPendingInstall(origin) {
  return pendingByOrigin.has(origin);
}

module.exports = {
  installPwaListeners,
  confirmInstall,
  launchInstalledApp,
  uninstallApp,
  listInstalledApps,
  isPendingInstall
};
