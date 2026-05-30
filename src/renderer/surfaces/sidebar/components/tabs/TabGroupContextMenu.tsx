import { type CSSProperties, type KeyboardEvent } from "react";
import { FiArchive, FiChevronDown, FiChevronRight, FiCopy, FiGrid, FiMoon, FiPlus, FiX } from "react-icons/fi";

import type { TabGroup } from "../../../../domain/browser";
import { TAB_GROUP_COLOR_SWATCHES } from "../../../../domain/tabs/groups";
import { SidebarMenuItem, SidebarMenuSeparator } from "../common/SidebarMenuItem";
import { SidebarMenuSurface } from "../common/SidebarMenuSurface";
import type { MoveWorkspaceTarget } from "../../model/tabContextMenuState";

interface TabGroupContextMenuProps {
  group: TabGroup;
  canSleepGroup: boolean;
  left: number;
  moveWorkspaceTargets: MoveWorkspaceTarget[];
  onCloseGroup: (groupId: string) => void;
  onClose: () => void;
  onDuplicateGroup: (groupId: string) => void;
  onMoveToNewWorkspace: (groupId: string) => void;
  onMoveToWorkspace: (groupId: string, workspaceId: string) => void;
  onSleepGroup: (groupId: string) => void;
  onToggleCollapsed: (groupId: string) => void;
  onUngroupGroup: (groupId: string) => void;
  onUpdate: (groupId: string, patch: Partial<Pick<TabGroup, "name" | "color">>) => void;
  tabCount: number;
  top: number;
}

export function TabGroupContextMenu({
  group,
  canSleepGroup,
  left,
  moveWorkspaceTargets,
  onClose,
  onCloseGroup,
  onDuplicateGroup,
  onMoveToNewWorkspace,
  onMoveToWorkspace,
  onSleepGroup,
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
    <SidebarMenuSurface className="tab-context-menu tab-group-context-menu" style={{ left, top, "--group-color": group.color } as CSSProperties}>
      <SidebarMenuItem icon={group.isCollapsed ? FiChevronRight : FiChevronDown} role="menuitem" onClick={() => run(() => onToggleCollapsed(group.id))}>
        {group.isCollapsed ? "Expand group" : "Collapse group"}
      </SidebarMenuItem>
      <SidebarMenuItem icon={FiMoon} role="menuitem" disabled={!canSleepGroup} onClick={() => run(() => onSleepGroup(group.id))}>
        Sleep group
      </SidebarMenuItem>
      <SidebarMenuItem icon={FiCopy} role="menuitem" onClick={() => run(() => onDuplicateGroup(group.id))}>
        Duplicate group
      </SidebarMenuItem>
      <label className="sidebar-menu-field">
        <span>Name</span>
        <input
          aria-label="Group name"
          value={group.name}
          onChange={(event) => onUpdate(group.id, { name: event.target.value })}
          onKeyDown={closeOnEnter}
        />
      </label>
      <div className="sidebar-menu-swatches" role="group" aria-label="Group color">
        {TAB_GROUP_COLOR_SWATCHES.map((color) => (
          <button
            key={color}
            className="sidebar-menu-swatch"
            type="button"
            aria-label={`Use ${color}`}
            aria-pressed={group.color.toLowerCase() === color}
            style={{ "--swatch": color } as CSSProperties}
            onClick={() => onUpdate(group.id, { color })}
          />
        ))}
      </div>
      <SidebarMenuSeparator />
      {moveWorkspaceTargets.map((workspace) => (
        <SidebarMenuItem
          key={workspace.id}
          icon={FiGrid}
          role="menuitem"
          onClick={() => run(() => onMoveToWorkspace(group.id, workspace.id))}
        >
          Move group to {workspace.name}
        </SidebarMenuItem>
      ))}
      <SidebarMenuItem icon={FiPlus} role="menuitem" onClick={() => run(() => onMoveToNewWorkspace(group.id))}>
        Move group to New Space
      </SidebarMenuItem>
      <SidebarMenuSeparator />
      <SidebarMenuItem icon={FiX} role="menuitem" onClick={() => run(() => onCloseGroup(group.id))}>
        Close group
      </SidebarMenuItem>
      <SidebarMenuItem icon={FiArchive} className="danger" role="menuitem" onClick={() => run(() => onUngroupGroup(group.id))}>
        Ungroup {tabCount} {tabCount === 1 ? "tab" : "tabs"}
      </SidebarMenuItem>
    </SidebarMenuSurface>
  );
}
