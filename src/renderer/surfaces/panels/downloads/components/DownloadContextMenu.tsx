import type { DownloadEntry } from "../../../../domain/browser";
import { getDownloadActionsState } from "../model/downloadActions";

export function DownloadContextMenu({
  download,
  left,
  onClose,
  top
}: {
  download: DownloadEntry;
  left: number;
  onClose: () => void;
  top: number;
}) {
  const actionsState = getDownloadActionsState(download);
  const run = (action: () => void) => {
    action();
    onClose();
  };

  return (
    <div
      className="download-context-menu"
      role="menu"
      style={{ left, top }}
      onContextMenu={(event) => event.preventDefault()}
    >
      <button
        type="button"
        role="menuitem"
        disabled={!actionsState.canOpen}
        onClick={() => run(() => window.astraShell?.openPath(download.savePath))}
      >
        Open download
      </button>
      <button
        type="button"
        role="menuitem"
        disabled={!actionsState.canShowInFolder}
        onClick={() => run(() => window.astraShell?.showItemInFolder(download.savePath))}
      >
        Show in folder
      </button>
      <button
        type="button"
        role="menuitem"
        disabled={!download.savePath}
        onClick={() => run(() => void navigator.clipboard?.writeText(download.savePath))}
      >
        Copy file path
      </button>
    </div>
  );
}
