import type { ChangeEvent, RefObject } from "react";

import type { MemoryUsageBreakdown } from "../../../../common/memory/memoryUsage";
import type { WorkspaceStorageUsage } from "../../../../app/controller/useProfileStorageUsage";
import type { MemoryUsageStatus } from "../../../../app/controller/useMemoryUsage";
import type { MemorySaverState } from "../../../../common/memory/memorySaverState";
import { MemorySaverSection } from "./MemorySaverSection";
import { MemoryUsagePanel } from "./MemoryUsagePanel";
import { ProfileStorageSection } from "./ProfileStorageSection";

export function DataSettingsSection({
  dataSummary,
  importInputRef,
  importStatus,
  memoryBreakdown,
  memoryHistory,
  memorySaver,
  memoryStatus,
  memoryError,
  onClearBrowsingData,
  onClearProfile,
  onExportBackup,
  onImportBackup,
  onRefreshMemory,
  onRefreshProfileStorage,
  onSleepInactiveTabs,
  onUpdateMemorySaver,
  profileStorageEntries,
  profileStorageError,
  profileStorageStatus,
  bookmarksImportStatus,
  bookmarksImportInputRef,
  onImportBookmarks
}: {
  dataSummary: string;
  importInputRef: RefObject<HTMLInputElement | null>;
  importStatus: string | null;
  memoryBreakdown: MemoryUsageBreakdown;
  memoryError: string | null;
  memoryHistory: number[];
  memorySaver: MemorySaverState;
  memoryStatus: MemoryUsageStatus;
  onClearBrowsingData: () => void;
  onClearProfile: (workspaceId: string) => void;
  onExportBackup: () => void;
  onImportBackup: (event: ChangeEvent<HTMLInputElement>) => void;
  onRefreshMemory: () => void;
  onRefreshProfileStorage: () => void;
  onSleepInactiveTabs: () => void;
  onUpdateMemorySaver: (patch: { memorySaverEnabled?: boolean; memorySaverIdleMinutes?: number }) => void;
  profileStorageEntries: WorkspaceStorageUsage[];
  profileStorageError: string | null;
  profileStorageStatus: string;
  bookmarksImportStatus: string | null;
  bookmarksImportInputRef: RefObject<HTMLInputElement | null>;
  onImportBookmarks: (event: ChangeEvent<HTMLInputElement>) => void;
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
      <MemorySaverSection
        memorySaver={memorySaver}
        onSleepInactiveTabs={onSleepInactiveTabs}
        onUpdateMemorySaver={onUpdateMemorySaver}
      />
      <MemoryUsagePanel
        breakdown={memoryBreakdown}
        error={memoryError}
        history={memoryHistory}
        memorySaver={memorySaver}
        onRefresh={onRefreshMemory}
        status={memoryStatus}
      />
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
      <section className="settings-section" aria-label="Bookmarks import">
        <div className="section-copy">
          <span>Bookmarks import</span>
          <span>{bookmarksImportStatus ?? "Import Chrome / Edge / Firefox / Safari Netscape bookmarks.html"}</span>
        </div>
        <div className="button-cluster">
          <button className="toolbar-button" type="button" onClick={() => bookmarksImportInputRef.current?.click()}>
            Import bookmarks (HTML)
          </button>
        </div>
        <input
          ref={bookmarksImportInputRef}
          className="hidden-file-input"
          type="file"
          accept="text/html,.html"
          onChange={onImportBookmarks}
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
