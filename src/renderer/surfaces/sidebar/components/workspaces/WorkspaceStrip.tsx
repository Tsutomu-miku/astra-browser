import { Fragment, useRef, useState, type CSSProperties, type DragEvent, type MouseEvent, type WheelEvent } from "react";
import {
  FiArrowRight,
  FiChevronLeft,
  FiChevronRight,
  FiClock,
  FiColumns,
  FiDownload,
  FiGrid,
  FiLock,
  FiMinimize2,
  FiMoon,
  FiMoreHorizontal,
  FiPlus,
  FiSettings,
  FiSidebar,
  FiSquare,
  FiTrash2,
  FiUnlock
} from "react-icons/fi";

import { useContextMenuDismissal, type ContextMenuCloseOptions } from "../../../../common/context-menu/menuDismissal";
import { getAnchoredContextMenuPosition } from "../../../../common/context-menu/menuPosition";
import type { MemorySaverState } from "../../../../common/memory/memorySaverState";
import type { BrowserController } from "../../../../app/controller/types";
import type { Workspace } from "../../../../domain/browser";
import { SidebarMenuItem, SidebarMenuSeparator } from "../common/SidebarMenuItem";
import { SidebarMenuSurface } from "../common/SidebarMenuSurface";
import { readSidebarWorkspaceDragId } from "../../model/sidebarDragSources";
import { acceptSidebarRowReorderDrag, clearSidebarRowReorderDrop } from "../../model/sidebarRowReorderDrop";
import { acceptSidebarNewWorkspaceDropTarget } from "../../model/sidebarWorkspaceDropIntent";
import {
  WORKSPACE_ACCENT_SWATCHES,
  getAdjacentWorkspaceId,
  getNewWorkspaceAccessibilityLabel,
  getNewWorkspaceDropTargetState,
  getWorkspaceAccessibilityLabel,
  getWorkspaceDropTargetState,
  getWorkspaceInitial,
  getWorkspaceTabCount,
  getWorkspaceWheelDirection
} from "../../model/workspaceStripState";
import { openSidebarKeyboardContextMenu } from "../../model/sidebarKeyboardContextMenu";
import { handleWorkspaceFocusNavigation } from "../../model/workspaceFocusNavigation";

