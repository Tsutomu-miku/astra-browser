import { useState, type CSSProperties } from "react";
import { FiArrowRight, FiPlus, FiSettings, FiTrash2 } from "react-icons/fi";

import type { Workspace } from "../../../../domain/browser";
import { SidebarMenuItem, SidebarMenuSeparator } from "../common/SidebarMenuItem";
import { SidebarMenuSurface } from "../common/SidebarMenuSurface";
import { WORKSPACE_ACCENT_SWATCHES } from "../../model/workspaceStripState";

export function WorkspaceContextMenu({
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
  const [nameDraft, setNameDraft] = useState(workspace.name);
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
          value={nameDraft}
          onChange={(event) => {
            setNameDraft(event.target.value);
            onUpdate(workspace.id, { name: event.target.value });
          }}
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
