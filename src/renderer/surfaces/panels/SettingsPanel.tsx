import { type ChangeEvent, useMemo, useRef, useState } from "react";
import { FiX } from "react-icons/fi";

import { createBrowserStateBackup, parseBrowserStateBackup } from "../../platform/persistence/browserBackup";
import type { BrowserController } from "../../app/controller/types";
import { useMemoryUsage } from "../../app/controller/useMemoryUsage";
import { useProfileStorageUsage } from "../../app/controller/useProfileStorageUsage";
import { getMemorySaverState } from "../../common/memory/memorySaverState";
import { DataSettingsSection } from "./settings/components/DataSettingsSection";
import { GlobalSettingsSection } from "./settings/components/GlobalSettingsSection";
import { SettingsSectionNav } from "./settings/components/SettingsSectionNav";
import { SpaceSettingsSection } from "./settings/components/SpaceSettingsSection";
import { UpcomingSettingsSection } from "./settings/components/UpcomingSettingsSection";
import { WorkspaceManagementSection } from "./settings/components/WorkspaceManagementSection";
import {
  SETTINGS_SECTIONS,
  isLegacyRealSection,
  type SettingsSectionId
} from "./settings/model/settingsSections";

export function SettingsPanel({ controller }: { controller: BrowserController }) {
  const { actions, activeWorkspace, setPanel, state } = controller;
  const profileStorage = useProfileStorageUsage(state.workspaces);
  const memoryUsage = useMemoryUsage(state.workspaces, profileStorage.entries);
  const memorySaver = getMemorySaverState(activeWorkspace, state);
  const [activeSection, setActiveSection] = useState<SettingsSectionId>("you-and-astra");
  const importInputRef = useRef<HTMLInputElement | null>(null);
  const [importStatus, setImportStatus] = useState<string | null>(null);

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

  const active = sectionById.get(activeSection);

  return (
    <aside className="settings-panel">
      <header className="panel-header">
        <h2>Settings</h2>
        <button className="icon-button" title="Close settings" type="button" onClick={() => setPanel(null)}><FiX /></button>
      </header>
      <form className="settings-form" onSubmit={(event) => event.preventDefault()}>
        <SettingsSectionNav activeSection={activeSection} onSelect={setActiveSection} />
        <div className="settings-panels">
          {/* Legacy 4 panels (with real content). */}
          {activeSection === "global" && (
            <GlobalSettingsSection
              settings={state.settings}
              onUpdateSettings={actions.updateSettings}
              onUpsertPerOriginZoom={(origin, zoom) => actions.setPerOriginZoom(origin, zoom)}
              onClearPerOriginZoom={(origin) => actions.clearPerOriginZoom(origin)}
              onResetPerOriginZoom={() => actions.clearAllPerOriginZoomSettings()}
            />
          )}
          {activeSection === "space" && (
            <SpaceSettingsSection
              workspace={activeWorkspace}
              searchEngine={state.settings.searchEngine}
              onUpdateWorkspace={actions.updateWorkspace}
            />
          )}
          {activeSection === "data" && (
            <DataSettingsSection
              dataSummary={getDataSummary(state.history.length, state.downloads.length, state.sitePermissions.length)}
              importInputRef={importInputRef}
              importStatus={importStatus}
              memoryBreakdown={memoryUsage.breakdown}
              memoryError={memoryUsage.error}
              memoryHistory={memoryUsage.history}
              memorySaver={memorySaver}
              memoryStatus={memoryUsage.status}
              onClearBrowsingData={actions.clearBrowsingData}
              onClearProfile={(workspaceId) => {
                actions.clearWorkspaceBrowsingData(workspaceId);
                window.setTimeout(() => void profileStorage.refresh(), 250);
              }}
              onExportBackup={exportBackup}
              onImportBackup={importBackup}
              onRefreshMemory={() => void memoryUsage.refresh()}
              onRefreshProfileStorage={profileStorage.refresh}
              onSleepInactiveTabs={actions.sleepInactiveTabs}
              onUpdateMemorySaver={actions.updateSettings}
              profileStorageEntries={profileStorage.entries}
              profileStorageError={profileStorage.error}
              profileStorageStatus={profileStorage.status}
            />
          )}
          {activeSection === "workspaces" && (
            <WorkspaceManagementSection
              activeWorkspaceId={activeWorkspace.id}
              onAddWorkspace={actions.addWorkspace}
              onDeleteWorkspace={actions.deleteWorkspace}
              workspaceCount={state.workspaces.length}
            />
          )}

          {/* About panel is small enough to do real content at M0. */}
          {activeSection === "about" && <AboutPanel />}

          {/* New 16 Chrome-style panels: M0 is skeleton only. */}
          {!isLegacyRealSection(activeSection) && activeSection !== "about" && active && (
            <UpcomingSettingsSection section={active} />
          )}
        </div>
      </form>
    </aside>
  );
}

function getDataSummary(history: number, downloads: number, permissions: number): string {
  return `${history} history · ${downloads} downloads · ${permissions} permissions`;
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
