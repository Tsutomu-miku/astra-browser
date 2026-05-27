import { FiExternalLink, FiFolder, FiX } from "react-icons/fi";

import { formatBytes, type DownloadEntry } from "../../domain/browser-core";
import type { BrowserController } from "../../hooks/types";
import { getDownloadActionsState } from "./downloadActions";

export function DownloadsPanel({ controller }: { controller: BrowserController }) {
  const { setPanel, state } = controller;

  return (
    <aside className="downloads-panel">
      <header className="panel-header">
        <h2>Downloads</h2>
        <button className="icon-button" title="Close downloads" type="button" onClick={() => setPanel(null)}><FiX /></button>
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
  const actionsState = getDownloadActionsState(download);
  return (
    <article className="download-item">
      <div className="download-main">
        <span className="download-title">{download.filename}</span>
        <span className="download-meta">{getDownloadMeta(download, actionsState.progress)}</span>
      </div>
      <div className="download-actions">
        <button
          className="icon-button"
          type="button"
          title="Open download"
          disabled={!actionsState.canOpen}
          onClick={() => window.astraShell?.openPath(download.savePath)}
        >
          <FiExternalLink />
        </button>
        <button
          className="icon-button"
          type="button"
          title="Show in folder"
          disabled={!actionsState.canShowInFolder}
          onClick={() => window.astraShell?.showItemInFolder(download.savePath)}
        >
          <FiFolder />
        </button>
      </div>
      <progress className="download-progress" max="100" value={actionsState.progress} />
    </article>
  );
}

function getDownloadMeta(download: DownloadEntry, progress: number): string {
  const size = download.totalBytes ? formatBytes(download.totalBytes) : "Unknown size";
  if (download.state === "completed") return `${size} · Completed`;
  if (download.state === "interrupted") return `${size} · Interrupted`;
  if (download.state === "cancelled") return `${size} · Cancelled`;
  return `${size} · ${progress}%`;
}
