import { FiExternalLink, FiFolder } from "react-icons/fi";

import { formatBytes, type DownloadEntry } from "../../../domain/browser";
import { getDownloadActionsState } from "../model/downloadActions";

export function DownloadItem({ download }: { download: DownloadEntry }) {
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
