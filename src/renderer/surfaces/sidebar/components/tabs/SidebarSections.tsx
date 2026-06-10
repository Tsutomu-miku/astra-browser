import { useCallback, useState, type DragEvent, type MouseEvent } from "react";

import type { DropAxis } from "../../../../common/drag-drop/dropPlacement";
import { getPointerDropPlacement } from "../../../../common/drag-drop/dropPlacement";
import {
  DEFAULT_SIDEBAR_SECTION_COLLAPSED,
  toggleSidebarSectionCollapsed,
  type SidebarSectionCollapsedState,
  type SidebarSectionId
} from "../../../../common/sidebar/sidebarSections";
import { type BrowserTab, type ClosedTab, type Favorite, type FaviconCache, type TabGroup } from "../../../../domain/browser";
import type { BrowserController } from "../../../../app/controller/types";
import {
  isSidebarUrlActive
} from "../../model/sidebarItemState";
import { acceptSidebarTabFolderDrag, clearSidebarTabFolderDrop } from "../../model/sidebarTabFolderDrop";
import { getSidebarSearchTargetElementId, type SidebarFilterResult, type SidebarSearchTarget } from "../../sidebarFiltering";
import { SidebarSectionHeader } from "../common/SidebarSectionHeader";
import { FavoriteButton, TabRow } from "./SidebarItems";
import { SidebarTabsSection } from "./SidebarTabsSection";
import { TabGroupSection } from "./TabGroupSection";
import { readSidebarGroupDragId } from "../../model/sidebarDragSources";

const SIDEBAR_ESSENTIALS_LIMIT = 8;

