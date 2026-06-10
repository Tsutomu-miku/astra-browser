/* eslint-disable max-lines */
// SettingsPanel is the composition root for all 21 settings sections.
// Splitting it further would require stitching props through another
// intermediate layer without reducing the number of owned state variables.
import { type ChangeEvent, useMemo, useRef, useState } from "react";
import { FiX } from "react-icons/fi";

import { createBrowserStateBackup, parseBrowserStateBackup } from "../../platform/persistence/browserBackup";
import type { BrowserController } from "../../app/controller/types";
import { useMemoryUsage } from "../../app/controller/useMemoryUsage";
import { useProfileStorageUsage } from "../../app/controller/useProfileStorageUsage";
import { getMemorySaverState } from "../../common/memory/memorySaverState";
import type { PasswordEntry } from "../../domain/browser";
import {
  buildTranslateUrl,
  type ReaderSettings,
  type SearchEngineKey,
  type StartupBehavior,
  type TranslationSettings
} from "../../domain/browser";
import type { LegacyPanelProps } from "./settings/components/legacy/LegacyPanelRenders";
import { renderLegacyPanels } from "./settings/components/legacy/LegacyPanelRenders";
import type { M1PanelProps } from "./settings/components/M1PanelRenders";
import { renderM1Panels } from "./settings/components/M1PanelRenders";
import { createPasswordEntry } from "../../domain/browser/passwordVault";
import { PasswordEditorDialog } from "./settings/components/autofill/PasswordEditorDialog";
import { SettingsSectionNav } from "./settings/components/SettingsSectionNav";
import { UpcomingSettingsSection } from "./settings/components/UpcomingSettingsSection";
import {
  SETTINGS_SECTIONS,
  isInteractiveSection,
  type SettingsSectionId
} from "./settings/model/settingsSections";

export function getDataSummary(history: number, downloads: number, permissions: number): string {
  return `${history} history · ${downloads} downloads · ${permissions} permissions`;
}

