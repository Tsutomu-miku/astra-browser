import type { CSSProperties, DragEvent } from "react";
import { FiChevronLeft, FiChevronRight, FiPlus } from "react-icons/fi";

import type { Workspace } from "../../../domain/browser-core";
import {
  getWorkspaceButtonLabel,
  getWorkspaceInitial,
  getWorkspaceTabCount
} from "../model/workspaceStripState";

export function WorkspaceStrip({
  activeWorkspaceId,
  draggingTabId,
  draggingWorkspaceId,
  onDragEnd,
  onDragOver,
  onDragStart,
  onDrop,
  onNewWorkspace,
  onSelect,
  onToggleSidebar,
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
  onNewWorkspace: () => void;
  onSelect: (workspaceId: string) => void;
  onToggleSidebar: () => void;
  sidebarCollapsed: boolean;
  workspaces: Workspace[];
}) {
  return (
    <section className="workspace-strip" aria-label="Workspaces">
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
    </section>
  );
}
