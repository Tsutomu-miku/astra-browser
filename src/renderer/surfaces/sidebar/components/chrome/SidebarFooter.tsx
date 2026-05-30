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
import type { ClosedTab, Favorite, BrowserTab } from "../../../../domain/browser";
import { handleSidebarFooterFocusNavigation } from "../../model/sidebarFooterFocusNavigation";
import {
  acceptSidebarSplitDropTarget,
  getSidebarSplitDropSource,
  resolveSidebarSplitDrop
} from "../../model/sidebarSplitDropTarget";

export function SidebarFooter({
  actions,
  activeTabId,
  closedTabs,
  compactMode,
  draggingClosedTabIndex,
  draggingEssentialId,
  draggingFavoriteId,
  draggingTabId,
  essentials,
  favorites,
  floatingSidebarOpen,
  memorySaver,
  setPanel,
  setDraggingClosedTabIndex,
  setDraggingEssentialId,
  setDraggingFavoriteId,
  setDraggingTabId,
  splitLayout,
  splitMode,
  tabs
}: {
  actions: BrowserController["actions"];
  activeTabId: string;
  closedTabs: ClosedTab[];
  compactMode: boolean;
  draggingClosedTabIndex: number | null;
  draggingEssentialId: string | null;
  draggingFavoriteId: string | null;
  draggingTabId: string | null;
  essentials: Favorite[];
  favorites: Favorite[];
  floatingSidebarOpen: boolean;
  memorySaver: MemorySaverState;
  setPanel: BrowserController["setPanel"];
  setDraggingClosedTabIndex: (closedTabIndex: number | null) => void;
  setDraggingEssentialId: (essentialId: string | null) => void;
  setDraggingFavoriteId: (favoriteId: string | null) => void;
  setDraggingTabId: (tabId: string | null) => void;
  splitLayout: BrowserController["splitLayout"];
  splitMode: boolean;
  tabs: BrowserTab[];
}) {
  const sidebarToggleLabel = compactMode
    ? floatingSidebarOpen ? "Unpin floating sidebar" : "Pin floating sidebar"
    : "Focus sidebar";
  const splitDropState = {
    activeTabId,
    closedTabs,
    draggingClosedTabIndex,
    draggingEssentialId,
    draggingFavoriteId,
    draggingTabId,
    essentials,
    favorites,
    tabs
  };
  const splitDropSource = getSidebarSplitDropSource(splitDropState);
  const showSplitDropTarget = Boolean(splitDropSource);
  const splitButtonLabel = splitDropSource
    ? `Split view, drop ${splitDropSource.title || "item"} here`
    : "Split view";
  const memorySaverLabel = `${memorySaver.reclaimableTabs} ready`;
  const memorySaverMode = memorySaver.sleepEnabled ? `Auto ${memorySaver.sleepAfterMinutes}m` : "Manual";

  function dropTabIntoSplit(event: DragEvent<HTMLButtonElement>) {
    const source = resolveSidebarSplitDrop(event, splitDropState);
    if (!source) return;

    if (source.type === "tab") {
      actions.openTabInSplit(source.tabId);
    } else {
      actions.openUrlInSplit(source.url, source.title);
    }
    setDraggingClosedTabIndex(null);
    setDraggingEssentialId(null);
    setDraggingFavoriteId(null);
    setDraggingTabId(null);
  }

  return (
    <footer className="sidebar-footer" onKeyDown={handleSidebarFooterFocusNavigation}>
      {splitMode && (
        <div className="sidebar-split-layout" aria-label="Split layout">
          <button
            type="button"
            aria-label="Horizontal split layout"
            aria-pressed={splitLayout === "horizontal"}
            onClick={() => actions.setSplitLayout("horizontal")}
          >
            <FiColumns />
          </button>
          <button
            type="button"
            aria-label="Vertical split layout"
            aria-pressed={splitLayout === "vertical"}
            onClick={() => actions.setSplitLayout("vertical")}
          >
            <FiSidebar />
          </button>
          <button
            type="button"
            aria-label="Grid split layout"
            aria-pressed={splitLayout === "grid"}
            onClick={() => actions.setSplitLayout("grid")}
          >
            <FiGrid />
          </button>
        </div>
      )}
      <button
        className="sidebar-memory-saver"
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
        type="button"
        aria-label={sidebarToggleLabel}
        aria-pressed={compactMode ? floatingSidebarOpen : undefined}
        onClick={actions.toggleSidebar}
      >
        {compactMode ? floatingSidebarOpen ? <FiLock /> : <FiUnlock /> : <FiSidebar />}
      </button>
      <button
        className="icon-button"
        type="button"
        aria-label="Compact mode"
        aria-pressed={compactMode}
        onClick={actions.toggleCompactMode}
      >
        <FiMinimize2 />
      </button>
      <button
        className="icon-button"
        type="button"
        aria-label={splitButtonLabel}
        aria-pressed={splitMode}
        data-drop-target={showSplitDropTarget}
        onClick={actions.toggleSplitMode}
        onDragOver={(event) => acceptSidebarSplitDropTarget(event, splitDropState)}
        onDrop={dropTabIntoSplit}
      >
        <FiSquare />
      </button>
      <button className="icon-button" type="button" aria-label="History" onClick={() => setPanel("history")}><FiClock /></button>
      <button className="icon-button" type="button" aria-label="Downloads" onClick={() => setPanel("downloads")}><FiDownload /></button>
      <button className="icon-button" type="button" aria-label="Settings" onClick={() => setPanel("settings")}><FiSettings /></button>
    </footer>
  );
}
