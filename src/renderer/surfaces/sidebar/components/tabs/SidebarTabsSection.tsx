import type { DragEvent, MouseEvent } from "react";
import { FiPlus } from "react-icons/fi";

import { getPointerDropPlacement, type DropAxis } from "../../../../common/drag-drop/dropPlacement";
import type { BrowserTab, FaviconCache, TabGroup } from "../../../../domain/browser";
import type { BrowserController } from "../../../../app/controller/types";
import { readSidebarGroupDragId } from "../../model/sidebarDragSources";
import {
  acceptSidebarTabFolderDrag,
  getSidebarTabFolderDragId
} from "../../model/sidebarTabFolderDrop";
import { getSidebarSearchTargetElementId, type SidebarFilterResult, type SidebarSearchTarget } from "../../sidebarFiltering";
import { TabRow } from "./SidebarItems";
import { TabGroupSection } from "./TabGroupSection";

export function SidebarTabsSection({
  actions,
  activeSearchTarget,
  activeTab,
  draggingGroupId,
  draggingTabId,
  faviconCache,
  filteredItems,
  isCrossFolderDrag,
  onTabContextMenu,
  onTabDrop,
  onTabGroupCreate,
  onTabGroupContextMenu,
  onTabsDrop,
  onRenameTab,
  setDraggingGroupId,
  setDraggingTabId,
  splitTabIds
}: {
  actions: BrowserController["actions"];
  activeSearchTarget?: SidebarSearchTarget;
  activeTab: BrowserTab;
  draggingGroupId: string | null;
  draggingTabId: string | null;
  faviconCache?: FaviconCache;
  filteredItems: SidebarFilterResult;
  isCrossFolderDrag?: (targetTab: BrowserTab) => (draggedId: string) => boolean;
  onTabContextMenu: (event: MouseEvent, tab: BrowserTab) => void;
  onTabDrop: (event: DragEvent<HTMLElement>, targetTabId: string, axis?: DropAxis) => void;
  onTabGroupCreate?: (sourceTabId: string, targetTabId: string) => void;
  onTabGroupContextMenu: (event: MouseEvent, group: TabGroup) => void;
  onTabsDrop: (event: DragEvent<HTMLElement>) => void;
  onRenameTab?: (tabId: string, customTitle: string | undefined) => void;
  setDraggingGroupId: (groupId: string | null) => void;
  setDraggingTabId: (tabId: string | null) => void;
  splitTabIds: string[];
}) {
  const onGroupDrop = (event: DragEvent<HTMLElement>, targetGroupId: string) => {
    event.preventDefault();
    event.stopPropagation();
    const groupId = readSidebarGroupDragId({ draggingGroupId }, (type) => event.dataTransfer.getData(type));
    if (!groupId || groupId === targetGroupId) {
      setDraggingGroupId(null);
      return;
    }

    actions.reorderTabGroup(groupId, targetGroupId, getPointerDropPlacement(event.currentTarget, event, "vertical"));
    setDraggingGroupId(null);
  };
  const acceptTabsFolderDrag = (event: DragEvent<HTMLElement>) => {
    acceptSidebarTabFolderDrag(event, draggingTabId);
  };

  return (
    <section
      className="sidebar-section tabs-section"
      onDragEnter={(event) => {
        acceptTabsFolderDrag(event);
      }}
      onDragOver={(event) => {
        acceptTabsFolderDrag(event);
      }}
      onDrop={onTabsDrop}
    >
      {!filteredItems.isFiltering && (
        <div className="tabs-section-toolbar">
          <button
            className="sidebar-new-tab-button"
            type="button"
            title="New Tab"
            aria-label="New Tab"
            onClick={(event) => {
              event.stopPropagation();
              actions.newTab();
            }}
          >
            <FiPlus />
          </button>
        </div>
      )}
      <nav
        className="tabs"
        aria-label="Tabs"
      >
        {filteredItems.groupedTabs.map(({ group, tabs }) => (
          <TabGroupSection
            key={group.id}
            activeTab={activeTab}
            draggingGroupId={draggingGroupId}
            group={group}
            faviconCache={faviconCache}
            isCrossFolderDrag={isCrossFolderDrag}
            searchSelectedTabId={activeSearchTarget?.type === "tab" ? activeSearchTarget.id : undefined}
            splitTabIds={splitTabIds}
            tabs={tabs}
            draggingTabId={draggingTabId}
            onClose={actions.closeTab}
            onContextMenu={onTabContextMenu}
            onDrop={onTabDrop}
            onGroupTab={onTabGroupCreate}
            onMoveTabToGroupFolder={(tabId, groupId) => actions.moveTabToFolderEnd(tabId, { type: "group", groupId })}
            onGroupDrop={onGroupDrop}
            onGroupContextMenu={onTabGroupContextMenu}
            onPreview={actions.openGlance}
            onRenameTab={onRenameTab}
            onSelect={actions.selectTab}
            onSplit={actions.openTabInSplit}
            onToggle={() => actions.toggleTabGroupCollapsed(group.id)}
            setDraggingTabId={setDraggingTabId}
            setDraggingGroupId={setDraggingGroupId}
          />
        ))}
        {filteredItems.regularTabs.map((tab) => (
          <TabRow
            key={tab.id}
            activeTabId={activeTab.id}
            draggingTabId={draggingTabId}
            faviconCache={faviconCache}
            id={getSidebarSearchTargetElementId({ type: "tab", id: tab.id, title: tab.title || tab.url, url: tab.url })}
            isCrossFolderDrag={isCrossFolderDrag?.(tab)}
            splitTabIds={splitTabIds}
            isSearchSelected={activeSearchTarget?.type === "tab" && activeSearchTarget.id === tab.id}
            tab={tab}
            onClose={actions.closeTab}
            onContextMenu={onTabContextMenu}
            onDrop={onTabDrop}
            onGroupTab={onTabGroupCreate}
            onPreview={actions.openGlance}
            onRenameTab={onRenameTab}
            onSelect={actions.selectTab}
            onSplit={actions.openTabInSplit}
            setDraggingTabId={setDraggingTabId}
          />
        ))}
        {filteredItems.isFiltering && !filteredItems.hasMatches && (
          <p className="sidebar-empty" role="status">No matches</p>
        )}
      </nav>
    </section>
  );
}
