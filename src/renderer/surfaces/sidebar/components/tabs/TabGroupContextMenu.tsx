import type { CSSProperties, KeyboardEvent } from "react";

import type { TabGroup } from "../../../../domain/browser";
import { TAB_GROUP_COLOR_SWATCHES } from "../../../../domain/tabs/groups";
import type { MoveWorkspaceTarget } from "../../model/tabContextMenuState";

interface TabGroupContextMenuProps {
  group: TabGroup;
  left: number;
  moveWorkspaceTargets: MoveWorkspaceTarget[];
  onCloseGroup: (groupId: string) => void;
  onClose: () => void;
  onMoveToWorkspace: (groupId: string, workspaceId: string) => void;
  onToggleCollapsed: (groupId: string) => void;
  onUngroupGroup: (groupId: string) => void;
  onUpdate: (groupId: string, patch: Partial<Pick<TabGroup, "name" | "color">>) => void;
  tabCount: number;
  top: number;
}

export function TabGroupContextMenu({
  group,
  left,
  moveWorkspaceTargets,
  onClose,
  onCloseGroup,
  onMoveToWorkspace,
  onToggleCollapsed,
  onUngroupGroup,
  onUpdate,
  tabCount,
  top
}: TabGroupContextMenuProps) {
  const run = (action: () => void) => {
    action();
    onClose();
  };
  const closeOnEnter = (event: KeyboardEvent<HTMLInputElement>) => {
    if (event.key === "Enter") onClose();
  };

  return (
    <div
      className="tab-context-menu tab-group-context-menu"
      role="menu"
      style={{ left, top, "--group-color": group.color } as CSSProperties}
      onClick={(event) => event.stopPropagation()}
      onContextMenu={(event) => event.preventDefault()}
    >
      <button type="button" role="menuitem" onClick={() => run(() => onToggleCollapsed(group.id))}>
        {group.isCollapsed ? "Expand group" : "Collapse group"}
      </button>
      <label className="tab-group-menu-field">
        <span>Name</span>
        <input
          aria-label="Group name"
          value={group.name}
          onChange={(event) => onUpdate(group.id, { name: event.target.value })}
          onKeyDown={closeOnEnter}
        />
      </label>
      <div className="tab-group-menu-swatches" role="group" aria-label="Group color">
        {TAB_GROUP_COLOR_SWATCHES.map((color) => (
          <button
            key={color}
            className="tab-group-menu-swatch"
            type="button"
            title={color}
            aria-label={`Use ${color}`}
            aria-pressed={group.color.toLowerCase() === color}
            style={{ "--swatch": color } as CSSProperties}
            onClick={() => onUpdate(group.id, { color })}
          />
        ))}
      </div>
      {moveWorkspaceTargets.length > 0 && (
        <>
          <span className="tab-context-menu-separator" />
          {moveWorkspaceTargets.map((workspace) => (
            <button
              key={workspace.id}
              type="button"
              role="menuitem"
              onClick={() => run(() => onMoveToWorkspace(group.id, workspace.id))}
            >
              Move group to {workspace.name}
            </button>
          ))}
        </>
      )}
      <span className="tab-context-menu-separator" />
      <button type="button" role="menuitem" onClick={() => run(() => onCloseGroup(group.id))}>
        Close group
      </button>
      <button className="danger" type="button" role="menuitem" onClick={() => run(() => onUngroupGroup(group.id))}>
        Ungroup {tabCount} {tabCount === 1 ? "tab" : "tabs"}
      </button>
    </div>
  );
}
