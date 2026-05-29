import { useEffect, useRef, useState, type CSSProperties, type DragEvent, type MouseEvent, type WheelEvent } from "react";
import { FiChevronLeft, FiChevronRight, FiLock, FiPlus, FiTrash2, FiUnlock } from "react-icons/fi";

import { handleMenuKeyboardNavigation, useMenuInitialFocus } from "../../../../common/context-menu/menuKeyboardNavigation";
import type { Workspace } from "../../../../domain/browser";
import {
  WORKSPACE_ACCENT_SWATCHES,
  getAdjacentWorkspaceId,
  getWorkspaceButtonLabel,
  getWorkspaceInitial,
  getWorkspaceTabCount,
  getWorkspaceWheelDirection
} from "../../model/workspaceStripState";
import { openSidebarKeyboardContextMenu } from "../../model/sidebarKeyboardContextMenu";

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
  const menuWorkspace = menu ? workspaces.find((workspace) => workspace.id === menu.workspaceId) : undefined;
  const sidebarToggleLabel = compactMode
    ? floatingSidebarOpen ? "Unpin floating sidebar" : "Pin floating sidebar"
    : sidebarCollapsed ? "Expand sidebar" : "Collapse sidebar";

  useEffect(() => {
    if (!menu) return;

    const close = () => setMenu(null);
    const closeOnEscape = (event: KeyboardEvent) => {
      if (event.key === "Escape") close();
    };

    window.addEventListener("click", close);
    window.addEventListener("blur", close);
    window.addEventListener("keydown", closeOnEscape);
    window.addEventListener("scroll", close, true);
    return () => {
      window.removeEventListener("click", close);
      window.removeEventListener("blur", close);
      window.removeEventListener("keydown", closeOnEscape);
      window.removeEventListener("scroll", close, true);
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
    setMenu({
      left: Math.min(event.clientX, window.innerWidth - 216),
      top: Math.min(event.clientY, window.innerHeight - 320),
      workspaceId
    });
  }

  return (
    <section className="workspace-strip" aria-label="Workspaces" onWheel={onWorkspaceWheel}>
      {workspaces.map((workspace) => (
        <button
          className="workspace-button"
          key={workspace.id}
          style={{ "--accent": workspace.accent } as CSSProperties}
          title={getWorkspaceButtonLabel(workspace)}
          type="button"
          draggable
          aria-label={getWorkspaceButtonLabel(workspace)}
          aria-current={workspace.id === activeWorkspaceId}
          data-dragging={draggingWorkspaceId === workspace.id}
          data-drop-target={Boolean(
            (draggingGroupId && workspace.id !== activeWorkspaceId) ||
            draggingClosedTabIndex !== null ||
            (draggingFavoriteId && workspace.id !== activeWorkspaceId) ||
            (draggingTabId && workspace.id !== activeWorkspaceId) ||
            (draggingWorkspaceId && workspace.id !== draggingWorkspaceId)
          )}
          onDragStart={(event) => onDragStart(event, workspace.id)}
          onDragEnd={onDragEnd}
          onDragOver={(event) => onDragOver(event, workspace.id)}
          onDrop={(event) => onDrop(event, workspace.id)}
          onContextMenu={(event) => openWorkspaceMenu(event, workspace.id)}
          onKeyDown={openSidebarKeyboardContextMenu}
          onClick={() => onSelect(workspace.id)}
        >
          <span className="workspace-initial">{getWorkspaceInitial(workspace)}</span>
          <span className="workspace-tab-count" aria-hidden="true">{getWorkspaceTabCount(workspace)}</span>
        </button>
      ))}
      <button
        className="workspace-button workspace-new-button"
        title="New Space"
        type="button"
        aria-label="New Space"
        data-drop-target={Boolean(draggingClosedTabIndex !== null || draggingFavoriteId || draggingGroupId || draggingTabId)}
        onDragOver={(event) => {
          if (draggingClosedTabIndex !== null || draggingFavoriteId || draggingGroupId || draggingTabId) {
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
        title={sidebarToggleLabel}
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
          onClose={() => setMenu(null)}
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
      <button type="button" role="menuitem" onClick={() => run(() => onOpenSettings(workspace.id))}>
        Space settings
      </button>
      <button type="button" role="menuitem" onClick={() => run(() => onSelect(workspace.id))}>
        Switch to Space
      </button>
      <button type="button" role="menuitem" onClick={() => run(onNewWorkspace)}>
        New Space
      </button>
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
            title={accent}
            aria-label={`Use ${accent}`}
            aria-pressed={workspace.accent.toLowerCase() === accent}
            style={{ "--swatch": accent } as CSSProperties}
            onClick={() => onUpdate(workspace.id, { accent })}
          />
        ))}
      </div>
      <button
        className="danger"
        type="button"
        role="menuitem"
        disabled={!canDelete}
        onClick={() => run(() => onDelete(workspace.id))}
      >
        <FiTrash2 />
        Delete Space
      </button>
    </div>
  );
}
