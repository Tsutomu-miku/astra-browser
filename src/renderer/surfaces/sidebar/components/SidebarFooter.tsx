import {
  FiClock,
  FiColumns,
  FiDownload,
  FiMinimize2,
  FiSettings,
  FiSidebar
} from "react-icons/fi";

import type { BrowserController } from "../../../app/controller/types";

export function SidebarFooter({
  actions,
  compactMode,
  setPanel,
  splitMode
}: {
  actions: BrowserController["actions"];
  compactMode: boolean;
  setPanel: BrowserController["setPanel"];
  splitMode: boolean;
}) {
  return (
    <footer className="sidebar-footer">
      <button className="icon-button" title="Focus sidebar" type="button" onClick={actions.toggleSidebar}><FiSidebar /></button>
      <button className="icon-button" title="Compact mode" type="button" aria-pressed={compactMode} onClick={actions.toggleCompactMode}><FiMinimize2 /></button>
      <button className="icon-button" title="Split view" type="button" aria-pressed={splitMode} onClick={actions.toggleSplitMode}><FiColumns /></button>
      <button className="icon-button" title="History" type="button" onClick={() => setPanel("history")}><FiClock /></button>
      <button className="icon-button" title="Downloads" type="button" onClick={() => setPanel("downloads")}><FiDownload /></button>
      <button className="icon-button" title="Settings" type="button" onClick={() => setPanel("settings")}><FiSettings /></button>
    </footer>
  );
}
