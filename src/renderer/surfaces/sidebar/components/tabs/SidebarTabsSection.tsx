import type { DragEvent, MouseEvent } from "react";

import { getPointerDropPlacement } from "../../../../common/drag-drop/dropPlacement";
import type { DropAxis } from "../../../../common/drag-drop/dropPlacement";
import type { BrowserTab, FaviconCache, TabGroup } from "../../../../domain/browser";
import type { BrowserController } from "../../../../app/controller/types";
import { readSidebarGroupDragId } from "../../model/sidebarDragSources";
import { acceptSidebarTabFolderDrag } from "../../model/sidebarTabFolderDrop";
import { getSidebarSearchTargetElementId, type SidebarFilterResult, type SidebarSearchTarget } from "../../sidebarFiltering";
import { SidebarSectionHeader, TabRow } from "./SidebarItems";
import { TabGroupSection } from "./TabGroupSection";

export function SidebarTabsSection({
  actions,
  activeSearchTarget,
  activeTab,
  draggingGroupId,
  draggingTabId,
  faviconCache,
  filteredItems,
  isCollapsed,
  onTabContextMenu,
  onTabDrop,
  onTabGroupContextMenu,
  onTabsDrop,
  onToggle,
  setDraggingGroupId,
  setDraggingTabId,
  splitTabIds,
  tabCount
}: {
  actions: BrowserController["actions"];
  activeSearchTarget?: SidebarSearchTarget;
  activeTab: BrowserTab;
  draggingGroupId: string | null;
  draggingTabId: string | null;
  faviconCache?: FaviconCache;
  filteredItems: SidebarFilterResult;
  isCollapsed: boolean;
  onTabContextMenu: (event: MouseEvent, tab: BrowserTab) => void;
  onTabDrop: (event: DragEvent<HTMLElement>, targetTabId: string, axis?: DropAxis) => void;
  onTabGroupContextMenu: (event: MouseEvent, group: TabGroup) => void;
  onTabsDrop: (event: DragEvent<HTMLElement>) => void;
  onToggle: () => void;
  setDraggingGroupId: (groupId: string | null) => void;
  setDraggingTabId: (tabId: string | null) => void;
  splitTabIds: string[];
  tabCount: number;
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
      <SidebarSectionHeader
        count={tabCount}
        isCollapsed={isCollapsed}
        title="Tabs"
        onToggle={onToggle}
      />
      {!isCollapsed && <nav
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
            searchSelectedTabId={activeSearchTarget?.type === "tab" ? activeSearchTarget.id : undefined}
            splitTabIds={splitTabIds}
            tabs={tabs}
            draggingTabId={draggingTabId}
            onClose={actions.closeTab}
            onContextMenu={onTabContextMenu}
            onDrop={onTabDrop}
            onMoveTabToGroupFolder={(tabId, groupId) => actions.moveTabToFolderEnd(tabId, { type: "group", groupId })}
            onGroupDrop={onGroupDrop}
            onGroupContextMenu={onTabGroupContextMenu}
            onPreview={actions.openGlance}
            onSelect={actions.selectTab}
            onSplit={actions.openTabInSplit}
            onToggle={() => actions.toggleTabGroupCollapsed(group.id)}
            onUpdate={actions.updateTabGroup}
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
            splitTabIds={splitTabIds}
            isSearchSelected={activeSearchTarget?.type === "tab" && activeSearchTarget.id === tab.id}
            tab={tab}
            onClose={actions.closeTab}
            onContextMenu={onTabContextMenu}
            onDrop={onTabDrop}
            onPreview={actions.openGlance}
            onSelect={actions.selectTab}
            onSplit={actions.openTabInSplit}
            setDraggingTabId={setDraggingTabId}
          />
        ))}
        {filteredItems.isFiltering && !filteredItems.hasMatches && (
          <p className="sidebar-empty" role="status">No matches</p>
        )}
      </nav>}
    </section>
  );
}
