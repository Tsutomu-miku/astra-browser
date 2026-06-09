import { type CSSProperties } from "react";
import {
  FiChevronLeft,
  FiClock,
  FiColumns,
  FiDownload,
  FiGrid,
  FiLock,
  FiMinimize2,
  FiMoon,
  FiSidebar,
  FiSquare,
  FiSettings,
  FiUnlock
} from "react-icons/fi";

import type { MemorySaverState } from "../../../../common/memory/memorySaverState";
import type { BrowserController } from "../../../../app/controller/types";
import { SidebarMenuItem, SidebarMenuSeparator } from "../common/SidebarMenuItem";
import { SidebarMenuSurface } from "../common/SidebarMenuSurface";

interface SidebarMoreMenuProps {
  compactMode: boolean;
  floatingSidebarOpen: boolean;
  left: number;
  memorySaver: MemorySaverState;
  sidebarCollapsed: boolean;
  splitLayout: BrowserController["splitLayout"];
  splitMode: boolean;
  top: number;
  onClose: () => void;
  onSetSplitLayout: BrowserController["actions"]["setSplitLayout"];
  onSetPanel: BrowserController["setPanel"];
  onSleepInactiveTabs: BrowserController["actions"]["sleepInactiveTabs"];
  onToggleCompactMode: BrowserController["actions"]["toggleCompactMode"];
  onToggleSidebar: () => void;
  onToggleSplitMode: BrowserController["actions"]["toggleSplitMode"];
}

export function SidebarMoreMenu({
  compactMode,
  floatingSidebarOpen,
  left,
  memorySaver,
  sidebarCollapsed,
  splitLayout,
  splitMode,
  top,
  onClose,
  onSetSplitLayout,
  onSetPanel,
  onSleepInactiveTabs,
  onToggleCompactMode,
  onToggleSidebar,
  onToggleSplitMode
}: SidebarMoreMenuProps) {
  const run = (action: () => void) => {
    action();
    onClose();
  };
  const sidebarToggleLabel = compactMode
    ? floatingSidebarOpen ? "Unpin floating sidebar" : "Pin floating sidebar"
    : sidebarCollapsed ? "Expand sidebar" : "Collapse sidebar";
  const memorySaverLabel = memorySaver.sleepEnabled
    ? `Sleep inactive tabs (auto)`
    : `Sleep inactive tabs (manual)`;
  const memorySaverHint = `${memorySaver.reclaimableTabs} tabs ready`;

  return (
    <SidebarMenuSurface className="sidebar-more-menu" style={{ left, top } as CSSProperties}>
      <SidebarMenuItem
        icon={FiMoon}
        role="menuitem"
        disabled={memorySaver.reclaimableTabs === 0}
        onClick={() => run(onSleepInactiveTabs)}
      >
        <span className="sidebar-menu-item-main">
          <span>{memorySaverLabel}</span>
          <small className="sidebar-menu-item-hint">{memorySaverHint}</small>
        </span>
      </SidebarMenuItem>
      <SidebarMenuSeparator />
      <SidebarMenuItem
        aria-pressed={splitMode}
        icon={FiSquare}
        role="menuitem"
        onClick={() => run(onToggleSplitMode)}
      >
        Split view
      </SidebarMenuItem>
      {splitMode && (
        <>
          <SidebarMenuItem
            aria-pressed={splitLayout === "horizontal"}
            icon={FiColumns}
            role="menuitem"
            onClick={() => run(() => onSetSplitLayout("horizontal"))}
          >
            Horizontal split
          </SidebarMenuItem>
          <SidebarMenuItem
            aria-pressed={splitLayout === "vertical"}
            icon={FiSidebar}
            role="menuitem"
            onClick={() => run(() => onSetSplitLayout("vertical"))}
          >
            Vertical split
          </SidebarMenuItem>
          <SidebarMenuItem
            aria-pressed={splitLayout === "grid"}
            icon={FiGrid}
            role="menuitem"
            onClick={() => run(() => onSetSplitLayout("grid"))}
          >
            Grid split
          </SidebarMenuItem>
          <SidebarMenuSeparator />
        </>
      )}
      <SidebarMenuItem
        aria-pressed={compactMode}
        icon={FiMinimize2}
        role="menuitem"
        onClick={() => run(onToggleCompactMode)}
      >
        Compact mode
      </SidebarMenuItem>
      <SidebarMenuItem
        aria-pressed={compactMode ? floatingSidebarOpen : undefined}
        icon={compactMode ? (floatingSidebarOpen ? FiUnlock : FiLock) : FiChevronLeft}
        role="menuitem"
        onClick={() => run(onToggleSidebar)}
      >
        {sidebarToggleLabel}
      </SidebarMenuItem>
      <SidebarMenuSeparator />
      <SidebarMenuItem icon={FiClock} role="menuitem" onClick={() => run(() => onSetPanel("history"))}>
        History
      </SidebarMenuItem>
      <SidebarMenuItem icon={FiDownload} role="menuitem" onClick={() => run(() => onSetPanel("downloads"))}>
        Downloads
      </SidebarMenuItem>
      <SidebarMenuItem icon={FiSettings} role="menuitem" onClick={() => run(() => onSetPanel("settings"))}>
        Settings
      </SidebarMenuItem>
    </SidebarMenuSurface>
  );
}
