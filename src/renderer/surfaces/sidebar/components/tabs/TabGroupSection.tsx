import type { CSSProperties, DragEvent, KeyboardEvent, MouseEvent } from "react";

import { getDisclosureKeyboardToggleIntent } from "../../../../common/disclosure/disclosureKeyboard";
import { type DropAxis } from "../../../../common/drag-drop/dropPlacement";
import type { BrowserTab, FaviconCache, TabGroup } from "../../../../domain/browser";
import { SIDEBAR_DRAG_DATA, readSidebarGroupDragId, readSidebarTabDragEventId } from "../../model/sidebarDragSources";
import { openSidebarKeyboardContextMenu } from "../../model/sidebarKeyboardContextMenu";
import { acceptSidebarRowReorderDrag, clearSidebarRowReorderDrop, resolveSidebarRowReorderDrop } from "../../model/sidebarRowReorderDrop";
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
          const draggedGroupId = acceptSidebarRowReorderDrag(event, {
            readDragId: getDraggedGroupId,
            targetId: group.id
          });
          if (!draggedGroupId && draggedTabId) {
            event.preventDefault();
          }
        }}
        onDragLeave={clearSidebarRowReorderDrop}
        onDrop={(event) => {
          const draggedGroupId = resolveSidebarRowReorderDrop(event, {
            readDragId: getDraggedGroupId,
            targetId: group.id
          });
          if (draggedGroupId) {
            onGroupDrop(event, group.id);
            return;
          }
          if (getDraggedGroupId(event)) {
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
        <span className="tab-group-title">{group.name}</span>
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
