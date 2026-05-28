import { type ChangeEvent, useRef, useState } from "react";
import { FiX } from "react-icons/fi";

import { createBrowserStateBackup, parseBrowserStateBackup } from "../../platform/persistence/browserBackup";
import type { BrowserController } from "../../app/controller/types";
import { useProfileStorageUsage } from "../../app/controller/useProfileStorageUsage";
import { getMemorySaverState } from "../../common/memory/memorySaverState";
import { DataSettingsSection } from "./settings/components/DataSettingsSection";
import { GlobalSettingsSection } from "./settings/components/GlobalSettingsSection";
import { SettingsSectionNav } from "./settings/components/SettingsSectionNav";
import { SpaceSettingsSection } from "./settings/components/SpaceSettingsSection";
import { WorkspaceManagementSection } from "./settings/components/WorkspaceManagementSection";
import type { SettingsSectionId } from "./settings/model/settingsSections";

export function SettingsPanel({ controller }: { controller: BrowserController }) {
  const { actions, activeWorkspace, setPanel, state } = controller;
  const profileStorage = useProfileStorageUsage(state.workspaces);
  const memorySaver = getMemorySaverState(activeWorkspace, state);
  const [activeSection, setActiveSection] = useState<SettingsSectionId>("global");
  const importInputRef = useRef<HTMLInputElement | null>(null);
  const [importStatus, setImportStatus] = useState<string | null>(null);

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

  return (
    <aside className="settings-panel">
      <header className="panel-header">
        <h2>Settings</h2>
        <button className="icon-button" title="Close settings" type="button" onClick={() => setPanel(null)}><FiX /></button>
      </header>
      <form className="settings-form" onSubmit={(event) => event.preventDefault()}>
        <SettingsSectionNav activeSection={activeSection} onSelect={setActiveSection} />
        {activeSection === "global" && (
          <GlobalSettingsSection settings={state.settings} onUpdateSettings={actions.updateSettings} />
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
            memorySaver={memorySaver}
            onClearBrowsingData={actions.clearBrowsingData}
            onClearProfile={(workspaceId) => {
              actions.clearWorkspaceBrowsingData(workspaceId);
              window.setTimeout(() => void profileStorage.refresh(), 250);
            }}
            onExportBackup={exportBackup}
            onImportBackup={importBackup}
            onRefreshProfileStorage={profileStorage.refresh}
            onSleepInactiveTabs={actions.sleepInactiveTabs}
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
      </form>
    </aside>
  );
}

function getDataSummary(history: number, downloads: number, permissions: number): string {
  return `${history} history · ${downloads} downloads · ${permissions} permissions`;
}
