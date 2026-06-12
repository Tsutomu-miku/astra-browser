import type { DragEvent, MouseEvent } from "react";
import { FiPlus } from "react-icons/fi";

import { getPointerDropPlacement, type DropAxis } from "../../../../common/drag-drop/dropPlacement";
import type { BrowserTab, FaviconCache, SplitTab, TabGroup } from "../../../../domain/browser";
import type { BrowserController } from "../../../../app/controller/types";
import { getSplitForTab, isTabInSplit } from "../../../../domain/tabs/splitView";
import { readSidebarGroupDragId } from "../../model/sidebarDragSources";
import {
  acceptSidebarTabFolderDrag,
  clearSidebarTabFolderDrop,
  getSidebarTabFolderDragId
} from "../../model/sidebarTabFolderDrop";
import { getSidebarSearchTargetElementId, type SidebarFilterResult, type SidebarSearchTarget } from "../../sidebarFiltering";
import { TabRow } from "./SidebarItems";
import { TabGroupSection } from "./TabGroupSection";
import { SplitTabRow } from "./SplitTabRow";

export function SidebarTabsSection({
  actions,
  activeSearchTarget,
  activeTab,
  activeSplitId,
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
  onRenameGroup,
  onRenameTab,
  setDraggingGroupId,
  setDraggingTabId,
  splitTabs,
  onSwapSplitPanes
}: {
  actions: BrowserController["actions"];
  activeSearchTarget?: SidebarSearchTarget;
  activeTab: BrowserTab;
  activeSplitId: string | null;
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
  onRenameGroup?: (groupId: string, customName: string | undefined) => void;
  onRenameTab?: (tabId: string, customTitle: string | undefined) => void;
  setDraggingGroupId: (groupId: string | null) => void;
  setDraggingTabId: (tabId: string | null) => void;
  splitTabs: SplitTab[];
  onSwapSplitPanes?: (splitId: string) => void;
}) {
  const onGroupDrop = (event: DragEvent<HTMLElement>, targetGroupId: string) => {
    event.preventDefault();
    event.stopPropagation();
    const sectionEl = event.currentTarget.closest(".sidebar-section") as HTMLElement | null;
    if (sectionEl) clearSidebarTabFolderDrop({ currentTarget: sectionEl });
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
  const handleTabsSectionDragLeave = (event: DragEvent<HTMLElement>) => {
    const container = event.currentTarget;
    const next = event.relatedTarget as Node | null;
    if (next && container.contains(next)) return;
    clearSidebarTabFolderDrop(event);
  };
  const handleTabsSectionDrop = (event: DragEvent<HTMLElement>) => {
    clearSidebarTabFolderDrop(event);
    onTabsDrop(event);
  };

  // Helper: find split for a tab (if any)
  const findSplitForTab = (tabId: string): SplitTab | undefined => {
    return splitTabs.find((s) => s.primaryTabId === tabId || s.secondaryTabId === tabId);
  };

  const isSplitActive = (split: SplitTab): boolean => {
    return activeSplitId === split.id;
  };

  const renderTabOrSplit = (tab: BrowserTab) => {
    const split = findSplitForTab(tab.id);
    if (split && split.secondaryTabId === tab.id) {
      // Secondary tab of a split — skip it (rendered as part of the primary's row)
      return null;
    }
    if (split && split.primaryTabId === tab.id) {
      // Primary tab of a split — render as SplitTabRow
      const secondaryTab = filteredItems.workspaceTabs?.find((t) => t.id === split.secondaryTabId);
      if (!secondaryTab) return null;
      return (
        <SplitTabRow
          key={`split-${split.id}`}
          activeTabId={activeTab.id}
          draggingTabId={draggingTabId}
          faviconCache={faviconCache}
          id={getSidebarSearchTargetElementId({ type: "tab", id: tab.id, title: tab.title || tab.url, url: tab.url })}
          isSearchSelected={activeSearchTarget?.type === "tab" && activeSearchTarget.id === tab.id}
          isActive={isSplitActive(split)}
          primaryTab={tab}
          secondaryTab={secondaryTab}
          onClose={(closeTabId) => actions.closeTab(closeTabId)}
          onContextMenu={onTabContextMenu}
          onDrop={onTabDrop}
          onPreview={actions.openGlance}
          onSelect={() => actions.selectSplitTab?.(split.id)}
          onSwapPanes={() => onSwapSplitPanes?.(split.id)}
          setDraggingTabId={setDraggingTabId}
        />
      );
    }
    // Regular tab
    return (
      <TabRow
        key={tab.id}
        activeTabId={activeTab.id}
        draggingTabId={draggingTabId}
        faviconCache={faviconCache}
        id={getSidebarSearchTargetElementId({ type: "tab", id: tab.id, title: tab.title || tab.url, url: tab.url })}
        isCrossFolderDrag={isCrossFolderDrag?.(tab)}
        splitTabIds={[]}
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
    );
  };

  return (
    <section
      className="sidebar-section tabs-section"
      onDragOver={(event) => {
        acceptTabsFolderDrag(event);
      }}
      onDragLeave={handleTabsSectionDragLeave}
      onDrop={handleTabsSectionDrop}
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
            activeSplitId={activeSplitId}
            draggingGroupId={draggingGroupId}
            group={group}
            faviconCache={faviconCache}
            isCrossFolderDrag={isCrossFolderDrag}
            searchSelectedTabId={activeSearchTarget?.type === "tab" ? activeSearchTarget.id : undefined}
            splitTabs={splitTabs}
            workspaceTabs={filteredItems.workspaceTabs ?? []}
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
            onRenameGroup={onRenameGroup}
            onRenameTab={onRenameTab}
            onSelect={actions.selectTab}
            onSplit={actions.openTabInSplit}
            onSwapSplitPanes={onSwapSplitPanes}
            onToggle={() => actions.toggleTabGroupCollapsed(group.id)}
            setDraggingTabId={setDraggingTabId}
            setDraggingGroupId={setDraggingGroupId}
          />
        ))}
        {filteredItems.regularTabs.map(renderTabOrSplit)}
        {filteredItems.isFiltering && !filteredItems.hasMatches && (
          <p className="sidebar-empty" role="status">No matches</p>
        )}
      </nav>
    </section>
  );
}
