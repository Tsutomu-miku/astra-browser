import type { CSSProperties, KeyboardEvent } from "react";

import type { TabGroup } from "../../../../domain/browser";
import { TAB_GROUP_COLOR_SWATCHES } from "../../../../domain/tabs/groups";

interface TabGroupContextMenuProps {
  group: TabGroup;
  left: number;
  onClose: () => void;
  onToggleCollapsed: (groupId: string) => void;
  onUngroupGroup: (groupId: string) => void;
  onUpdate: (groupId: string, patch: Partial<Pick<TabGroup, "name" | "color">>) => void;
  tabCount: number;
  top: number;
}

export function TabGroupContextMenu({
  group,
  left,
  onClose,
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
      <span className="tab-context-menu-separator" />
      <button className="danger" type="button" role="menuitem" onClick={() => run(() => onUngroupGroup(group.id))}>
        Ungroup {tabCount} {tabCount === 1 ? "tab" : "tabs"}
      </button>
    </div>
  );
}
