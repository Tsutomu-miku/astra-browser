import { FiX } from "react-icons/fi";

import type { BrowserController } from "../../app/controller/types";
import { DownloadContextMenu } from "./downloads/components/DownloadContextMenu";
import { DownloadItem } from "./downloads/components/DownloadItem";
import { useDownloadContextMenu } from "./downloads/components/useDownloadContextMenu";

export function DownloadsPanel({ controller }: { controller: BrowserController }) {
  const { setPanel, state } = controller;
  const { closeMenu, menu, openDownloadMenu } = useDownloadContextMenu();

  return (
    <aside className="downloads-panel">
      <header className="panel-header">
        <h2>Downloads</h2>
        <button className="icon-button" title="Close downloads" type="button" onClick={() => setPanel(null)}><FiX /></button>
      </header>
      <div className="downloads-list">
        {state.downloads.length === 0
          ? <p className="empty-state">No downloads yet</p>
          : state.downloads.map((download) => (
            <DownloadItem
              download={download}
              key={download.id}
              onContextMenu={openDownloadMenu}
            />
          ))}
        {menu && (
          <DownloadContextMenu
            download={menu.item}
            left={menu.left}
            top={menu.top}
            onClose={closeMenu}
          />
        )}
      </div>
    </aside>
  );
}
