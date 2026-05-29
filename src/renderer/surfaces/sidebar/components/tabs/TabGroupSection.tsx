import type { CSSProperties, DragEvent, MouseEvent } from "react";

import type { DropAxis } from "../../../../common/drag-drop/dropPlacement";
import type { BrowserTab, TabGroup } from "../../../../domain/browser";
import { openSidebarKeyboardContextMenu } from "../../model/sidebarKeyboardContextMenu";
import { TabRow } from "./SidebarItems";

export function TabGroupSection({
  activeTab,
  draggingGroupId,
  draggingTabId,
  group,
  onAssignTab,
  onClose,
  onContextMenu,
  onGroupContextMenu,
  onDrop,
  onPreview,
  onSelect,
  onSplit,
  onToggle,
  onUpdate,
  searchSelectedTabId,
  setDraggingGroupId,
  setDraggingTabId,
  splitTabIds,
  tabs
}: {
  activeTab: BrowserTab;
  draggingGroupId: string | null;
  draggingTabId: string | null;
  group: TabGroup;
  onAssignTab: (tabId: string, groupId: string) => void;
  onClose: (tabId: string) => void;
  onContextMenu: (event: MouseEvent, tab: BrowserTab) => void;
  onGroupContextMenu: (event: MouseEvent, group: TabGroup) => void;
  onDrop: (event: DragEvent<HTMLElement>, targetTabId: string, axis?: DropAxis) => void;
  onPreview: (url: string, title?: string) => void;
  onSelect: (tabId: string) => void;
  onSplit: (tabId: string) => void;
  onToggle: () => void;
  onUpdate: (groupId: string, patch: Partial<Pick<TabGroup, "name" | "color">>) => void;
  searchSelectedTabId?: string;
  setDraggingGroupId: (groupId: string | null) => void;
  setDraggingTabId: (tabId: string | null) => void;
  splitTabIds: string[];
  tabs: BrowserTab[];
}) {
  const hasActiveTab = tabs.some((tab) => tab.id === activeTab.id);

  return (
    <section className="tab-group" style={{ "--group-color": group.color } as CSSProperties}>
      <div
        className="tab-group-header"
        draggable
        data-dragging={draggingGroupId === group.id}
        data-drop-target={Boolean(draggingTabId)}
        onContextMenu={(event) => onGroupContextMenu(event, group)}
        onKeyDown={openSidebarKeyboardContextMenu}
        onDragStart={(event) => {
          setDraggingGroupId(group.id);
          event.dataTransfer.effectAllowed = "move";
          event.dataTransfer.setData("text/group-id", group.id);
        }}
        onDragEnd={() => setDraggingGroupId(null)}
        onDragOver={(event) => {
          if (draggingTabId) event.preventDefault();
        }}
        onDrop={(event) => {
          event.preventDefault();
          const tabId = draggingTabId || event.dataTransfer.getData("text/plain");
          if (tabId) onAssignTab(tabId, group.id);
          setDraggingTabId(null);
        }}
      >
        <button
          className="tab-group-toggle"
          type="button"
          aria-expanded={!group.isCollapsed}
          title={group.isCollapsed ? "Expand group" : "Collapse group"}
          onClick={onToggle}
        >
          <span className="tab-group-dot" />
        </button>
        <input
          className="tab-group-title-input"
          aria-label="Tab group name"
          value={group.name}
          onChange={(event) => onUpdate(group.id, { name: event.target.value })}
        />
        <span className="tab-group-count">{tabs.length}</span>
        <input
          className="tab-group-color"
          aria-label="Tab group color"
          type="color"
          value={group.color}
          onChange={(event) => onUpdate(group.id, { color: event.target.value })}
        />
      </div>
      {(!group.isCollapsed || hasActiveTab) && tabs.map((tab) => (
        <TabRow
          key={tab.id}
          activeTabId={activeTab.id}
          draggingTabId={draggingTabId}
          isSearchSelected={searchSelectedTabId === tab.id}
          splitTabIds={splitTabIds}
          tab={tab}
          onClose={onClose}
          onContextMenu={onContextMenu}
          onDrop={onDrop}
          onPreview={onPreview}
          onSelect={onSelect}
          onSplit={onSplit}
          setDraggingTabId={setDraggingTabId}
        />
      ))}
    </section>
  );
}