export function WorkspaceStrip({
  activeWorkspaceId,
  compactMode,
  draggingGroupId,
  draggingClosedTabIndex = null,
  draggingFavoriteId = null,
  draggingTabId,
  draggingWorkspaceId,
  floatingSidebarOpen,
  memorySaver,
  onDragEnd,
  onDragOver,
  onDragStart,
  onDrop,
  onOpenSettings,
  onNewWorkspace,
  onNewWorkspaceDrop,
  onDeleteWorkspace,
  onSelect,
  onSetSplitLayout,
  onSetPanel,
  onSleepInactiveTabs,
  onToggleCompactMode,
  onToggleSidebar,
  onToggleSplitMode,
  onUpdateWorkspace,
  sidebarCollapsed,
  splitLayout,
  splitMode,
  workspaces
}: {
  activeWorkspaceId: string;
  compactMode: boolean;
  draggingGroupId: string | null;
  draggingClosedTabIndex?: number | null;
  draggingFavoriteId?: string | null;
  draggingTabId: string | null;
  draggingWorkspaceId: string | null;
  floatingSidebarOpen: boolean;
  memorySaver: MemorySaverState;
  onDragEnd: () => void;
  onDragOver: (event: DragEvent<HTMLButtonElement>, workspaceId: string) => void;
  onDragStart: (event: DragEvent<HTMLButtonElement>, workspaceId: string) => void;
  onDrop: (event: DragEvent<HTMLButtonElement>, workspaceId: string) => void;
  onDeleteWorkspace: (workspaceId: string) => void;
  onNewWorkspace: () => void;
  onNewWorkspaceDrop: (event: DragEvent<HTMLButtonElement>) => void;
  onOpenSettings: (workspaceId: string) => void;
  onSelect: (workspaceId: string) => void;
  onSetSplitLayout: BrowserController["actions"]["setSplitLayout"];
  onSetPanel: BrowserController["setPanel"];
  onSleepInactiveTabs: BrowserController["actions"]["sleepInactiveTabs"];
  onToggleCompactMode: BrowserController["actions"]["toggleCompactMode"];
  onToggleSidebar: () => void;
  onToggleSplitMode: BrowserController["actions"]["toggleSplitMode"];
  onUpdateWorkspace: (workspaceId: string, patch: Partial<Pick<Workspace, "accent" | "name">>) => void;
  sidebarCollapsed: boolean;
  splitLayout: BrowserController["splitLayout"];
  splitMode: boolean;
  workspaces: Workspace[];
}) {
  const [workspaceMenu, setWorkspaceMenu] = useState<{ left: number; top: number; workspaceId: string } | null>(null);
  const [moreMenu, setMoreMenu] = useState<{ left: number; top: number } | null>(null);
  const menuTriggerRef = useRef<HTMLElement | null>(null);
  const moreMenuTriggerRef = useRef<HTMLElement | null>(null);
  const menuWorkspace = workspaceMenu ? workspaces.find((workspace) => workspace.id === workspaceMenu.workspaceId) : undefined;
  const sidebarToggleLabel = compactMode
    ? floatingSidebarOpen ? "Unpin floating sidebar" : "Pin floating sidebar"
    : sidebarCollapsed ? "Expand sidebar" : "Collapse sidebar";
  const moreButtonLabel = "More";
  const isNewWorkspaceDropTarget = getNewWorkspaceDropTargetState({
    draggingClosedTabIndex,
    draggingFavoriteId,
    draggingGroupId,
    draggingTabId
  });

  useContextMenuDismissal({ isOpen: Boolean(workspaceMenu), onClose: closeWorkspaceMenu });
  useContextMenuDismissal({ isOpen: Boolean(moreMenu), onClose: closeMoreMenu });

  function onWorkspaceWheel(event: WheelEvent<HTMLElement>) {
    const direction = getWorkspaceWheelDirection(event.deltaX, event.deltaY);
    if (!direction) return;

    const workspaceId = getAdjacentWorkspaceId(workspaces, activeWorkspaceId, direction);
    if (!workspaceId) return;

    event.preventDefault();
    onSelect(workspaceId);
  }

  function openWorkspaceMenu(event: MouseEvent, workspaceId: string) {
    event.preventDefault();
    closeMoreMenu({ restoreFocus: false });
    menuTriggerRef.current = event.currentTarget instanceof HTMLElement ? event.currentTarget : null;
    setWorkspaceMenu({
      ...getAnchoredContextMenuPosition(event, {
        height: 320,
        width: 216
      }),
      workspaceId
    });
  }

  function closeWorkspaceMenu({ restoreFocus = true }: ContextMenuCloseOptions = {}) {
    setWorkspaceMenu(null);
    if (restoreFocus && menuTriggerRef.current?.isConnected) {
      menuTriggerRef.current.focus();
    }
    menuTriggerRef.current = null;
  }

  function openMoreMenu(event: MouseEvent) {
    event.preventDefault();
    event.stopPropagation();
    closeWorkspaceMenu({ restoreFocus: false });
    moreMenuTriggerRef.current = event.currentTarget instanceof HTMLElement ? event.currentTarget : null;
    setMoreMenu(getAnchoredContextMenuPosition(event, {
      height: 360,
      width: 216
    }));
  }

  function closeMoreMenu({ restoreFocus = true }: ContextMenuCloseOptions = {}) {
    setMoreMenu(null);
    if (restoreFocus && moreMenuTriggerRef.current?.isConnected) {
      moreMenuTriggerRef.current.focus();
    }
  }

  return (
    <section
      className="workspace-strip"
      aria-label="Workspaces"
      onKeyDown={handleWorkspaceFocusNavigation}
      onWheel={onWorkspaceWheel}
    >
      {workspaces.map((workspace) => {
        const isActive = workspace.id === activeWorkspaceId;
        const isDropTarget = getWorkspaceDropTargetState({ activeWorkspaceId, draggingClosedTabIndex, draggingFavoriteId, draggingGroupId, draggingTabId, draggingWorkspaceId, workspaceId: workspace.id });

        return (
          <button
            className="workspace-button"
            key={workspace.id}
            style={{ "--accent": workspace.accent } as CSSProperties}
            type="button"
            title={workspace.name}
            draggable
            aria-label={getWorkspaceAccessibilityLabel(workspace, { isActive, isDropTarget })}
            aria-current={isActive}
            tabIndex={isActive ? 0 : -1}
            data-dragging={draggingWorkspaceId === workspace.id}
            data-drop-target={isDropTarget}
            onDragStart={(event) => onDragStart(event, workspace.id)}
            onDragEnd={onDragEnd}
            onDragOver={(event) => {
              onDragOver(event, workspace.id);
              acceptSidebarRowReorderDrag(event, {
                readDragId: (currentEvent) => readSidebarWorkspaceDragId({ draggingWorkspaceId }, (type) => currentEvent.dataTransfer.getData(type)),
                targetId: workspace.id
              });
            }}
            onDragLeave={clearSidebarRowReorderDrop}
            onDrop={(event) => {
              clearSidebarRowReorderDrop(event);
              onDrop(event, workspace.id);
            }}
            onContextMenu={(event) => openWorkspaceMenu(event, workspace.id)}
            onKeyDown={openSidebarKeyboardContextMenu}
            onClick={() => onSelect(workspace.id)}
          >
            <span className="workspace-initial">{getWorkspaceInitial(workspace)}</span>
            <span className="workspace-tab-count" aria-hidden="true">{getWorkspaceTabCount(workspace)}</span>
          </button>
        );
      })}
      <button
        className="workspace-button workspace-new-button"
        type="button"
        aria-label={getNewWorkspaceAccessibilityLabel(isNewWorkspaceDropTarget)}
        data-drop-target={isNewWorkspaceDropTarget}
        onDragOver={(event) => {
          acceptSidebarNewWorkspaceDropTarget(event, {
            draggingClosedTabIndex,
            draggingFavoriteId,
            draggingGroupId,
            draggingTabId
          });
        }}
        onDrop={onNewWorkspaceDrop}
        onClick={onNewWorkspace}
      >
        <FiPlus />
      </button>
      <button
        className="workspace-button sidebar-toggle"
        type="button"
        aria-label={sidebarToggleLabel}
        aria-pressed={compactMode ? floatingSidebarOpen : undefined}
        onClick={onToggleSidebar}
      >
        {compactMode ? floatingSidebarOpen ? <FiLock /> : <FiUnlock /> : sidebarCollapsed ? <FiChevronRight /> : <FiChevronLeft />}
      </button>
      <button
        className="workspace-button sidebar-more-button"
        type="button"
        aria-label={moreButtonLabel}
        aria-haspopup="menu"
        aria-expanded={Boolean(moreMenu)}
        onClick={openMoreMenu}
      >
        <FiMoreHorizontal />
      </button>
      {workspaceMenu && menuWorkspace && (
        <WorkspaceContextMenu
          canDelete={workspaces.length > 1}
          left={workspaceMenu.left}
          top={workspaceMenu.top}
          workspace={menuWorkspace}
          onClose={closeWorkspaceMenu}
          onDelete={onDeleteWorkspace}
          onNewWorkspace={onNewWorkspace}
          onOpenSettings={onOpenSettings}
          onSelect={onSelect}
          onUpdate={onUpdateWorkspace}
        />
      )}
      {moreMenu && (
        <SidebarMoreMenu
          compactMode={compactMode}
          floatingSidebarOpen={floatingSidebarOpen}
          left={moreMenu.left}
          memorySaver={memorySaver}
          sidebarCollapsed={sidebarCollapsed}
          splitLayout={splitLayout}
          splitMode={splitMode}
          top={moreMenu.top}
          onClose={closeMoreMenu}
          onSetSplitLayout={onSetSplitLayout}
          onSetPanel={onSetPanel}
          onSleepInactiveTabs={onSleepInactiveTabs}
          onToggleCompactMode={onToggleCompactMode}
          onToggleSidebar={onToggleSidebar}
          onToggleSplitMode={onToggleSplitMode}
        />
      )}
    </section>
  );
}

