import { FiX } from "react-icons/fi";

import type { BrowserController } from "../../app/controller/types";
import { DownloadItem } from "./components/DownloadItem";

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
