import { FiTrash2, FiX } from "react-icons/fi";

import type { BrowserController } from "../../app/controller/types";
import type { BrowserStore } from "../../stores/browserStoreTypes";
import { DownloadContextMenu } from "./downloads/components/DownloadContextMenu";
import { DownloadItem } from "./downloads/components/DownloadItem";
import { useDownloadContextMenu } from "./downloads/components/useDownloadContextMenu";

export type DownloadsPanelStore = Pick<
  BrowserStore,
  "cancelDownload" | "pauseDownload" | "resumeDownload" | "removeDownload" | "clearAllDownloads" | "retryDownload"
>;

export function DownloadsPanel({
  controller,
  store
}: {
  controller: BrowserController;
  store: DownloadsPanelStore;
}) {
  const { setPanel, state } = controller;
  const { closeMenu, menu, openDownloadMenu } = useDownloadContextMenu();

  return (
    <aside className="downloads-panel">
      <header className="panel-header">
        <h2>Downloads</h2>
        <div className="panel-header-actions">
          {state.downloads.length > 0 && (
            <button
              className="icon-button"
              title="Clear finished downloads"
              type="button"
              onClick={() => store.clearAllDownloads()}
            >
              <FiTrash2 />
            </button>
          )}
          <button className="icon-button" title="Close downloads" type="button" onClick={() => setPanel(null)}><FiX /></button>
        </div>
      </header>
      <div className="downloads-list">
        {state.downloads.length === 0
          ? <p className="empty-state">No downloads yet</p>
          : state.downloads.map((download) => (
            <DownloadItem
              download={download}
              key={download.id}
              onContextMenu={openDownloadMenu}
              store={store}
            />
          ))}
        {menu && (
          <DownloadContextMenu
            download={menu.item}
            left={menu.left}
            top={menu.top}
            onClose={closeMenu}
            store={store}
          />
        )}
      </div>
    </aside>
  );
}
