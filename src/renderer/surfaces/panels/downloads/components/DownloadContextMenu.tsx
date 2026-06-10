import type { DownloadEntry } from "../../../../domain/browser";
import type { BrowserStore } from "../../../../stores/browserStoreTypes";
import { getDownloadActionsState } from "../model/downloadActions";

export function DownloadContextMenu({
  download,
  left,
  onClose,
  top,
  store
}: {
  download: DownloadEntry;
  left: number;
  onClose: () => void;
  top: number;
  store: Pick<
    BrowserStore,
    "cancelDownload" | "pauseDownload" | "resumeDownload" | "removeDownload"
  >;
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
      {actionsState.canPause && (
        <button
          type="button"
          role="menuitem"
          onClick={() => run(() => { void store.pauseDownload(download.id); })}
        >
          Pause
        </button>
      )}
      {actionsState.canResume && (
        <button
          type="button"
          role="menuitem"
          onClick={() => run(() => { void store.resumeDownload(download.id); })}
        >
          Resume
        </button>
      )}
      {actionsState.canCancel && (
        <button
          type="button"
          role="menuitem"
          onClick={() => run(() => store.cancelDownload(download.id))}
        >
          Cancel
        </button>
      )}
      {(actionsState.canPause || actionsState.canResume || actionsState.canCancel) && (
        <div className="download-context-menu-separator" />
      )}
      <button
        type="button"
        role="menuitem"
        disabled={!actionsState.canOpen}
        onClick={() => run(() => { void window.astraShell?.openPath(download.savePath); })}
      >
        Open download
      </button>
      <button
        type="button"
        role="menuitem"
        disabled={!actionsState.canShowInFolder}
        onClick={() => run(() => { void window.astraShell?.showItemInFolder(download.savePath); })}
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
      {actionsState.canRemove && (
        <>
          <div className="download-context-menu-separator" />
          <button
            type="button"
            role="menuitem"
            onClick={() => run(() => store.removeDownload(download.id))}
          >
            Remove from list
          </button>
        </>
      )}
    </div>
  );
}
