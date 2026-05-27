import { useEffect, useState, type CSSProperties, type DragEvent, type MouseEvent, type WheelEvent } from "react";
import { FiChevronLeft, FiChevronRight, FiPlus, FiTrash2 } from "react-icons/fi";

import type { Workspace } from "../../../../domain/browser";
import {
  WORKSPACE_ACCENT_SWATCHES,
  getAdjacentWorkspaceId,
  getWorkspaceButtonLabel,
  getWorkspaceInitial,
  getWorkspaceTabCount,
  getWorkspaceWheelDirection
} from "../../model/workspaceStripState";

export function WorkspaceStrip({
  activeWorkspaceId,
  draggingTabId,
  draggingWorkspaceId,
  onDragEnd,
  onDragOver,
  onDragStart,
  onDrop,
  onNewWorkspace,
  onDeleteWorkspace,
  onSelect,
  onToggleSidebar,
  onUpdateWorkspace,
  sidebarCollapsed,
  workspaces
}: {
  activeWorkspaceId: string;
  draggingTabId: string | null;
  draggingWorkspaceId: string | null;
  onDragEnd: () => void;
  onDragOver: (event: DragEvent<HTMLButtonElement>, workspaceId: string) => void;
  onDragStart: (event: DragEvent<HTMLButtonElement>, workspaceId: string) => void;
  onDrop: (event: DragEvent<HTMLButtonElement>, workspaceId: string) => void;
  onDeleteWorkspace: (workspaceId: string) => void;
  onNewWorkspace: () => void;
  onSelect: (workspaceId: string) => void;
  onToggleSidebar: () => void;
  onUpdateWorkspace: (workspaceId: string, patch: Partial<Pick<Workspace, "accent" | "name">>) => void;
  sidebarCollapsed: boolean;
  workspaces: Workspace[];
}) {
  const [menu, setMenu] = useState<{ left: number; top: number; workspaceId: string } | null>(null);
  const menuWorkspace = menu ? workspaces.find((workspace) => workspace.id === menu.workspaceId) : undefined;

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
      top: Math.min(event.clientY, window.innerHeight - 260),
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
            (draggingTabId && workspace.id !== activeWorkspaceId) ||
            (draggingWorkspaceId && workspace.id !== draggingWorkspaceId)
          )}
          onDragStart={(event) => onDragStart(event, workspace.id)}
          onDragEnd={onDragEnd}
          onDragOver={(event) => onDragOver(event, workspace.id)}
          onDrop={(event) => onDrop(event, workspace.id)}
          onContextMenu={(event) => openWorkspaceMenu(event, workspace.id)}
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
        onClick={onNewWorkspace}
      >
        <FiPlus />
      </button>
      <button
        className="workspace-button sidebar-toggle"
        title={sidebarCollapsed ? "Expand sidebar" : "Collapse sidebar"}
        type="button"
        onClick={onToggleSidebar}
      >
        {sidebarCollapsed ? <FiChevronRight /> : <FiChevronLeft />}
      </button>
      {menu && menuWorkspace && (
        <WorkspaceContextMenu
          canDelete={workspaces.length > 1}
          left={menu.left}
          top={menu.top}
          workspace={menuWorkspace}
          onClose={() => setMenu(null)}
          onDelete={onDeleteWorkspace}
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
  onSelect,
  onUpdate,
  top,
  workspace
}: {
  canDelete: boolean;
  left: number;
  onClose: () => void;
  onDelete: (workspaceId: string) => void;
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
    <div
      className="workspace-context-menu"
      role="menu"
      style={{ left, top, "--accent": workspace.accent } as CSSProperties}
      onClick={(event) => event.stopPropagation()}
      onContextMenu={(event) => event.preventDefault()}
    >
      <button type="button" role="menuitem" onClick={() => run(() => onSelect(workspace.id))}>
        Switch to Space
      </button>
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
