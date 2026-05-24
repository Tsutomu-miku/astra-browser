import { formatBytes, type DownloadEntry } from "../../domain/browser-core";
import type { BrowserController } from "../../hooks/types";

export function DownloadsPanel({ controller }: { controller: BrowserController }) {
  const { setPanel, state } = controller;

  return (
    <aside className="downloads-panel">
      <header className="panel-header">
        <h2>Downloads</h2>
        <button className="icon-button" title="Close downloads" type="button" onClick={() => setPanel(null)}>×</button>
      </header>
      <div className="downloads-list">
        {state.downloads.length === 0
          ? <p className="empty-state">No downloads yet</p>
          : state.downloads.map((download) => <DownloadItem key={download.id} download={download} />)}
      </div>
    </aside>
  );
}

function DownloadItem({ download }: { download: DownloadEntry }) {
  const progress = getDownloadProgress(download);
  return (
    <article className="download-item">
      <div className="download-main">
        <span className="download-title">{download.filename}</span>
        <span className="download-meta">{getDownloadMeta(download, progress)}</span>
      </div>
      <progress className="download-progress" max="100" value={progress} />
      <button
        className="toolbar-button download-action"
        type="button"
        disabled={download.state !== "completed" || !download.savePath}
        onClick={() => window.astraShell?.showItemInFolder(download.savePath)}
      >
        Show
      </button>
    </article>
  );
}

function getDownloadProgress(download: DownloadEntry): number {
  if (!download.totalBytes) return download.state === "completed" ? 100 : 0;
  return Math.round((download.receivedBytes / download.totalBytes) * 100);
}

function getDownloadMeta(download: DownloadEntry, progress: number): string {
  const size = download.totalBytes ? formatBytes(download.totalBytes) : "Unknown size";
  if (download.state === "completed") return `${size} · Completed`;
  if (download.state === "interrupted") return `${size} · Interrupted`;
  if (download.state === "cancelled") return `${size} · Cancelled`;
  return `${size} · ${progress}%`;
}
