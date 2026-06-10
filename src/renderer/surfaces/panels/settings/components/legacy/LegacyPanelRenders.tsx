import type { ChangeEvent, RefObject } from "react";

import type { MemoryUsageBreakdown } from "../../../../../common/memory/memoryUsage";
import type { MemorySaverState } from "../../../../../common/memory/memorySaverState";
import type { MemoryUsageStatus } from "../../../../../app/controller/useMemoryUsage";
import type {
  BrowserSettings,
  SearchEngineKey,
  Workspace
} from "../../../../../domain/browser";
import type { WorkspaceStorageUsage } from "../../../../../app/controller/useProfileStorageUsage";
import { DataSettingsSection } from "../DataSettingsSection";
import { GlobalSettingsSection } from "../GlobalSettingsSection";
import { SpaceSettingsSection } from "../SpaceSettingsSection";
import { WorkspaceManagementSection } from "../WorkspaceManagementSection";
import type { SettingsSectionId } from "../../model/settingsSections";
import { getDataSummary } from "../../../SettingsPanel";

export interface LegacyPanelProps {
  activeWorkspace: Workspace;
  activeWorkspaceId: string;
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
  onUpdateMemorySaver: (patch: Partial<BrowserSettings>) => void;
  onUpdateSettings: (patch: Partial<BrowserSettings>) => void;
  onUpdateWorkspace: (workspace: Partial<Workspace>) => void;
  onUpsertPerOriginZoom: (origin: string, zoom: number) => void;
  onClearPerOriginZoom: (origin: string) => void;
  onResetPerOriginZoom: () => void;
  profileStorageEntries: WorkspaceStorageUsage[];
  profileStorageError: string | null;
  profileStorageStatus: string;
  searchEngine: SearchEngineKey;
  settings: BrowserSettings;
  workspaceCount: number;
  onAddWorkspace: () => void;
  onDeleteWorkspace: (workspaceId: string) => void;
  historyCount: number;
  downloadCount: number;
  permissionCount: number;
  bookmarksImportStatus: string | null;
  bookmarksImportInputRef: RefObject<HTMLInputElement | null>;
  onImportBookmarks: (event: ChangeEvent<HTMLInputElement>) => void;
}

export function renderLegacyPanels(
  activeSection: SettingsSectionId,
  p: LegacyPanelProps
) {
  switch (activeSection) {
    case "global":
      return (
        <GlobalSettingsSection
          settings={p.settings}
          onUpdateSettings={p.onUpdateSettings}
          onClearPerOriginZoom={p.onClearPerOriginZoom}
          onResetPerOriginZoom={p.onResetPerOriginZoom}
          onUpsertPerOriginZoom={p.onUpsertPerOriginZoom}
        />
      );
    case "space":
      return (
        <SpaceSettingsSection
          workspace={p.activeWorkspace}
          searchEngine={p.searchEngine}
          onUpdateWorkspace={p.onUpdateWorkspace}
        />
      );
    case "data":
      return (
        <DataSettingsSection
          dataSummary={getDataSummary(
            p.historyCount,
            p.downloadCount,
            p.permissionCount
          )}
          importInputRef={p.importInputRef}
          importStatus={p.importStatus}
          memoryBreakdown={p.memoryBreakdown}
          memoryError={p.memoryError}
          memoryHistory={p.memoryHistory}
          memorySaver={p.memorySaver}
          memoryStatus={p.memoryStatus}
          onClearBrowsingData={p.onClearBrowsingData}
          onClearProfile={p.onClearProfile}
          onExportBackup={p.onExportBackup}
          onImportBackup={p.onImportBackup}
          onRefreshMemory={p.onRefreshMemory}
          onRefreshProfileStorage={p.onRefreshProfileStorage}
          onSleepInactiveTabs={p.onSleepInactiveTabs}
          onUpdateMemorySaver={p.onUpdateMemorySaver}
          profileStorageEntries={p.profileStorageEntries}
          profileStorageError={p.profileStorageError}
          profileStorageStatus={p.profileStorageStatus}
          bookmarksImportStatus={p.bookmarksImportStatus}
          bookmarksImportInputRef={p.bookmarksImportInputRef}
          onImportBookmarks={p.onImportBookmarks}
        />
      );
    case "workspaces":
      return (
        <WorkspaceManagementSection
          activeWorkspaceId={p.activeWorkspaceId}
          onAddWorkspace={p.onAddWorkspace}
          onDeleteWorkspace={p.onDeleteWorkspace}
          workspaceCount={p.workspaceCount}
        />
      );
    default:
      return null;
  }
}
