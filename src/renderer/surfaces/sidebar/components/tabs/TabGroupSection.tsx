import type { CSSProperties, DragEvent, KeyboardEvent, MouseEvent } from "react";

import { getDisclosureKeyboardToggleIntent } from "../../../../common/disclosure/disclosureKeyboard";
import { clearDropTargetActive, type DropAxis } from "../../../../common/drag-drop/dropPlacement";
import type { BrowserTab, FaviconCache, TabGroup } from "../../../../domain/browser";
import { SIDEBAR_DRAG_DATA } from "../../model/sidebarDragSources";
import { openSidebarKeyboardContextMenu } from "../../model/sidebarKeyboardContextMenu";
import { clearSidebarRowReorderDrop } from "../../model/sidebarRowReorderDrop";
import { acceptSidebarTabGroupHeaderDrag, resolveSidebarTabGroupHeaderDrop } from "../../model/sidebarTabGroupHeaderDrop";
import { getSidebarSearchTargetElementId } from "../../sidebarFiltering";
import { SidebarItemIcon } from "../common/SidebarItemIcon";
import { TabRow } from "./SidebarItems";

export function TabGroupSection({
  activeTab,
  draggingGroupId,
  draggingTabId,
  faviconCache,
  group,
  isCrossFolderDrag,
  onClose,
  onContextMenu,
  onGroupDrop,
  onGroupContextMenu,
  onDrop,
  onGroupTab,
  onMoveTabToGroupFolder,
  onPreview,
  onRenameTab,
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
  isCrossFolderDrag?: (targetTab: BrowserTab) => (draggedId: string) => boolean;
  onClose: (tabId: string) => void;
  onContextMenu: (event: MouseEvent, tab: BrowserTab) => void;
  onGroupDrop: (event: DragEvent<HTMLElement>, targetGroupId: string) => void;
  onGroupContextMenu: (event: MouseEvent, group: TabGroup) => void;
  onDrop: (event: DragEvent<HTMLElement>, targetTabId: string, axis?: DropAxis) => void;
  onGroupTab?: (sourceTabId: string, targetTabId: string) => void;
  onMoveTabToGroupFolder: (tabId: string, groupId: string) => void;
  onPreview: (url: string, title?: string) => void;
  onRenameTab?: (tabId: string, customTitle: string | undefined) => void;
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
  const tabGroupHeaderDropState = { draggingGroupId, draggingTabId };
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
        onDoubleClick={onToggle}
        onDragStart={(event) => {
          setDraggingGroupId(group.id);
          event.dataTransfer.effectAllowed = "move";
          event.dataTransfer.setData(SIDEBAR_DRAG_DATA.groupId, group.id);
        }}
        onDragEnd={() => setDraggingGroupId(null)}
        onDragOver={(event) => {
          const intent = acceptSidebarTabGroupHeaderDrag(event, tabGroupHeaderDropState, group.id);
          if (intent) {
            const ancestor = event.currentTarget.closest(".sidebar-section, .essentials") as HTMLElement | null;
            if (ancestor) clearDropTargetActive(ancestor);
            event.stopPropagation();
          }
        }}
        onDragLeave={(event) => {
          const container = event.currentTarget;
          const next = event.relatedTarget as Node | null;
          if (next && container.contains(next)) return;
          clearSidebarRowReorderDrop(event);
        }}
        onDrop={(event) => {
          const intent = resolveSidebarTabGroupHeaderDrop(event, tabGroupHeaderDropState, group.id);
          if (!intent) return;

          if (intent.type === "group") {
            onGroupDrop(event, group.id);
            return;
          }
          if (intent.type === "currentGroup") {
            setDraggingGroupId(null);
            return;
          }

          onMoveTabToGroupFolder(intent.tabId, group.id);
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
          isCrossFolderDrag={isCrossFolderDrag?.(tab)}
          isSearchSelected={searchSelectedTabId === tab.id}
          splitTabIds={splitTabIds}
          tab={tab}
          onClose={onClose}
          onContextMenu={onContextMenu}
          onDrop={onDrop}
          onGroupTab={onGroupTab}
          onPreview={onPreview}
          onRenameTab={onRenameTab}
          onSelect={onSelect}
          onSplit={onSplit}
          setDraggingTabId={setDraggingTabId}
        />
      ))}
      {group.isCollapsed && !hasActiveTab && tabs.length > 0 && (
        <div className="tab-group-collapsed-preview" aria-hidden="true">
          {tabs.slice(0, 4).map((tab, index) => (
            <span
              className="tab-group-collapsed-favicon"
              key={tab.id}
              style={{ "--folder-index": index } as CSSProperties}
              title={tab.title || tab.url}
            >
              <SidebarItemIcon
                className="tab-favicon"
                faviconCache={faviconCache}
                faviconUrl={tab.faviconUrl}
                url={tab.url}
              />
            </span>
          ))}
        </div>
      )}
    </section>
  );
}
