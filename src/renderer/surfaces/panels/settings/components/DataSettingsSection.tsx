import type { ChangeEvent, RefObject } from "react";

import type { WorkspaceStorageUsage } from "../../../../app/controller/useProfileStorageUsage";
import type { MemorySaverState } from "../model/memorySaverState";
import { MemorySaverSection } from "./MemorySaverSection";
import { ProfileStorageSection } from "./ProfileStorageSection";

export function DataSettingsSection({
  dataSummary,
  importInputRef,
  importStatus,
  memorySaver,
  onClearBrowsingData,
  onClearProfile,
  onExportBackup,
  onImportBackup,
  onRefreshProfileStorage,
  onSleepInactiveTabs,
  profileStorageEntries,
  profileStorageError,
  profileStorageStatus
}: {
  dataSummary: string;
  importInputRef: RefObject<HTMLInputElement | null>;
  importStatus: string | null;
  memorySaver: MemorySaverState;
  onClearBrowsingData: () => void;
  onClearProfile: (workspaceId: string) => void;
  onExportBackup: () => void;
  onImportBackup: (event: ChangeEvent<HTMLInputElement>) => void;
  onRefreshProfileStorage: () => void;
  onSleepInactiveTabs: () => void;
  profileStorageEntries: WorkspaceStorageUsage[];
  profileStorageError: string | null;
  profileStorageStatus: string;
}) {
  return (
    <section className="settings-pane" aria-label="Data settings">
      <section className="settings-section" aria-label="Browsing data">
        <div className="section-copy">
          <span>Browsing data</span>
          <span>{dataSummary}</span>
        </div>
        <button className="toolbar-button" type="button" onClick={onClearBrowsingData}>Clear</button>
      </section>
      <MemorySaverSection memorySaver={memorySaver} onSleepInactiveTabs={onSleepInactiveTabs} />
      <section className="settings-section" aria-label="Browser backup">
        <div className="section-copy">
          <span>Browser backup</span>
          <span>{importStatus ?? "Export or import Spaces, tabs, favorites, history, and settings"}</span>
        </div>
        <div className="button-cluster">
          <button className="toolbar-button" type="button" onClick={onExportBackup}>Export</button>
          <button className="toolbar-button" type="button" onClick={() => importInputRef.current?.click()}>Import</button>
        </div>
        <input
          ref={importInputRef}
          className="hidden-file-input"
          type="file"
          accept="application/json,.json"
          onChange={onImportBackup}
        />
      </section>
      <ProfileStorageSection
        entries={profileStorageEntries}
        error={profileStorageError}
        onClearProfile={onClearProfile}
        onRefresh={onRefreshProfileStorage}
        status={profileStorageStatus}
      />
    </section>
  );
}
