import { useEffect, useRef, useState, type CSSProperties, type DragEvent, type MouseEvent, type WheelEvent } from "react";
import { FiArrowRight, FiChevronLeft, FiChevronRight, FiLock, FiPlus, FiSettings, FiTrash2, FiUnlock } from "react-icons/fi";

import { handleMenuKeyboardNavigation, useMenuInitialFocus } from "../../../../common/context-menu/menuKeyboardNavigation";
import { getAnchoredContextMenuPosition } from "../../../../common/context-menu/menuPosition";
import { clearDropPlacement, updateDropPlacement } from "../../../../common/drag-drop/dropPlacement";
import type { Workspace } from "../../../../domain/browser";
import { SidebarMenuItem } from "../tabs/SidebarMenuItem";
import { readSidebarFavoriteDragId, readSidebarTabDragEventId } from "../../model/sidebarDragSources";
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
  onDragEnd,
  onDragOver,
  onDragStart,
  onDrop,
  onOpenSettings,
  onNewWorkspace,
  onNewWorkspaceDrop,
  onDeleteWorkspace,
  onSelect,
  onToggleSidebar,
  onUpdateWorkspace,
  sidebarCollapsed,
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
  onDragEnd: () => void;
  onDragOver: (event: DragEvent<HTMLButtonElement>, workspaceId: string) => void;
  onDragStart: (event: DragEvent<HTMLButtonElement>, workspaceId: string) => void;
  onDrop: (event: DragEvent<HTMLButtonElement>, workspaceId: string) => void;
  onDeleteWorkspace: (workspaceId: string) => void;
  onNewWorkspace: () => void;
  onNewWorkspaceDrop: (event: DragEvent<HTMLButtonElement>) => void;
  onOpenSettings: (workspaceId: string) => void;
  onSelect: (workspaceId: string) => void;
  onToggleSidebar: () => void;
  onUpdateWorkspace: (workspaceId: string, patch: Partial<Pick<Workspace, "accent" | "name">>) => void;
  sidebarCollapsed: boolean;
  workspaces: Workspace[];
}) {
  const [menu, setMenu] = useState<{ left: number; top: number; workspaceId: string } | null>(null);
  const menuTriggerRef = useRef<HTMLElement | null>(null);
  const menuWorkspace = menu ? workspaces.find((workspace) => workspace.id === menu.workspaceId) : undefined;
  const sidebarToggleLabel = compactMode
    ? floatingSidebarOpen ? "Unpin floating sidebar" : "Pin floating sidebar"
    : sidebarCollapsed ? "Expand sidebar" : "Collapse sidebar";
  const isNewWorkspaceDropTarget = getNewWorkspaceDropTargetState({
    draggingClosedTabIndex,
    draggingFavoriteId,
    draggingGroupId,
    draggingTabId
  });

  useEffect(() => {
    if (!menu) return;

    const closeOnEscape = (event: KeyboardEvent) => {
      if (event.key === "Escape") closeWorkspaceMenu();
    };
    const closeWithoutFocusRestore = () => closeWorkspaceMenu({ restoreFocus: false });

    window.addEventListener("click", closeWithoutFocusRestore);
    window.addEventListener("blur", closeWithoutFocusRestore);
    window.addEventListener("keydown", closeOnEscape);
    window.addEventListener("scroll", closeWithoutFocusRestore, true);
    return () => {
      window.removeEventListener("click", closeWithoutFocusRestore);
      window.removeEventListener("blur", closeWithoutFocusRestore);
      window.removeEventListener("keydown", closeOnEscape);
      window.removeEventListener("scroll", closeWithoutFocusRestore, true);
    };
  }, [menu]);

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
    menuTriggerRef.current = event.currentTarget instanceof HTMLElement ? event.currentTarget : null;
    setMenu({
      ...getAnchoredContextMenuPosition(event, {
        height: 320,
        width: 216
      }),
      workspaceId
    });
  }

  function closeWorkspaceMenu({ restoreFocus = true }: { restoreFocus?: boolean } = {}) {
    setMenu(null);
    if (restoreFocus && menuTriggerRef.current?.isConnected) {
      menuTriggerRef.current.focus();
    }
    menuTriggerRef.current = null;
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
              if (draggingWorkspaceId && workspace.id !== draggingWorkspaceId) {
                updateDropPlacement(event.currentTarget, event, "vertical");
              }
            }}
            onDragLeave={(event) => clearDropPlacement(event.currentTarget)}
            onDrop={(event) => {
              clearDropPlacement(event.currentTarget);
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
          if (
            draggingClosedTabIndex !== null ||
            draggingFavoriteId ||
            readSidebarFavoriteDragId({ draggingFavoriteId }, (type) => event.dataTransfer.getData(type)) ||
            draggingGroupId ||
            draggingTabId ||
            readSidebarTabDragEventId({ draggingTabId }, event.dataTransfer)
          ) {
            event.preventDefault();
            event.dataTransfer.dropEffect = "move";
          }
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
      {menu && menuWorkspace && (
        <WorkspaceContextMenu
          canDelete={workspaces.length > 1}
          left={menu.left}
          top={menu.top}
          workspace={menuWorkspace}
          onClose={closeWorkspaceMenu}
          onDelete={onDeleteWorkspace}
          onNewWorkspace={onNewWorkspace}
          onOpenSettings={onOpenSettings}
          onSelect={onSelect}
          onUpdate={onUpdateWorkspace}
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
  const menuRef = useRef<HTMLDivElement | null>(null);
  useMenuInitialFocus(menuRef);
  const run = (action: () => void) => {
    action();
    onClose();
  };

  return (
    <div
      ref={menuRef}
      className="workspace-context-menu"
      role="menu"
      style={{ left, top, "--accent": workspace.accent } as CSSProperties}
      onClick={(event) => event.stopPropagation()}
      onContextMenu={(event) => event.preventDefault()}
      onKeyDown={handleMenuKeyboardNavigation}
    >
      <SidebarMenuItem icon={FiSettings} role="menuitem" onClick={() => run(() => onOpenSettings(workspace.id))}>
        Space settings
      </SidebarMenuItem>
      <SidebarMenuItem icon={FiArrowRight} role="menuitem" onClick={() => run(() => onSelect(workspace.id))}>
        Switch to Space
      </SidebarMenuItem>
      <SidebarMenuItem icon={FiPlus} role="menuitem" onClick={() => run(onNewWorkspace)}>
        New Space
      </SidebarMenuItem>
      <span className="tab-context-menu-separator" />
      <label className="workspace-menu-field">
        <span>Name</span>
        <input
          value={workspace.name}
          onChange={(event) => onUpdate(workspace.id, { name: event.target.value })}
          onKeyDown={(event) => {
            if (event.key === "Enter") onClose();
          }}
        />
      </label>
      <div className="workspace-menu-swatches" role="group" aria-label="Space accent">
        {WORKSPACE_ACCENT_SWATCHES.map((accent) => (
          <button
            key={accent}
            className="workspace-menu-swatch"
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
    </div>
  );
}
