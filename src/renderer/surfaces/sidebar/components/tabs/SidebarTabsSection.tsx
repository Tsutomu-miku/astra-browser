import type { DragEvent, MouseEvent } from "react";

import { getPointerDropPlacement } from "../../../../common/drag-drop/dropPlacement";
import type { DropAxis } from "../../../../common/drag-drop/dropPlacement";
import { readSidebarTabDragPayload } from "../../../../common/drag-drop/sidebarDragPayload";
import type { BrowserTab, TabGroup } from "../../../../domain/browser";
import type { BrowserController } from "../../../../app/controller/types";
import { getSidebarSearchTargetElementId, type SidebarFilterResult, type SidebarSearchTarget } from "../../sidebarFiltering";
import { SidebarSectionHeader, TabRow } from "./SidebarItems";
import { TabGroupSection } from "./TabGroupSection";

export function SidebarTabsSection({
  actions,
  activeSearchTarget,
  activeTab,
  draggingGroupId,
  draggingTabId,
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
    if (!draggingGroupId || draggingGroupId === targetGroupId) {
      setDraggingGroupId(null);
      return;
    }

    actions.reorderTabGroup(draggingGroupId, targetGroupId, getPointerDropPlacement(event.currentTarget, event, "vertical"));
    setDraggingGroupId(null);
  };

  return (
    <section className="sidebar-section tabs-section">
      <SidebarSectionHeader
        count={tabCount}
        isCollapsed={isCollapsed}
        title="Tabs"
        onToggle={onToggle}
      />
      {!isCollapsed && <nav
        className="tabs"
        aria-label="Tabs"
        onDragEnter={(event) => {
          const draggedTabId = draggingTabId || readSidebarTabDragPayload(event.dataTransfer);
          if (draggedTabId && filteredItems.pinnedTabs.some((tab) => tab.id === draggedTabId)) {
            event.preventDefault();
          }
        }}
        onDragOver={(event) => {
          const draggedTabId = draggingTabId || readSidebarTabDragPayload(event.dataTransfer);
          const canUnpinDraggedTab = Boolean(
            draggedTabId && filteredItems.pinnedTabs.some((tab) => tab.id === draggedTabId)
          );
          if (canUnpinDraggedTab) {
            event.preventDefault();
            event.dataTransfer.dropEffect = "move";
          }
        }}
        onDrop={onTabsDrop}
      >
        {filteredItems.groupedTabs.map(({ group, tabs }) => (
          <TabGroupSection
            key={group.id}
            activeTab={activeTab}
            draggingGroupId={draggingGroupId}
            group={group}
            searchSelectedTabId={activeSearchTarget?.type === "tab" ? activeSearchTarget.id : undefined}
            splitTabIds={splitTabIds}
            tabs={tabs}
            draggingTabId={draggingTabId}
            onAssignTab={actions.assignTabToGroup}
            onClose={actions.closeTab}
            onContextMenu={onTabContextMenu}
            onDrop={onTabDrop}
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
          <p className="sidebar-empty">No matching tabs</p>
        )}
      </nav>}
    </section>
  );
}
