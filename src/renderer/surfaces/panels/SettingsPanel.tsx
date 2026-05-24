import { type ChangeEvent, useRef, useState } from "react";

import {
  formatBytes,
  normalizeAddress,
  SEARCH_ENGINES,
  type SearchEngineKey,
  type StartupBehavior
} from "../../domain/browser-core";
import { createBrowserStateBackup, parseBrowserStateBackup } from "../../hooks/browserBackup";
import type { BrowserController } from "../../hooks/types";
import { useProfileStorageUsage, type WorkspaceStorageUsage } from "../../hooks/useProfileStorageUsage";

export function SettingsPanel({ controller }: { controller: BrowserController }) {
  const { actions, activeWorkspace, setPanel, state } = controller;
  const profileStorage = useProfileStorageUsage(state.workspaces);
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
        <button className="icon-button" title="Close settings" type="button" onClick={() => setPanel(null)}>×</button>
      </header>
      <form className="settings-form" onSubmit={(event) => event.preventDefault()}>
        <label className="field">
          <span>Homepage</span>
          <input
            autoComplete="off"
            spellCheck={false}
            value={state.settings.homepage}
            onChange={(event) => actions.updateSettings({ homepage: event.target.value })}
            onBlur={(event) => actions.updateSettings({ homepage: normalizeAddress(event.target.value, state.settings.searchEngine) })}
          />
        </label>
        <label className="field">
          <span>Search engine</span>
          <select
            value={state.settings.searchEngine}
            onChange={(event) => actions.updateSettings({ searchEngine: event.target.value as SearchEngineKey })}
          >
            {Object.entries(SEARCH_ENGINES).map(([key, engine]) => (
              <option key={key} value={key}>{engine.name}</option>
            ))}
          </select>
        </label>
        <label className="field">
          <span>Startup</span>
          <select
            value={state.settings.startupBehavior}
            onChange={(event) => actions.updateSettings({ startupBehavior: event.target.value as StartupBehavior })}
          >
            <option value="restore">Restore previous session</option>
            <option value="homepage">Open homepage in each Space</option>
          </select>
        </label>
        <label className="field">
          <span>Workspace name</span>
          <input value={activeWorkspace.name} onChange={(event) => actions.updateWorkspace({ name: event.target.value })} />
        </label>
        <label className="field">
          <span>Workspace accent</span>
          <input type="color" value={activeWorkspace.accent} onChange={(event) => actions.updateWorkspace({ accent: event.target.value })} />
        </label>
        <label className="field">
          <span>Workspace homepage</span>
          <input
            autoComplete="off"
            spellCheck={false}
            value={activeWorkspace.homepage}
            onChange={(event) => actions.updateWorkspace({ homepage: event.target.value })}
            onBlur={(event) => actions.updateWorkspace({ homepage: normalizeAddress(event.target.value, state.settings.searchEngine) })}
          />
        </label>
        <label className="field">
          <span>Workspace profile</span>
          <input value={activeWorkspace.profileName} onChange={(event) => actions.updateWorkspace({ profileName: event.target.value })} />
        </label>
        <section className="settings-section" aria-label="Browsing data">
          <div className="section-copy">
            <span>Browsing data</span>
            <span>{getDataSummary(state.history.length, state.downloads.length, state.sitePermissions.length)}</span>
          </div>
          <button className="toolbar-button" type="button" onClick={actions.clearBrowsingData}>Clear</button>
        </section>
        <section className="settings-section" aria-label="Browser backup">
          <div className="section-copy">
            <span>Browser backup</span>
            <span>{importStatus ?? "Export or import Spaces, tabs, favorites, history, and settings"}</span>
          </div>
          <div className="button-cluster">
            <button className="toolbar-button" type="button" onClick={exportBackup}>Export</button>
            <button className="toolbar-button" type="button" onClick={() => importInputRef.current?.click()}>Import</button>
          </div>
          <input
            ref={importInputRef}
            className="hidden-file-input"
            type="file"
            accept="application/json,.json"
            onChange={importBackup}
          />
        </section>
        <ProfileStorageSection
          entries={profileStorage.entries}
          error={profileStorage.error}
          onClearProfile={(workspaceId) => {
            actions.clearWorkspaceBrowsingData(workspaceId);
            window.setTimeout(() => void profileStorage.refresh(), 250);
          }}
          onRefresh={profileStorage.refresh}
          status={profileStorage.status}
        />
        <section className="settings-section" aria-label="Workspace management">
          <div className="section-copy">
            <span>Workspaces</span>
            <span>{state.workspaces.length} spaces</span>
          </div>
          <div className="button-cluster">
            <button className="toolbar-button" type="button" onClick={actions.addWorkspace}>New</button>
            <button
              className="toolbar-button"
              type="button"
              disabled={state.workspaces.length <= 1}
              onClick={() => actions.deleteWorkspace(activeWorkspace.id)}
            >
              Delete
            </button>
          </div>
        </section>
      </form>
    </aside>
  );
}

function ProfileStorageSection({
  entries,
  error,
  onClearProfile,
  onRefresh,
  status
}: {
  entries: WorkspaceStorageUsage[];
  error: string | null;
  onClearProfile: (workspaceId: string) => void;
  onRefresh: () => void;
  status: string;
}) {
  return (
    <section className="settings-section is-stacked" aria-label="Profile storage">
      <div className="section-copy">
        <span>Profile storage</span>
        <span>{getStorageStatusLabel(status, error)}</span>
      </div>
      <button className="toolbar-button" type="button" disabled={status === "loading"} onClick={onRefresh}>
        Refresh
      </button>
      <div className="profile-storage-list">
        {entries.map((entry) => (
          <ProfileStorageRow
            entry={entry}
            key={`${entry.workspaceId}-${entry.partition}`}
            onClearProfile={onClearProfile}
          />
        ))}
      </div>
    </section>
  );
}

function ProfileStorageRow({
  entry,
  onClearProfile
}: {
  entry: WorkspaceStorageUsage;
  onClearProfile: (workspaceId: string) => void;
}) {
  return (
    <article className="profile-storage-row" title={entry.storagePath ?? entry.partition}>
      <div className="profile-storage-main">
        <span className="profile-storage-name">{entry.workspaceName}</span>
        <span className="profile-storage-profile">{entry.profileName} · {entry.partition}</span>
      </div>
      <div className="profile-storage-metrics">
        <span>{formatBytes(entry.totalBytes)}</span>
        <span>{formatBytes(entry.cacheBytes)} cache</span>
      </div>
      <button className="toolbar-button" type="button" onClick={() => onClearProfile(entry.workspaceId)}>Clear</button>
    </article>
  );
}

function getDataSummary(history: number, downloads: number, permissions: number): string {
  return `${history} history · ${downloads} downloads · ${permissions} permissions`;
}

function getStorageStatusLabel(status: string, error: string | null): string {
  if (status === "loading") return "Inspecting Chromium profiles";
  if (status === "unavailable") return "Available in Electron runtime";
  if (status === "error") return error ?? "Unable to inspect storage";
  return "Chromium cache and profile data by Space";
}