export function SidebarSections({
  actions,
  activeSearchTarget,
  activeTab,
  closedTabs,
  draggingClosedTabIndex = null,
  draggingEssentialId,
  faviconCache,
  draggingFavoriteId,
  draggingGroupId,
  draggingTabId,
  filteredItems,
  onEssentialDragStart,
  onEssentialDrop,
  onEssentialReorderDrop,
  collapsedSections,
  onFavoriteDragStart,
  onFavoriteDrop,
  onFavoriteReorderDrop,
  onFavoriteTabDrop,
  onClosedTabContextMenu,
  onTabGroupContextMenu,
  onRenameGroup,
  onRenameTab,
  onTabContextMenu,
  onTabDrop,
  onTabGroupCreate,
  onTabsDrop = () => undefined,
  onToggleSection,
  onQuickEntryContextMenu,
  setDraggingEssentialId,
  setDraggingFavoriteId,
  setDraggingClosedTabIndex = () => undefined,
  setDraggingGroupId,
  setDraggingTabId,
  splitTabIds,
  workspaceTabs = []
}: {
  actions: BrowserController["actions"];
  activeSearchTarget?: SidebarSearchTarget;
  activeTab: BrowserTab;
  closedTabs?: ClosedTab[];
  draggingClosedTabIndex?: number | null;
  draggingEssentialId: string | null;
  faviconCache?: FaviconCache;
  draggingFavoriteId: string | null;
  draggingGroupId: string | null;
  draggingTabId: string | null;
  filteredItems: SidebarFilterResult;
  onEssentialDragStart: (event: DragEvent<HTMLElement>, essentialId: string) => void;
  onEssentialDrop: (event: DragEvent<HTMLElement>) => void;
  onEssentialReorderDrop: (event: DragEvent<HTMLElement>, targetEssentialId: string, axis: DropAxis) => void;
  collapsedSections?: SidebarSectionCollapsedState;
  onFavoriteDragStart: (event: DragEvent<HTMLElement>, favoriteId: string) => void;
  onFavoriteDrop: (event: DragEvent<HTMLElement>) => void;
  onFavoriteReorderDrop: (event: DragEvent<HTMLElement>, targetFavoriteId: string, axis: DropAxis) => void;
  onFavoriteTabDrop?: (event: DragEvent<HTMLElement>, targetTabId: string, axis: DropAxis) => void;
  onClosedTabContextMenu?: (event: MouseEvent, tab: ClosedTab, closedIndex: number) => void;
  onQuickEntryContextMenu: (event: MouseEvent, item: Favorite, kind: "essential" | "favorite") => void;
  onRenameGroup?: (groupId: string, customName: string | undefined) => void;
  onRenameTab?: (tabId: string, customTitle: string | undefined) => void;
  onTabContextMenu: (event: MouseEvent, tab: BrowserTab) => void;
  onTabGroupContextMenu: (event: MouseEvent, group: TabGroup) => void;
  onTabDrop: (event: DragEvent<HTMLElement>, targetTabId: string, axis?: DropAxis) => void;
  onTabGroupCreate?: (sourceTabId: string, targetTabId: string) => void;
  onTabsDrop?: (event: DragEvent<HTMLElement>) => void;
  onToggleSection?: (sectionId: SidebarSectionId) => void;
  setDraggingEssentialId: (essentialId: string | null) => void;
  setDraggingFavoriteId: (favoriteId: string | null) => void;
  setDraggingClosedTabIndex?: (closedIndex: number | null) => void;
  setDraggingGroupId: (groupId: string | null) => void;
  setDraggingTabId: (tabId: string | null) => void;
  splitTabIds: string[];
  workspaceTabs?: BrowserTab[];
}) {
  const [localCollapsedSections, setLocalCollapsedSections] = useState<SidebarSectionCollapsedState>(DEFAULT_SIDEBAR_SECTION_COLLAPSED);
  const currentCollapsedSections = collapsedSections ?? localCollapsedSections;
  const isSectionCollapsed = (sectionId: SidebarSectionId) => (
    !filteredItems.isFiltering &&
    currentCollapsedSections[sectionId]
  );
  const toggleSection = (sectionId: SidebarSectionId) => {
    if (onToggleSection) {
      onToggleSection(sectionId);
      return;
    }

    setLocalCollapsedSections((current) => toggleSidebarSectionCollapsed(current, sectionId));
  };
  const showFavoritesFolder = filteredItems.favorites.length > 0 || !filteredItems.isFiltering;
  const favoriteSearchSelectedTabId = activeSearchTarget?.type === "favorite" ? activeSearchTarget.id : undefined;
  const handleSectionDragLeave = (event: DragEvent<HTMLElement>) => {
    const container = event.currentTarget;
    const next = event.relatedTarget as Node | null;
    if (next && container.contains(next)) return;
    clearSidebarTabFolderDrop(event);
  };
  const handleEssentialDrop = (event: DragEvent<HTMLElement>) => {
    clearSidebarTabFolderDrop(event);
    onEssentialDrop(event);
  };
  const handleFavoriteDrop = (event: DragEvent<HTMLElement>) => {
    clearSidebarTabFolderDrop(event);
    onFavoriteDrop(event);
  };
  const getTabById = useCallback((tabId: string) => workspaceTabs.find((t) => t.id === tabId), [workspaceTabs]);
  const buildCrossFolderCheckFor = useCallback((targetTab: BrowserTab) => (draggedId: string) => {
    const dragged = getTabById(draggedId);
    if (!dragged) return false;
    return dragged.isFavorite !== targetTab.isFavorite || dragged.isPinned !== targetTab.isPinned;
  }, [getTabById]);
  const onFavoriteGroupDrop = (event: DragEvent<HTMLElement>, targetGroupId: string) => {
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
  const renderFavoriteItem = (
    entry: { kind: "tab"; tab: BrowserTab } | { kind: "group"; group: TabGroup; tabs: BrowserTab[] }
  ) => {
    if (entry.kind === "tab") {
      const tab = entry.tab;
      return (
        <TabRow
          key={tab.id}
          activeTabId={activeTab.id}
          draggingTabId={draggingTabId}
          faviconCache={faviconCache}
          id={getSidebarSearchTargetElementId({ type: "favorite", id: tab.id, tabId: tab.id, title: tab.title, url: tab.url })}
          isCrossFolderDrag={buildCrossFolderCheckFor(tab)}
          isSearchSelected={favoriteSearchSelectedTabId === tab.id}
          labelKind="favorite tab"
          splitTabIds={splitTabIds}
          tab={tab}
          onClose={actions.closeTab}
          onContextMenu={onTabContextMenu}
          onDrop={(event) => onFavoriteTabDrop?.(event, tab.id, "vertical")}
          onGroupTab={onTabGroupCreate}
          onPreview={actions.openGlance}
          onRenameTab={onRenameTab}
          onSelect={actions.selectTab}
          onSplit={actions.openTabInSplit}
          setDraggingTabId={setDraggingTabId}
        />
      );
    }
    return (
      <TabGroupSection
        key={entry.group.id}
        activeTab={activeTab}
        draggingGroupId={draggingGroupId}
        draggingTabId={draggingTabId}
        faviconCache={faviconCache}
        group={entry.group}
        isCrossFolderDrag={buildCrossFolderCheckFor}
        onClose={actions.closeTab}
        onContextMenu={onTabContextMenu}
        onDrop={(event, tabId, axis = "vertical") => onFavoriteTabDrop?.(event, tabId, axis)}
        onGroupDrop={onFavoriteGroupDrop}
        onGroupContextMenu={onTabGroupContextMenu}
        onGroupTab={onTabGroupCreate}
        onMoveTabToGroupFolder={(tabId, groupId) => actions.moveTabToFolderEnd(tabId, { type: "group", groupId })}
        onPreview={actions.openGlance}
        onRenameGroup={onRenameGroup}
        onRenameTab={onRenameTab}
        onSelect={actions.selectTab}
        onSplit={actions.openTabInSplit}
        onToggle={() => actions.toggleTabGroupCollapsed(entry.group.id)}
        searchSelectedTabId={favoriteSearchSelectedTabId}
        setDraggingGroupId={setDraggingGroupId}
        setDraggingTabId={setDraggingTabId}
        splitTabIds={splitTabIds}
        tabs={entry.tabs}
      />
    );
  };

  return (
    <>
      {filteredItems.essentials.length > 0 && (
        <section className="sidebar-section">
          <nav
            className="essentials"
            aria-label="Essentials"
            onDragOver={(event) => {
              acceptSidebarTabFolderDrag(event, draggingTabId, "copy");
            }}
            onDragLeave={handleSectionDragLeave}
            onDrop={handleEssentialDrop}
          >
            {(filteredItems.isFiltering ? filteredItems.essentials : filteredItems.essentials.slice(0, SIDEBAR_ESSENTIALS_LIMIT)).map((essential) => (
              <FavoriteButton
                key={essential.id}
                draggable
                faviconCache={faviconCache}
                draggingQuickEntryId={draggingEssentialId}
                favorite={essential}
                id={getSidebarSearchTargetElementId({ type: "essential", id: essential.id, title: essential.title, url: essential.url })}
                isActive={isSidebarUrlActive(activeTab.url, essential.url)}
                isSearchSelected={activeSearchTarget?.type === "essential" && activeSearchTarget.id === essential.id}
                kind="essential"
                dropAxis="horizontal"
                onContextMenu={(event, item) => onQuickEntryContextMenu(event, item, "essential")}
                onDragStart={onEssentialDragStart}
                onDragEnd={() => setDraggingEssentialId(null)}
                onDrop={onEssentialReorderDrop}
                onOpen={actions.navigateActiveTab}
                onOpenInSplit={actions.openUrlInSplit}
                onPreview={actions.openGlance}
              />
            ))}
          </nav>
        </section>
      )}

      {showFavoritesFolder && (
        <section
          className="sidebar-section"
          onDragOver={(event) => {
            acceptSidebarTabFolderDrag(event, draggingTabId);
          }}
          onDragLeave={handleSectionDragLeave}
          onDrop={handleFavoriteDrop}
        >
          <SidebarSectionHeader
            count={filteredItems.favorites.length}
            isCollapsed={isSectionCollapsed("favorites")}
            title="Favorites"
            onToggle={() => toggleSection("favorites")}
          />
          {!isSectionCollapsed("favorites") && filteredItems.favorites.length > 0 && <nav
            className="favorites"
            aria-label="Favorites"
          >
            {filteredItems.favorites.map((entry) => renderFavoriteItem(entry))}
          </nav>}
        </section>
      )}

      <SidebarTabsSection
        actions={actions}
        activeSearchTarget={activeSearchTarget}
        activeTab={activeTab}
        draggingGroupId={draggingGroupId}
        draggingTabId={draggingTabId}
        filteredItems={filteredItems}
        faviconCache={faviconCache}
        isCrossFolderDrag={buildCrossFolderCheckFor}
        splitTabIds={splitTabIds}
        onTabContextMenu={onTabContextMenu}
        onTabDrop={onTabDrop}
        onTabGroupCreate={onTabGroupCreate}
        onTabGroupContextMenu={onTabGroupContextMenu}
        onTabsDrop={onTabsDrop}
        onRenameTab={onRenameTab}
        setDraggingGroupId={setDraggingGroupId}
        setDraggingTabId={setDraggingTabId}
      />
    </>
  );
}