function WorkspaceContextMenu({
  canDelete,
  left,
  onClose,
  onDelete,
  onNewWorkspace,
  onOpenSettings,
  onSelect,
  onUpdate,
  top,
  workspace
}: {
  canDelete: boolean;
  left: number;
  onClose: () => void;
  onDelete: (workspaceId: string) => void;
  onNewWorkspace: () => void;
  onOpenSettings: (workspaceId: string) => void;
  onSelect: (workspaceId: string) => void;
  onUpdate: (workspaceId: string, patch: Partial<Pick<Workspace, "accent" | "name">>) => void;
  top: number;
  workspace: Workspace;
}) {
  const run = (action: () => void) => {
    action();
    onClose();
  };

  return (
    <SidebarMenuSurface className="workspace-context-menu" style={{ left, top, "--accent": workspace.accent } as CSSProperties}>
      <SidebarMenuItem icon={FiSettings} role="menuitem" onClick={() => run(() => onOpenSettings(workspace.id))}>
        Space settings
      </SidebarMenuItem>
      <SidebarMenuItem icon={FiArrowRight} role="menuitem" onClick={() => run(() => onSelect(workspace.id))}>
        Switch to Space
      </SidebarMenuItem>
      <SidebarMenuItem icon={FiPlus} role="menuitem" onClick={() => run(onNewWorkspace)}>
        New Space
      </SidebarMenuItem>
      <SidebarMenuSeparator />
      <label className="sidebar-menu-field">
        <span>Name</span>
        <input
          aria-label="Space name"
          value={workspace.name}
          onChange={(event) => onUpdate(workspace.id, { name: event.target.value })}
          onKeyDown={(event) => {
            if (event.key === "Enter") onClose();
          }}
        />
      </label>
      <div className="sidebar-menu-swatches" role="group" aria-label="Space accent">
        {WORKSPACE_ACCENT_SWATCHES.map((accent) => (
          <button
            key={accent}
            className="sidebar-menu-swatch"
            type="button"
            aria-label={`Use ${accent}`}
            aria-pressed={workspace.accent.toLowerCase() === accent}
            style={{ "--swatch": accent } as CSSProperties}
            onClick={() => onUpdate(workspace.id, { accent })}
          />
        ))}
      </div>
      <SidebarMenuItem
        className="danger"
        icon={FiTrash2}
        role="menuitem"
        disabled={!canDelete}
        onClick={() => run(() => onDelete(workspace.id))}
      >
        Delete Space
      </SidebarMenuItem>
    </SidebarMenuSurface>
  );
}

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

function SidebarMoreMenu({
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
