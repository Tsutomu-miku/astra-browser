import type { DragEvent, MouseEvent } from "react";

import type { BrowserTab, TabGroup } from "../../../../domain/browser";
import type { BrowserController } from "../../../../app/controller/types";
import type { SidebarFilterResult, SidebarSearchTarget } from "../../sidebarFiltering";
import { SidebarSectionHeader, TabRow } from "./SidebarItems";
import { TabGroupSection } from "./TabGroupSection";
import { TabOrganizationDropTargets } from "./TabOrganizationDropTargets";

export function SidebarTabsSection({
  actions,
  activeSearchTarget,
  activeTab,
  canUnpinDraggedTabToTabs,
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
  canUnpinDraggedTabToTabs: boolean;
  draggingGroupId: string | null;
  draggingTabId: string | null;
  filteredItems: SidebarFilterResult;
  isCollapsed: boolean;
  onTabContextMenu: (event: MouseEvent, tab: BrowserTab) => void;
  onTabDrop: (event: DragEvent<HTMLElement>, targetTabId: string) => void;
  onTabGroupContextMenu: (event: MouseEvent, group: TabGroup) => void;
  onTabsDrop: (event: DragEvent<HTMLElement>) => void;
  onToggle: () => void;
  setDraggingGroupId: (groupId: string | null) => void;
  setDraggingTabId: (tabId: string | null) => void;
  splitTabIds: string[];
  tabCount: number;
}) {
  const canCreateGroupFromDraggedTab = Boolean(
    draggingTabId && filteredItems.regularTabs.some((tab) => tab.id === draggingTabId)
  );
  const canUngroupDraggedTab = Boolean(
    draggingTabId && filteredItems.groupedTabs.some((entry) => entry.tabs.some((tab) => tab.id === draggingTabId))
  );

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
        data-drop-target={canUnpinDraggedTabToTabs}
        onDragOver={(event) => {
          if (canUnpinDraggedTabToTabs) {
            event.preventDefault();
            event.dataTransfer.dropEffect = "move";
          }
        }}
        onDrop={onTabsDrop}
      >
        {!filteredItems.isFiltering && (
          <TabOrganizationDropTargets
            canCreateGroup={canCreateGroupFromDraggedTab}
            canUngroup={canUngroupDraggedTab}
            draggingTabId={draggingTabId}
            onCreateGroup={actions.groupTab}
            onUngroupTab={actions.ungroupTab}
            setDraggingTabId={setDraggingTabId}
          />
        )}
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
