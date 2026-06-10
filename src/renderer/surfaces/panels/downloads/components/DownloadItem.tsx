import type { MouseEvent } from "react";
import {
  FiExternalLink,
  FiFolder,
  FiPause,
  FiPlay,
  FiRefreshCw,
  FiTrash2,
  FiXCircle
} from "react-icons/fi";

import type { DownloadEntry } from "../../../../domain/browser";
import type { BrowserStore } from "../../../../stores/browserStoreTypes";
import { getDownloadActionsState } from "../model/downloadActions";
import { getDownloadMeta } from "../model/downloadMeta";

export interface DownloadItemStoreSubset {
  cancelDownload: BrowserStore["cancelDownload"];
  pauseDownload: BrowserStore["pauseDownload"];
  resumeDownload: BrowserStore["resumeDownload"];
  removeDownload: BrowserStore["removeDownload"];
  retryDownload?: (url: string) => void;
}

export function DownloadItem({
  download,
  onContextMenu,
  store
}: {
  download: DownloadEntry;
  onContextMenu: (event: MouseEvent, download: DownloadEntry) => void;
  store: DownloadItemStoreSubset;
}) {
  const actionsState = getDownloadActionsState(download);

  return (
    <article
      className={`download-item ${actionsState.isPaused ? "is-paused" : ""} ${actionsState.isTerminal ? "is-terminal" : ""}`}
      onContextMenu={(event) => onContextMenu(event, download)}
    >
      <div className="download-main">
        <span className="download-title" title={download.url || download.filename}>
          {download.filename}
        </span>
        <span className="download-meta">{getDownloadMeta(download, actionsState.progress)}</span>
      </div>
      <div className="download-actions">
        {actionsState.canPause && (
          <button
            className="icon-button"
            type="button"
            title="Pause download"
            onClick={() => { void store.pauseDownload(download.id); }}
          >
            <FiPause />
          </button>
        )}
        {actionsState.canResume && (
          <button
            className="icon-button"
            type="button"
            title="Resume download"
            onClick={() => { void store.resumeDownload(download.id); }}
          >
            <FiPlay />
          </button>
        )}
        {actionsState.canCancel && (
          <button
            className="icon-button"
            type="button"
            title="Cancel download"
            onClick={() => store.cancelDownload(download.id)}
          >
            <FiXCircle />
          </button>
        )}
        <button
          className="icon-button"
          type="button"
          title="Open download"
          disabled={!actionsState.canOpen}
          onClick={() => { void window.astraShell?.openPath(download.savePath); }}
        >
          <FiExternalLink />
        </button>
        <button
          className="icon-button"
          type="button"
          title="Show in folder"
          disabled={!actionsState.canShowInFolder}
          onClick={() => { void window.astraShell?.showItemInFolder(download.savePath); }}
        >
          <FiFolder />
        </button>
        {actionsState.canRetry && (
          <button
            className="icon-button"
            type="button"
            title="Retry download"
            disabled={!download.url}
            onClick={() => {
              if (download.url && store.retryDownload) store.retryDownload(download.url);
            }}
          >
            <FiRefreshCw />
          </button>
        )}
        {actionsState.canRemove && (
          <button
            className="icon-button"
            type="button"
            title="Remove from list"
            onClick={() => store.removeDownload(download.id)}
          >
            <FiTrash2 />
          </button>
        )}
      </div>
      <progress className="download-progress" max="100" value={actionsState.progress} />
    </article>
  );
}
