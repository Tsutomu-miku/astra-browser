import type { CSSProperties, DragEvent, KeyboardEvent, MouseEvent } from "react";

import { getDisclosureKeyboardToggleIntent } from "../../../../common/disclosure/disclosureKeyboard";
import { clearDropPlacement, updateDropPlacement, type DropAxis } from "../../../../common/drag-drop/dropPlacement";
import type { BrowserTab, FaviconCache, TabGroup } from "../../../../domain/browser";
import { SIDEBAR_DRAG_DATA, readSidebarGroupDragId, readSidebarTabDragEventId } from "../../model/sidebarDragSources";
import { openSidebarKeyboardContextMenu } from "../../model/sidebarKeyboardContextMenu";
import { getSidebarSearchTargetElementId } from "../../sidebarFiltering";
import { TabRow } from "./SidebarItems";

export function TabGroupSection({
  activeTab,
  draggingGroupId,
  draggingTabId,
  faviconCache,
  group,
  onClose,
  onContextMenu,
  onGroupDrop,
  onGroupContextMenu,
  onDrop,
  onMoveTabToGroupFolder,
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
  faviconCache?: FaviconCache;
  group: TabGroup;
  onClose: (tabId: string) => void;
  onContextMenu: (event: MouseEvent, tab: BrowserTab) => void;
  onGroupDrop: (event: DragEvent<HTMLElement>, targetGroupId: string) => void;
  onGroupContextMenu: (event: MouseEvent, group: TabGroup) => void;
  onDrop: (event: DragEvent<HTMLElement>, targetTabId: string, axis?: DropAxis) => void;
  onMoveTabToGroupFolder: (tabId: string, groupId: string) => void;
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
  const getDraggedGroupId = (event: DragEvent<HTMLElement>) => readSidebarGroupDragId(
    { draggingGroupId },
    (type) => event.dataTransfer.getData(type)
  );
  const handleToggleKeyDown = (event: KeyboardEvent<HTMLButtonElement>) => {
    if (openSidebarKeyboardContextMenu(event)) return;

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
        data-collapsed={group.isCollapsed}
        data-group-id={group.id}
        data-dragging={draggingGroupId === group.id}
        data-drop-target={Boolean(draggingGroupId && draggingGroupId !== group.id)}
        onContextMenu={(event) => onGroupContextMenu(event, group)}
        onDragStart={(event) => {
          setDraggingGroupId(group.id);
          event.dataTransfer.effectAllowed = "move";
          event.dataTransfer.setData(SIDEBAR_DRAG_DATA.groupId, group.id);
        }}
        onDragEnd={() => setDraggingGroupId(null)}
        onDragOver={(event) => {
          const draggedTabId = readSidebarTabDragEventId({ draggingTabId }, event.dataTransfer);
          const draggedGroupId = getDraggedGroupId(event);
          if (draggedGroupId && draggedGroupId !== group.id) {
            event.preventDefault();
            event.dataTransfer.dropEffect = "move";
            updateDropPlacement(event.currentTarget, event, "vertical");
          } else if (draggedTabId) {
            event.preventDefault();
          }
        }}
        onDragLeave={(event) => clearDropPlacement(event.currentTarget)}
        onDrop={(event) => {
          clearDropPlacement(event.currentTarget);
          const draggedGroupId = getDraggedGroupId(event);
          if (draggedGroupId && draggedGroupId !== group.id) {
            onGroupDrop(event, group.id);
            return;
          }
          if (draggedGroupId) {
            event.preventDefault();
            setDraggingGroupId(null);
            return;
          }
          const tabId = readSidebarTabDragEventId({ draggingTabId }, event.dataTransfer);
          if (!tabId) return;

          event.preventDefault();
          onMoveTabToGroupFolder(tabId, group.id);
          setDraggingTabId(null);
        }}
      >
        <button
          className="tab-group-toggle"
          type="button"
          aria-expanded={!group.isCollapsed}
          aria-label={`${group.isCollapsed ? "Expand" : "Collapse"} tab group ${group.name}`}
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
      </div>
      {(!group.isCollapsed || hasActiveTab) && tabs.map((tab) => (
        <TabRow
          key={tab.id}
          activeTabId={activeTab.id}
          draggingTabId={draggingTabId}
          faviconCache={faviconCache}
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
