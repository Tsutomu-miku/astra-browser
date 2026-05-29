import { useState, type DragEvent, type MouseEvent } from "react";

import type { DropAxis } from "../../../../common/drag-drop/dropPlacement";
import { readSidebarTabDragPayload } from "../../../../common/drag-drop/sidebarDragPayload";
import type { BrowserTab, ClosedTab, Favorite, TabGroup } from "../../../../domain/browser";
import type { BrowserController } from "../../../../app/controller/types";
import { isSidebarFavoriteActive, isSidebarUrlActive } from "../../model/sidebarItemState";
import { hasSidebarSectionDragReveal, type SidebarSectionId } from "../../model/sidebarSectionState";
import { getSidebarSearchTargetElementId, type SidebarFilterResult, type SidebarSearchTarget } from "../../sidebarFiltering";
import { ClosedTabButton } from "./ClosedTabButton";
import { FavoriteButton, SidebarSectionHeader } from "./SidebarItems";
import { SidebarPinnedTabs } from "./SidebarPinnedTabs";
import { SidebarTabsSection } from "./SidebarTabsSection";

const SIDEBAR_RECENTLY_CLOSED_LIMIT = 4;

export function SidebarSections({
  actions,
  activeSearchTarget,
  activeTab,
  closedTabs,
  draggingClosedTabIndex = null,
  draggingEssentialId,
  draggingFavoriteId,
  draggingGroupId,
  draggingTabId,
  filteredItems,
  onEssentialDragStart,
  onEssentialDrop,
  onEssentialReorderDrop,
  onFavoriteDragStart,
  onFavoriteDrop,
  onFavoriteReorderDrop,
  onClosedTabContextMenu,
  onTabGroupContextMenu,
  onPinDrop,
  onTabContextMenu,
  onTabDrop,
  onTabPointerDrop = () => undefined,
  onTabsDrop = () => undefined,
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
  closedTabs: ClosedTab[];
  draggingClosedTabIndex?: number | null;
  draggingEssentialId: string | null;
  draggingFavoriteId: string | null;
  draggingGroupId: string | null;
  draggingTabId: string | null;
  filteredItems: SidebarFilterResult;
  onEssentialDragStart: (event: DragEvent<HTMLButtonElement>, essentialId: string) => void;
  onEssentialDrop: (event: DragEvent<HTMLElement>) => void;
  onEssentialReorderDrop: (event: DragEvent<HTMLElement>, targetEssentialId: string, axis: DropAxis) => void;
  onFavoriteDragStart: (event: DragEvent<HTMLButtonElement>, favoriteId: string) => void;
  onFavoriteDrop: (event: DragEvent<HTMLElement>) => void;
  onFavoriteReorderDrop: (event: DragEvent<HTMLElement>, targetFavoriteId: string, axis: DropAxis) => void;
  onClosedTabContextMenu: (event: MouseEvent, tab: ClosedTab, closedIndex: number) => void;
  onPinDrop: (event: DragEvent<HTMLElement>) => void;
  onQuickEntryContextMenu: (event: MouseEvent, item: Favorite, kind: "essential" | "favorite") => void;
  onTabContextMenu: (event: MouseEvent, tab: BrowserTab) => void;
  onTabGroupContextMenu: (event: MouseEvent, group: TabGroup) => void;
  onTabDrop: (event: DragEvent<HTMLElement>, targetTabId: string, axis?: DropAxis) => void;
  onTabPointerDrop?: (tabId: string, clientX: number, clientY: number) => void;
  onTabsDrop?: (event: DragEvent<HTMLElement>) => void;
  setDraggingEssentialId: (essentialId: string | null) => void;
  setDraggingFavoriteId: (favoriteId: string | null) => void;
  setDraggingClosedTabIndex?: (closedIndex: number | null) => void;
  setDraggingGroupId: (groupId: string | null) => void;
  setDraggingTabId: (tabId: string | null) => void;
  splitTabIds: string[];
  workspaceTabs?: BrowserTab[];
}) {
  const [collapsedSections, setCollapsedSections] = useState<Record<SidebarSectionId, boolean>>({
    essentials: false,
    favorites: false,
    pinned: false,
    recentlyClosed: false,
    tabs: false
  });
  const tabCount = filteredItems.groupedTabs.reduce((total, entry) => total + entry.tabs.length, 0) + filteredItems.regularTabs.length;
  const recentlyClosedTabs = closedTabs.slice(0, SIDEBAR_RECENTLY_CLOSED_LIMIT);
  const canUnpinDraggedTabToTabs = Boolean(
    draggingTabId && filteredItems.pinnedTabs.some((tab) => tab.id === draggingTabId)
  );
  const isSectionCollapsed = (sectionId: SidebarSectionId) => (
    !filteredItems.isFiltering &&
    !hasSidebarSectionDragReveal(sectionId, {
      essentialId: draggingEssentialId,
      favoriteId: draggingFavoriteId,
      tabId: draggingTabId
    }) &&
    collapsedSections[sectionId]
  );
  const toggleSection = (sectionId: SidebarSectionId) => {
    setCollapsedSections((current) => ({
      ...current,
      [sectionId]: !current[sectionId]
    }));
  };
  const openFavorite = (favorite: Favorite) => {
    const tab = workspaceTabs.find((candidate) => (
      candidate.id === favorite.tabId ||
      (!favorite.tabId && candidate.url === favorite.url)
    ));
    tab ? actions.selectTab(tab.id) : actions.openUrlInActiveWorkspace(favorite.url, favorite.title);
  };

  return (
    <>
      {(filteredItems.essentials.length > 0 || Boolean(draggingTabId)) && (
        <section className="sidebar-section">
          <SidebarSectionHeader
            count={filteredItems.essentials.length}
            dropLabel={draggingTabId ? "Drop to add" : undefined}
            isCollapsed={isSectionCollapsed("essentials")}
            title="Essentials"
            onToggle={() => toggleSection("essentials")}
          />
          {!isSectionCollapsed("essentials") && <nav
            className="essentials"
            aria-label="Essentials"
            data-drop-target={Boolean(draggingTabId)}
            onDragEnter={(event) => {
              if (draggingTabId || readSidebarTabDragPayload(event.dataTransfer)) {
                event.currentTarget.dataset.activeDropTarget = "true";
              }
            }}
            onDragOver={(event) => {
              if (draggingTabId || readSidebarTabDragPayload(event.dataTransfer)) {
                event.preventDefault();
                event.dataTransfer.dropEffect = "copy";
                event.currentTarget.dataset.activeDropTarget = "true";
              }
            }}
            onDragLeave={(event) => {
              delete event.currentTarget.dataset.activeDropTarget;
            }}
            onDrop={(event) => {
              delete event.currentTarget.dataset.activeDropTarget;
              onEssentialDrop(event);
            }}
          >
            {filteredItems.essentials.map((essential) => (
              <FavoriteButton
                key={essential.id}
                draggable
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
            {filteredItems.essentials.length === 0 && draggingTabId && (
              <p className="sidebar-drop-empty">Drop to essential</p>
            )}
          </nav>}
        </section>
      )}

      <SidebarPinnedTabs
        actions={actions}
        activeSearchTarget={activeSearchTarget}
        activeTab={activeTab}
        draggingTabId={draggingTabId}
        isCollapsed={isSectionCollapsed("pinned")}
        pinnedTabs={filteredItems.pinnedTabs}
        splitTabIds={splitTabIds}
        onTabContextMenu={onTabContextMenu}
        onTabDrop={onTabDrop}
        onPinDrop={onPinDrop}
        onToggle={() => toggleSection("pinned")}
        setDraggingTabId={setDraggingTabId}
      />

      {(filteredItems.favorites.length > 0 || Boolean(draggingTabId)) && (
        <section className="sidebar-section">
          <SidebarSectionHeader
            count={filteredItems.favorites.length}
            dropLabel={draggingTabId ? "Drop to add" : undefined}
            isCollapsed={isSectionCollapsed("favorites")}
            title="Favorites"
            onToggle={() => toggleSection("favorites")}
          />
          {!isSectionCollapsed("favorites") && <nav
            className="favorites"
            aria-label="Favorites"
            data-drop-target={Boolean(draggingTabId)}
            onDragEnter={(event) => {
              if (draggingTabId || readSidebarTabDragPayload(event.dataTransfer)) {
                event.currentTarget.dataset.activeDropTarget = "true";
              }
            }}
            onDragOver={(event) => {
              if (draggingTabId || readSidebarTabDragPayload(event.dataTransfer)) {
                event.preventDefault();
                event.dataTransfer.dropEffect = "copy";
                event.currentTarget.dataset.activeDropTarget = "true";
              }
            }}
            onDragLeave={(event) => {
              delete event.currentTarget.dataset.activeDropTarget;
            }}
            onDrop={(event) => {
              delete event.currentTarget.dataset.activeDropTarget;
              onFavoriteDrop(event);
            }}
          >
            {filteredItems.favorites.map((favorite) => (
              <FavoriteButton
                key={favorite.id}
                draggable
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
            ))}
            {filteredItems.favorites.length === 0 && draggingTabId && (
              <p className="sidebar-drop-empty">Drop to favorite</p>
            )}
          </nav>}
        </section>
      )}

      {!filteredItems.isFiltering && recentlyClosedTabs.length > 0 && (
        <section className="sidebar-section">
          <SidebarSectionHeader
            count={recentlyClosedTabs.length}
            isCollapsed={isSectionCollapsed("recentlyClosed")}
            title="Recently Closed"
            onToggle={() => toggleSection("recentlyClosed")}
          />
          {!isSectionCollapsed("recentlyClosed") && <nav className="recently-closed-tabs" aria-label="Recently closed tabs">
            {recentlyClosedTabs.map((tab, index) => (
              <ClosedTabButton
                key={`${tab.url}-${tab.closedAt}`}
                closedIndex={index}
                draggingClosedTabIndex={draggingClosedTabIndex}
                tab={tab}
                onContextMenu={onClosedTabContextMenu}
                onDragEnd={() => setDraggingClosedTabIndex(null)}
                onDragStart={(event) => {
                  setDraggingClosedTabIndex(index);
                  event.dataTransfer.effectAllowed = "move";
                  event.dataTransfer.setData("text/closed-tab-index", String(index));
                }}
                onOpenInSplit={actions.openUrlInSplit}
                onPreview={actions.openGlance}
                onRestore={actions.restoreClosedTab}
              />
            ))}
          </nav>}
        </section>
      )}

      <SidebarTabsSection
        actions={actions}
        activeSearchTarget={activeSearchTarget}
        activeTab={activeTab}
        canUnpinDraggedTabToTabs={canUnpinDraggedTabToTabs}
        draggingGroupId={draggingGroupId}
        draggingTabId={draggingTabId}
        filteredItems={filteredItems}
        isCollapsed={isSectionCollapsed("tabs")}
        splitTabIds={splitTabIds}
        tabCount={tabCount}
        onTabContextMenu={onTabContextMenu}
        onTabDrop={onTabDrop}
        onTabPointerDrop={onTabPointerDrop}
        onTabGroupContextMenu={onTabGroupContextMenu}
        onTabsDrop={onTabsDrop}
        onToggle={() => toggleSection("tabs")}
        setDraggingGroupId={setDraggingGroupId}
        setDraggingTabId={setDraggingTabId}
      />
    </>
  );
}
