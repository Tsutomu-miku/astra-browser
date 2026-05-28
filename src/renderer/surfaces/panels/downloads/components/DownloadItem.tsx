import type { MouseEvent } from "react";
import { FiExternalLink, FiFolder } from "react-icons/fi";

import type { DownloadEntry } from "../../../../domain/browser";
import { getDownloadActionsState } from "../model/downloadActions";
import { getDownloadMeta } from "../model/downloadMeta";

export function DownloadItem({
  download,
  onContextMenu
}: {
  download: DownloadEntry;
  onContextMenu: (event: MouseEvent, download: DownloadEntry) => void;
}) {
  const actionsState = getDownloadActionsState(download);

  return (
    <article className="download-item" onContextMenu={(event) => onContextMenu(event, download)}>
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