export function SettingsPanel({ controller }: { controller: BrowserController }) {
  const { actions, activeWorkspace, setPanel, state } = controller;
  const profileStorage = useProfileStorageUsage(state.workspaces);
  const memoryUsage = useMemoryUsage(state.workspaces, profileStorage.entries);
  const memorySaver = getMemorySaverState(activeWorkspace, state);
  const [activeSection, setActiveSection] = useState<SettingsSectionId>("you-and-astra");
  const importInputRef = useRef<HTMLInputElement | null>(null);
  const [importStatus, setImportStatus] = useState<string | null>(null);
  const [historyQuery, setHistoryQuery] = useState("");
  const bookmarksImportInputRef = useRef<HTMLInputElement | null>(null);
  const [bookmarksImportStatus, setBookmarksImportStatus] = useState<string | null>(null);
  const [passwordSearchQuery, setPasswordSearchQuery] = useState("");
  const [editingPassword, setEditingPassword] = useState<PasswordEntry | null>(null);

  const sectionById = useMemo(() => {
    const map = new Map<SettingsSectionId, (typeof SETTINGS_SECTIONS)[number]>();
    for (const s of SETTINGS_SECTIONS) map.set(s.id, s);
    return map;
  }, []);

  const exportBackup = () => {
    const url = URL.createObjectURL(new Blob([createBrowserStateBackup(state)], { type: "application/json" }));
    const link = document.createElement("a");
    link.href = url;
    link.download = `astra-browser-backup-${new Date().toISOString().slice(0, 10)}.json`;
    link.click();
    URL.revokeObjectURL(url);
  };

  const importBackup = async (event: ChangeEvent<HTMLInputElement>) => {
    const file = event.target.files?.[0];
    event.target.value = "";
    if (!file) return;
    try {
      actions.replaceBrowserState(parseBrowserStateBackup(await file.text()));
      setImportStatus(`Imported ${file.name}`);
    } catch {
      setImportStatus("Import failed");
    }
  };

  const importBookmarks = async (event: ChangeEvent<HTMLInputElement>) => {
    const file = event.target.files?.[0];
    event.target.value = "";
    if (!file) return;
    try {
      const html = await file.text();
      const summary = actions.importBookmarks(html, { source: "html" });
      const parts: string[] = [];
      if (summary.essentialsAdded) parts.push(`${summary.essentialsAdded} essentials`);
      if (summary.favoritesAdded) parts.push(`${summary.favoritesAdded} favorites`);
      setBookmarksImportStatus(`Imported ${file.name} — ${parts.join(" + ") || "0 bookmarks"}`);
    } catch {
      setBookmarksImportStatus("Bookmarks import failed");
    }
  };

  const onUpdateSettings = (patch: Partial<ReaderSettings | TranslationSettings | typeof state.settings>) => {
    actions.updateSettings(patch as never);
  };

  const active = sectionById.get(activeSection);
  const activeWorkspaceForTabs = state.workspaces.find((ws) => ws.id === state.activeWorkspaceId);
  const activeTab = activeWorkspaceForTabs?.tabs.find((tab) => tab.id === activeWorkspaceForTabs?.activeTabId);
  const onClearSitePermission = (profileId: string, origin: string, permission: string) =>
    actions.clearSitePermission(profileId, origin, permission);

  const historyCount = state.history.length;
  const downloadCount = state.downloads.length;
  const permissionCount = state.sitePermissions.length;
  const autofillCount =
    state.settings.autofill.passwords.length +
    state.settings.autofill.addresses.length +
    state.settings.autofill.paymentMethods.length;
  const activeProfileId = state.settings.activeProfileId ?? state.profiles[0]?.id ?? "personal";

  const legacyPanelProps: LegacyPanelProps = {
    activeWorkspace,
    activeWorkspaceId: activeWorkspace.id,
    importInputRef,
    importStatus,
    memoryBreakdown: memoryUsage.breakdown,
    memoryError: memoryUsage.error,
    memoryHistory: memoryUsage.history,
    memorySaver,
    memoryStatus: memoryUsage.status,
    onClearBrowsingData: actions.clearBrowsingData,
    onClearProfile: (workspaceId: string) => {
      actions.clearWorkspaceBrowsingData(workspaceId);
      window.setTimeout(() => void profileStorage.refresh(), 250);
    },
    onExportBackup: exportBackup,
    onImportBackup: importBackup,
    onRefreshMemory: () => void memoryUsage.refresh(),
    onRefreshProfileStorage: profileStorage.refresh,
    onSleepInactiveTabs: actions.sleepInactiveTabs,
    onUpdateMemorySaver: actions.updateSettings,
    onUpdateSettings: actions.updateSettings,
    onUpdateWorkspace: actions.updateWorkspace,
    onUpsertPerOriginZoom: (origin: string, zoom: number) => actions.setPerOriginZoom(origin, zoom),
    onClearPerOriginZoom: actions.clearPerOriginZoom,
    onResetPerOriginZoom: actions.clearAllPerOriginZoomSettings,
    profileStorageEntries: profileStorage.entries,
    profileStorageError: profileStorage.error,
    profileStorageStatus: profileStorage.status,
    searchEngine: state.settings.searchEngine,
    settings: state.settings,
    workspaceCount: state.workspaces.length,
    onAddWorkspace: actions.addWorkspace,
    onDeleteWorkspace: actions.deleteWorkspace,
    historyCount,
    downloadCount,
    permissionCount,
    bookmarksImportStatus,
    bookmarksImportInputRef,
    onImportBookmarks: importBookmarks
  };

  const m1PanelProps: M1PanelProps = {
    autofill: state.settings.autofill,
    onAddPassword: async () => setEditingPassword({
      id: "",
      origin: activeTab?.url || "https://",
      username: "",
      encryptedPassword: "",
      createdAt: Date.now(),
      updatedAt: Date.now()
    }),
    onEditPassword: (entry) => setEditingPassword(entry),
    onRevealPassword: (id) => actions.decryptPassword(id),
    onRemovePassword: actions.removePassword,
    passwordVaultUnlocked: controller?.passwordVaultUnlocked ?? false,
    onUnlockVault: (passphrase) => actions.unlockPasswordVault(passphrase),
    onLockVault: () => actions.lockPasswordVault(),
    passwordSearchQuery,
    setPasswordSearchQuery,
    onAddAddress: () => actions.upsertAddress({
      id: "",
      label: "Home",
      recipient: "New Recipient",
      address1: "123 Example St",
      city: "Shanghai",
      createdAt: Date.now()
    }),
    onRemoveAddress: actions.removeAddress,
    onAddPaymentMethod: () => actions.upsertPaymentMethod({
      id: "",
      label: "New card",
      cardholderName: "Your Name",
      cardLastFour: "0000",
      encryptedCardDetails: "",
      createdAt: Date.now(),
      updatedAt: Date.now()
    }),
    onRemovePaymentMethod: actions.removePaymentMethod,

    appearance: {
      settings: state.settings,
      onChange: actions.updateSettings,
      onClearPerOriginZoom: actions.clearPerOriginZoom,
      onResetPerOriginZoom: actions.clearAllPerOriginZoomSettings,
      onUpsertPerOriginZoom: actions.setPerOriginZoom
    },

    onClearBrowsingData: actions.clearBrowsingData,
    onClearProfile: (workspaceId: string) => {
      actions.clearWorkspaceBrowsingData(workspaceId);
      window.setTimeout(() => void profileStorage.refresh(), 250);
    },
    permissions: state.sitePermissions,
    onForgetPermission: (r) => onClearSitePermission(r.profileId, r.origin, r.permission),
    workspaces: state.workspaces.map((ws) => ({ id: ws.id, name: ws.name })),
    historyCount,
    downloadCount,
    onResetPermissionByKind: (kind) => state.sitePermissions
      .filter((r) => r.permission === kind)
      .forEach((r) => onClearSitePermission(r.profileId, r.origin, r.permission)),

    searchEngine: state.settings.searchEngine,
    onUpdateSearchEngine: (next: SearchEngineKey) => actions.updateSettings({ searchEngine: next }),
    startup: state.settings.startupBehavior,
    onUpdateStartup: (next: StartupBehavior) => actions.updateSettings({ startupBehavior: next }),

    downloads: state.downloads,
    onOpenDownload: (path: string) => void window.astraShell?.openPath(path),
    onOpenPath: (path: string) => void window.astraShell?.showItemInFolder(path),
    onCancelDownload: actions.cancelDownload,

    history: state.history,
    historyQuery,
    setHistoryQuery,
    onOpenHistory: (url: string, title: string) => actions.openUrlInActiveWorkspace(url, title),
    onClearHistoryEntry: actions.removeHistoryEntry,
    clearHistory: actions.clearHistory,

    reader: state.settings.reader,
    onUpdateReader: (patch: Partial<ReaderSettings>) =>
      onUpdateSettings({ reader: { ...state.settings.reader, ...patch } }),

    translation: state.settings.translation,
    activeUrl: activeTab?.url ?? null,
    onUpdateTranslation: (patch: Partial<TranslationSettings>) =>
      onUpdateSettings({ translation: { ...state.settings.translation, ...patch } }),
    onTranslateNow: (target: string) => {
      const url = buildTranslateUrl({
        provider: state.settings.translation.provider,
        url: activeTab?.url ?? "",
        targetLang: target
      });
      if (url) actions.openUrlInActiveWorkspace(url, `Translated: ${activeTab?.title ?? "Page"}`);
    },

    /* ===== M2.1 Profiles / Extensions ===== */
    profiles: state.profiles,
    activeProfileId,
    onSwitchProfile: (profileId) => {
      actions.switchProfile(profileId);
      actions.switchActiveProfile(profileId);
    },
    onAddProfile: (name, color) => actions.addProfile(name, color),
    onDeleteProfile: (id) => actions.removeProfile(id),
    extensions: state.extensions,
    onToggleExtension: (id, enabled) => actions.toggleExtensionEnabled(id, enabled),
    onUninstallExtension: (id) => actions.removeExtension(id),
    onInstallExtensionFromFile: () => {
      /* M2.1 MVP：安装入口占位，提示用户 Chrome Web Store 是主要来源 */
      setPanel("extensions" as never);
    },
    onOpenExtensionStore: () => actions.openUrlInActiveWorkspace(
      "https://chromewebstore.google.com",
      "Chrome Web Store"
    ),

    /* ===== M2.1 Print / System ===== */
    settings: state.settings,
    onChangeSettings: (patch) => actions.updateSettings(patch),
    onPrintActiveTab: (options) => actions.printActiveTab(options),
    onOpenFolder: (kind) => actions.openUserDataFolder(kind),
    onRestartBrowser: () => actions.restartBrowser(),
    autoUpdateStatus: "not configured",

    /* ===== M2.1 Reset-and-cleanup ===== */
    onResetSettings: () => actions.resetSettings(),
    onClearAllBrowsingData: () => actions.clearBrowsingData(),
    onClearHistory: () => actions.clearHistory(),
    onClearDownloads: () => actions.clearAllDownloads(),
    browsingDataCount: {
      history: historyCount,
      downloads: downloadCount,
      permissions: permissionCount,
      autofill: autofillCount
    },

    /* ===== M2.4 W-3 Installed apps ===== */
    installedApps: state.installedPwaApps,
    onLaunchInstalledApp: (origin) => actions.launchInstalledPwa(origin),
    onUninstallApp: (origin) => actions.uninstallPwa(origin),
    onRefreshInstalledApps: () => actions.reloadInstalledPwaApps()
  };

  return (
    <aside className="settings-panel">
      <header className="panel-header">
        <h2>Settings</h2>
        <button className="icon-button" title="Close settings" type="button" onClick={() => setPanel(null)}>
          <FiX />
        </button>
      </header>
      <form className="settings-form" onSubmit={(event) => event.preventDefault()}>
        <SettingsSectionNav activeSection={activeSection} onSelect={setActiveSection} />
        <div className="settings-panels">
          {renderLegacyPanels(activeSection, legacyPanelProps)}
          {renderM1Panels(activeSection, m1PanelProps)}
          {activeSection === "about" && <AboutPanel />}
          {!isInteractiveSection(activeSection) && active && (
            <UpcomingSettingsSection section={active} />
          )}
        </div>
      </form>
      {editingPassword && (
        <PasswordEditorDialog
          entry={editingPassword}
          onClose={() => setEditingPassword(null)}
          onSave={async (next) => {
            // Only encrypt once we have a non-empty plaintext password.
            // Existing PasswordEntry keeps encryptedPassword if no plaintext
            // replacement was provided (user only edited username/notes).
            const isNew = !editingPassword.id;
            const needEncryption = Boolean(next.plaintextPassword) || isNew;
            let encryptedPassword = editingPassword.encryptedPassword;
            if (needEncryption) {
              try {
                if (!controller?.passwordVaultUnlocked) await actions.unlockPasswordVault();
                if (next.plaintextPassword) {
                  const created = await createPasswordEntry({
                    origin: next.origin,
                    username: next.username,
                    password: next.plaintextPassword,
                    notes: next.notes
                  });
                  encryptedPassword = created.encryptedPassword;
                }
              } catch (err) {
                setEditingPassword(null);
                return err instanceof Error ? err.message : String(err);
              }
            }
            actions.upsertPassword({
              id: editingPassword.id || `pwd-${Date.now()}`,
              origin: next.origin,
              username: next.username,
              notes: next.notes,
              encryptedPassword,
              createdAt: editingPassword.createdAt || Date.now(),
              updatedAt: Date.now()
            });
            setEditingPassword(null);
            return null;
          }}
        />
      )}
    </aside>
  );
}

function AboutPanel() {
  return (
    <section className="settings-pane" aria-label="About Astra">
      <div className="about-panel-grid">
        <label className="field"><span>App</span><strong>Astra Browser</strong></label>
        <label className="field">
          <span>Version</span>
          <code>{typeof window.astraShell?.getVersion === "function" ? "(loaded via main IPC)" : "(dev mode)"}</code>
        </label>
        <label className="field"><span>Engine</span><code>Electron 42 · Chromium</code></label>
        <label className="field"><span>Open source</span>
          <a href="https://github.com/Tsutomu-miku/astra-browser" rel="noreferrer" target="_blank">
            github.com/Tsutomu-miku/astra-browser
          </a>
        </label>
        <p className="muted">
          M0 skeleton only. Full About page with license details, changelog,
          and auto-update progress will land in M2 alongside W-10 (auto-update)
          and W-11 (notarization / EV signing).
        </p>
      </div>
    </section>
  );
}
