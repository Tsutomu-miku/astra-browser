import { type DownloadEntry, type HistoryEntry, formatBytes } from "../../../../../domain/browser";
import {
  DangerButton,
  Empty,
  GroupHeader,
  NormalButton,
  SectionHeader
} from "../shared/SettingsUIPrimitives";

function downloadProgressPercent(entry: DownloadEntry): number {
  if (entry.totalBytes <= 0) return 0;
  return Math.min(100, Math.round((entry.receivedBytes / entry.totalBytes) * 100));
}

export function DownloadsSection({
  entries,
  onOpen,
  onOpenPath,
  onCancel
}: {
  entries: DownloadEntry[];
  onOpen: (entry: DownloadEntry) => void;
  onOpenPath: (entry: DownloadEntry) => void;
  onCancel: (id: string) => void;
}) {
  return (
    <section className="settings-pane" aria-label="Downloads">
      <SectionHeader title="Downloads" description="历史下载记录、进行中任务、打开 / 打开所在位置。" />
      {entries.length === 0 ? (
        <Empty text="尚无下载。下载启动后会自动显示在这里。" />
      ) : (
        <ul className="download-list">
          {entries.map((entry) => {
            const pct = downloadProgressPercent(entry);
            return (
              <li key={entry.id}>
                <div className="download-main">
                  <strong>{entry.filename}</strong>
                  <small>
                    {formatBytes(entry.receivedBytes)} / {formatBytes(entry.totalBytes)} · {entry.state}
                  </small>
                  <progress max={100} value={pct} />
                </div>
                <div className="row-actions">
                  <NormalButton onClick={() => onOpen(entry)}>Open</NormalButton>
                  <NormalButton onClick={() => onOpenPath(entry)}>Show in folder</NormalButton>
                  {entry.state === "progressing" ? (
                    <DangerButton onClick={() => onCancel(entry.id)}>Cancel</DangerButton>
                  ) : null}
                </div>
              </li>
            );
          })}
        </ul>
      )}
    </section>
  );
}

export function HistorySection({
  entries,
  query,
  setQuery,
  onOpen,
  onRemove,
  onClearAll
}: {
  entries: HistoryEntry[];
  query: string;
  setQuery: (q: string) => void;
  onOpen: (entry: HistoryEntry) => void;
  onRemove: (id: string) => void;
  onClearAll: () => void;
}) {
  const q = query.trim().toLowerCase();
  const filtered = entries
    .filter((e) => !q || e.title.toLowerCase().includes(q) || e.url.toLowerCase().includes(q))
    .sort((a, b) => b.visitedAt - a.visitedAt);
  const grouped = new Map<string, HistoryEntry[]>();
  for (const e of filtered) {
    const d = new Date(e.visitedAt).toDateString();
    if (!grouped.has(d)) grouped.set(d, []);
    grouped.get(d)!.push(e);
  }
  return (
    <section className="settings-pane" aria-label="History">
      <SectionHeader title="History" description="按天分组、支持搜索与批量删除（D-5）。" />
      <div className="history-toolbar">
        <input
          placeholder="Search history by title or URL"
          value={query}
          onChange={(e) => setQuery(e.target.value)}
        />
        <DangerButton onClick={onClearAll}>Clear all history</DangerButton>
      </div>
      {filtered.length === 0 ? (
        <Empty text="No history entries." />
      ) : (
        Array.from(grouped.entries()).map(([day, list]) => (
          <div key={day} className="history-day-group">
            <h4>{day}</h4>
            <ul className="history-list">
              {list.map((e) => (
                <li key={e.id}>
                  <button
                    type="button"
                    className="history-link"
                    onClick={() => onOpen(e)}
                  >
                    <strong>{e.title}</strong>
                    <code>{e.url}</code>
                    <small>{new Date(e.visitedAt).toLocaleTimeString()}</small>
                  </button>
                  <DangerButton
                    ariaLabel={`Remove history entry ${e.title}`}
                    onClick={() => onRemove(e.id)}
                  >×</DangerButton>
                </li>
              ))}
            </ul>
          </div>
        ))
      )}
    </section>
  );
}
