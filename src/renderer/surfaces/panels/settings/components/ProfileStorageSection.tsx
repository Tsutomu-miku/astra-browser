import { formatBytes } from "../../../../domain/browser";
import type { WorkspaceStorageUsage } from "../../../../app/controller/useProfileStorageUsage";

export function ProfileStorageSection({
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

function getStorageStatusLabel(status: string, error: string | null): string {
  if (status === "loading") return "Inspecting Chromium profiles";
  if (status === "unavailable") return "Available in Electron runtime";
  if (status === "error") return error ?? "Unable to inspect storage";
  return "Chromium cache and profile data by Space";
}
