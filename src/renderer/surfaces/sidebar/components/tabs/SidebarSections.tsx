import { useState, type DragEvent, type MouseEvent } from "react";

import type { DropAxis } from "../../../../common/drag-drop/dropPlacement";
import {
  DEFAULT_SIDEBAR_SECTION_COLLAPSED,
  toggleSidebarSectionCollapsed,
  type SidebarSectionCollapsedState,
  type SidebarSectionId
} from "../../../../common/sidebar/sidebarSections";
import { resolveFavoriteTab, resolveTabBackedFavoriteTab, type BrowserTab, type ClosedTab, type Favorite, type FaviconCache, type TabGroup } from "../../../../domain/browser";
import type { BrowserController } from "../../../../app/controller/types";
import {
  isSidebarFavoriteActive,
  isSidebarUrlActive
} from "../../model/sidebarItemState";
import { acceptSidebarTabFolderDrag } from "../../model/sidebarTabFolderDrop";
import { getSidebarSearchTargetElementId, type SidebarFilterResult, type SidebarSearchTarget } from "../../sidebarFiltering";
import { SidebarSectionHeader } from "../common/SidebarSectionHeader";
import { FavoriteButton, TabRow } from "./SidebarItems";
import { SidebarTabsSection } from "./SidebarTabsSection";

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
  onClosedTabContextMenu,
  onTabGroupContextMenu,
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
  onClosedTabContextMenu?: (event: MouseEvent, tab: ClosedTab, closedIndex: number) => void;
  onQuickEntryContextMenu: (event: MouseEvent, item: Favorite, kind: "essential" | "favorite") => void;
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
  const openFavorite = (favorite: Favorite) => {
    const tab = resolveFavoriteTab({ tabs: workspaceTabs }, favorite);
    tab ? actions.selectTab(tab.id) : actions.navigateActiveTab(favorite.url);
  };
  const showFavoritesFolder = filteredItems.favorites.length > 0 || !filteredItems.isFiltering;

  return (
    <>
      {filteredItems.essentials.length > 0 && (
        <section className="sidebar-section">
          <nav
            className="essentials"
            aria-label="Essentials"
            onDragEnter={(event) => {
              acceptSidebarTabFolderDrag(event, draggingTabId, "copy");
            }}
            onDragOver={(event) => {
              acceptSidebarTabFolderDrag(event, draggingTabId, "copy");
            }}
            onDrop={onEssentialDrop}
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
          onDragEnter={(event) => {
            acceptSidebarTabFolderDrag(event, draggingTabId);
          }}
          onDragOver={(event) => {
            acceptSidebarTabFolderDrag(event, draggingTabId);
          }}
          onDrop={onFavoriteDrop}
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
            {filteredItems.favorites.map((favorite) => {
              const tab = resolveTabBackedFavoriteTab({ tabs: workspaceTabs }, favorite);
              if (tab) {
                return (
                  <TabRow
                    key={favorite.id}
                    activeTabId={activeTab.id}
                    draggingTabId={draggingTabId}
                    faviconCache={faviconCache}
                    id={getSidebarSearchTargetElementId({ type: "favorite", id: favorite.id, title: favorite.title, url: favorite.url })}
                    isSearchSelected={activeSearchTarget?.type === "favorite" && activeSearchTarget.id === favorite.id}
                    labelKind="favorite tab"
                    splitTabIds={splitTabIds}
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
              }

              return (
                <FavoriteButton
                  key={favorite.id}
                  draggable
                  faviconCache={faviconCache}
                  draggingQuickEntryId={draggingFavoriteId}
                  favorite={favorite}
                  id={getSidebarSearchTargetElementId({ type: "favorite", id: favorite.id, title: favorite.title, url: favorite.url })}
                  isActive={isSidebarFavoriteActive(activeTab, favorite)}
                  isSearchSelected={activeSearchTarget?.type === "favorite" && activeSearchTarget.id === favorite.id}
                  kind="favorite"
                  onContextMenu={(event, item) => onQuickEntryContextMenu(event, item, "favorite")}
                  onDragStart={onFavoriteDragStart}
                  onDragEnd={() => setDraggingFavoriteId(null)}
                  onDrop={onFavoriteReorderDrop}
                  onOpen={() => openFavorite(favorite)}
                  onOpenInSplit={actions.openUrlInSplit}
                  onPreview={actions.openGlance}
                />
              );
            })}
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
