import {
  FiClock,
  FiColumns,
  FiDownload,
  FiGrid,
  FiLock,
  FiMinimize2,
  FiSettings,
  FiSidebar,
  FiSquare,
  FiUnlock
} from "react-icons/fi";

import type { BrowserController } from "../../../../app/controller/types";

export function SidebarFooter({
  actions,
  compactMode,
  floatingSidebarOpen,
  setPanel,
  splitLayout,
  splitMode
}: {
  actions: BrowserController["actions"];
  compactMode: boolean;
  floatingSidebarOpen: boolean;
  setPanel: BrowserController["setPanel"];
  splitLayout: BrowserController["splitLayout"];
  splitMode: boolean;
}) {
  const sidebarToggleLabel = compactMode
    ? floatingSidebarOpen ? "Unpin floating sidebar" : "Pin floating sidebar"
    : "Focus sidebar";

  return (
    <footer className="sidebar-footer">
      {splitMode && (
        <div className="sidebar-split-layout" aria-label="Split layout">
          <button
            type="button"
            title="Horizontal split layout"
            aria-pressed={splitLayout === "horizontal"}
            onClick={() => actions.setSplitLayout("horizontal")}
          >
            <FiColumns />
          </button>
          <button
            type="button"
            title="Vertical split layout"
            aria-pressed={splitLayout === "vertical"}
            onClick={() => actions.setSplitLayout("vertical")}
          >
            <FiSidebar />
          </button>
          <button
            type="button"
            title="Grid split layout"
            aria-pressed={splitLayout === "grid"}
            onClick={() => actions.setSplitLayout("grid")}
          >
            <FiGrid />
          </button>
        </div>
      )}
      <button
        className="icon-button"
        title={sidebarToggleLabel}
        type="button"
        aria-label={sidebarToggleLabel}
        aria-pressed={compactMode ? floatingSidebarOpen : undefined}
        onClick={actions.toggleSidebar}
      >
        {compactMode ? floatingSidebarOpen ? <FiLock /> : <FiUnlock /> : <FiSidebar />}
      </button>
      <button className="icon-button" title="Compact mode" type="button" aria-pressed={compactMode} onClick={actions.toggleCompactMode}><FiMinimize2 /></button>
      <button className="icon-button" title="Split view" type="button" aria-pressed={splitMode} onClick={actions.toggleSplitMode}><FiSquare /></button>
      <button className="icon-button" title="History" type="button" onClick={() => setPanel("history")}><FiClock /></button>
      <button className="icon-button" title="Downloads" type="button" onClick={() => setPanel("downloads")}><FiDownload /></button>
      <button className="icon-button" title="Settings" type="button" onClick={() => setPanel("settings")}><FiSettings /></button>
    </footer>
  );
}
