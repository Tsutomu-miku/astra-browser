import type { DragEvent } from "react";
import {
  FiClock,
  FiColumns,
  FiDownload,
  FiGrid,
  FiLock,
  FiMinimize2,
  FiMoon,
  FiSettings,
  FiSidebar,
  FiSquare,
  FiUnlock
} from "react-icons/fi";

import type { BrowserController } from "../../../../app/controller/types";
import type { MemorySaverState } from "../../../../common/memory/memorySaverState";

export function SidebarFooter({
  actions,
  activeTabId,
  compactMode,
  draggingTabId,
  floatingSidebarOpen,
  memorySaver,
  setPanel,
  setDraggingTabId,
  splitLayout,
  splitMode
}: {
  actions: BrowserController["actions"];
  activeTabId: string;
  compactMode: boolean;
  draggingTabId: string | null;
  floatingSidebarOpen: boolean;
  memorySaver: MemorySaverState;
  setPanel: BrowserController["setPanel"];
  setDraggingTabId: (tabId: string | null) => void;
  splitLayout: BrowserController["splitLayout"];
  splitMode: boolean;
}) {
  const sidebarToggleLabel = compactMode
    ? floatingSidebarOpen ? "Unpin floating sidebar" : "Pin floating sidebar"
    : "Focus sidebar";
  const canDropSplitTab = Boolean(draggingTabId && draggingTabId !== activeTabId);
  const memorySaverLabel = memorySaver.sleepingTabs > 0
    ? `${memorySaver.sleepingTabs} asleep`
    : `${memorySaver.reclaimableTabs} ready`;
  const memorySaverMode = memorySaver.sleepEnabled ? `Auto ${memorySaver.sleepAfterMinutes}m` : "Manual";

  function dropTabIntoSplit(event: DragEvent<HTMLButtonElement>) {
    event.preventDefault();
    const tabId = draggingTabId || event.dataTransfer.getData("text/plain");
    if (tabId && tabId !== activeTabId) actions.openTabInSplit(tabId);
    setDraggingTabId(null);
  }

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
        className="sidebar-memory-saver"
        title={`Memory Saver: ${memorySaver.summary}`}
        type="button"
        aria-label={`Memory Saver, ${memorySaver.summary}`}
        disabled={memorySaver.reclaimableTabs === 0}
        onClick={actions.sleepInactiveTabs}
      >
        <FiMoon />
        <span>{memorySaverLabel}</span>
        <small>{memorySaverMode}</small>
      </button>
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
      <button
        className="icon-button"
        title="Split view"
        type="button"
        aria-label="Split view"
        aria-pressed={splitMode}
        data-drop-target={canDropSplitTab}
        onClick={actions.toggleSplitMode}
        onDragOver={(event) => {
          if (!canDropSplitTab) return;
          event.preventDefault();
          event.dataTransfer.dropEffect = "move";
        }}
        onDrop={dropTabIntoSplit}
      >
        <FiSquare />
      </button>
      <button className="icon-button" title="History" type="button" onClick={() => setPanel("history")}><FiClock /></button>
      <button className="icon-button" title="Downloads" type="button" onClick={() => setPanel("downloads")}><FiDownload /></button>
      <button className="icon-button" title="Settings" type="button" onClick={() => setPanel("settings")}><FiSettings /></button>
    </footer>
  );
}
