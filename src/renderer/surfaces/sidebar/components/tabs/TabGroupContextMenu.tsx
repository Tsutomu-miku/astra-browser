import { useRef, type CSSProperties, type KeyboardEvent } from "react";
import { FiArchive, FiArrowRight, FiChevronDown, FiChevronRight, FiCopy, FiGrid, FiMoon, FiPlus, FiTrash2, FiX } from "react-icons/fi";

import { handleMenuKeyboardNavigation, useMenuInitialFocus } from "../../../../common/context-menu/menuKeyboardNavigation";
import type { TabGroup } from "../../../../domain/browser";
import { TAB_GROUP_COLOR_SWATCHES } from "../../../../domain/tabs/groups";
import { SidebarMenuItem } from "../common/SidebarMenuItem";
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
  const menuRef = useRef<HTMLDivElement | null>(null);
  useMenuInitialFocus(menuRef);
  const run = (action: () => void) => {
    action();
    onClose();
  };
  const closeOnEnter = (event: KeyboardEvent<HTMLInputElement>) => {
    if (event.key === "Enter") onClose();
  };

  return (
    <div
      ref={menuRef}
      className="tab-context-menu tab-group-context-menu"
      role="menu"
      style={{ left, top, "--group-color": group.color } as CSSProperties}
      onClick={(event) => event.stopPropagation()}
      onContextMenu={(event) => event.preventDefault()}
      onKeyDown={handleMenuKeyboardNavigation}
    >
      <SidebarMenuItem icon={group.isCollapsed ? FiChevronRight : FiChevronDown} role="menuitem" onClick={() => run(() => onToggleCollapsed(group.id))}>
        {group.isCollapsed ? "Expand group" : "Collapse group"}
      </SidebarMenuItem>
      <SidebarMenuItem icon={FiMoon} role="menuitem" disabled={!canSleepGroup} onClick={() => run(() => onSleepGroup(group.id))}>
        Sleep group
      </SidebarMenuItem>
      <SidebarMenuItem icon={FiCopy} role="menuitem" onClick={() => run(() => onDuplicateGroup(group.id))}>
        Duplicate group
      </SidebarMenuItem>
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
            aria-label={`Use ${color}`}
            aria-pressed={group.color.toLowerCase() === color}
            style={{ "--swatch": color } as CSSProperties}
            onClick={() => onUpdate(group.id, { color })}
          />
        ))}
      </div>
      <span className="tab-context-menu-separator" />
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
      <span className="tab-context-menu-separator" />
      <SidebarMenuItem icon={FiX} role="menuitem" onClick={() => run(() => onCloseGroup(group.id))}>
        Close group
      </SidebarMenuItem>
      <SidebarMenuItem icon={FiArchive} className="danger" role="menuitem" onClick={() => run(() => onUngroupGroup(group.id))}>
        Ungroup {tabCount} {tabCount === 1 ? "tab" : "tabs"}
      </SidebarMenuItem>
    </div>
  );
}
