import type { CSSProperties, DragEvent, KeyboardEvent, MouseEvent } from "react";

import { getDisclosureKeyboardToggleIntent } from "../../../../common/disclosure/disclosureKeyboard";
import { clearDropPlacement, updateDropPlacement, type DropAxis } from "../../../../common/drag-drop/dropPlacement";
import type { BrowserTab, TabGroup } from "../../../../domain/browser";
import { openSidebarKeyboardContextMenu } from "../../model/sidebarKeyboardContextMenu";
import { getSidebarSearchTargetElementId } from "../../sidebarFiltering";
import { TabRow } from "./SidebarItems";

export function TabGroupSection({
  activeTab,
  draggingGroupId,
  draggingTabId,
  group,
  onAssignTab,
  onClose,
  onContextMenu,
  onGroupDrop,
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
  onGroupDrop: (event: DragEvent<HTMLElement>, targetGroupId: string) => void;
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
  const handleToggleKeyDown = (event: KeyboardEvent<HTMLButtonElement>) => {
    const intent = getDisclosureKeyboardToggleIntent(event.key, group.isCollapsed);
    if (!intent) return;

    event.preventDefault();
    event.stopPropagation();
    onToggle();
  };

  return (
    <section className="tab-group" style={{ "--group-color": group.color } as CSSProperties}>
      <div
        className="tab-group-header"
        draggable
        data-dragging={draggingGroupId === group.id}
        data-drop-target={Boolean(draggingTabId || (draggingGroupId && draggingGroupId !== group.id))}
        onContextMenu={(event) => onGroupContextMenu(event, group)}
        onKeyDown={openSidebarKeyboardContextMenu}
        onDragStart={(event) => {
          setDraggingGroupId(group.id);
          event.dataTransfer.effectAllowed = "move";
          event.dataTransfer.setData("text/group-id", group.id);
        }}
        onDragEnd={() => setDraggingGroupId(null)}
        onDragOver={(event) => {
          if (draggingGroupId && draggingGroupId !== group.id) {
            event.preventDefault();
            event.dataTransfer.dropEffect = "move";
            updateDropPlacement(event.currentTarget, event, "vertical");
          } else if (draggingTabId) {
            event.preventDefault();
          }
        }}
        onDragLeave={(event) => clearDropPlacement(event.currentTarget)}
        onDrop={(event) => {
          clearDropPlacement(event.currentTarget);
          if (draggingGroupId && draggingGroupId !== group.id) {
            onGroupDrop(event, group.id);
            return;
          }
          if (draggingGroupId) {
            event.preventDefault();
            setDraggingGroupId(null);
            return;
          }
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
          aria-label={`${group.isCollapsed ? "Expand" : "Collapse"} tab group ${group.name}`}
          title={group.isCollapsed ? "Expand group" : "Collapse group"}
          onClick={onToggle}
          onKeyDown={handleToggleKeyDown}
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
          id={getSidebarSearchTargetElementId({ type: "tab", id: tab.id, title: tab.title || tab.url, url: tab.url })}
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
