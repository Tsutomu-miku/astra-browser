import { useState, type DragEvent, type MouseEvent } from "react";

import type { BrowserTab, ClosedTab, Favorite } from "../../../../domain/browser";
import type { BrowserController } from "../../../../app/controller/types";
import { isSidebarUrlActive } from "../../model/sidebarItemState";
import type { SidebarFilterResult, SidebarSearchTarget } from "../../sidebarFiltering";
import { ClosedTabButton } from "./ClosedTabButton";
import { FavoriteButton, SidebarSectionHeader, TabGroupSection, TabRow } from "./SidebarItems";
import { SidebarPinnedTabs } from "./SidebarPinnedTabs";

const SIDEBAR_RECENTLY_CLOSED_LIMIT = 4;

type SidebarSectionId = "essentials" | "favorites" | "pinned" | "recentlyClosed" | "tabs";

export function SidebarSections({
  actions,
  activeSearchTarget,
  activeTab,
  closedTabs,
  draggingFavoriteId,
  draggingTabId,
  filteredItems,
  onFavoriteDragStart,
  onFavoriteDrop,
  onFavoriteReorderDrop,
  onPinDrop,
  onTabContextMenu,
  onTabDrop,
  onQuickEntryContextMenu,
  setDraggingFavoriteId,
  setDraggingTabId,
  splitTabIds
}: {
  actions: BrowserController["actions"];
  activeSearchTarget?: SidebarSearchTarget;
  activeTab: BrowserTab;
  closedTabs: ClosedTab[];
  draggingFavoriteId: string | null;
  draggingTabId: string | null;
  filteredItems: SidebarFilterResult;
  onFavoriteDragStart: (event: DragEvent<HTMLButtonElement>, favoriteId: string) => void;
  onFavoriteDrop: (event: DragEvent<HTMLElement>) => void;
  onFavoriteReorderDrop: (event: DragEvent<HTMLElement>, targetFavoriteId: string) => void;
  onPinDrop: (event: DragEvent<HTMLElement>) => void;
  onQuickEntryContextMenu: (event: MouseEvent, item: Favorite, kind: "essential" | "favorite") => void;
  onTabContextMenu: (event: MouseEvent, tab: BrowserTab) => void;
  onTabDrop: (event: DragEvent<HTMLElement>, targetTabId: string) => void;
  setDraggingFavoriteId: (favoriteId: string | null) => void;
  setDraggingTabId: (tabId: string | null) => void;
  splitTabIds: string[];
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
  const isSectionCollapsed = (sectionId: SidebarSectionId) => !filteredItems.isFiltering && collapsedSections[sectionId];
  const toggleSection = (sectionId: SidebarSectionId) => {
    setCollapsedSections((current) => ({
      ...current,
      [sectionId]: !current[sectionId]
    }));
  };

  return (
    <>
      {filteredItems.essentials.length > 0 && (
        <section className="sidebar-section">
          <SidebarSectionHeader
            count={filteredItems.essentials.length}
            isCollapsed={isSectionCollapsed("essentials")}
            title="Essentials"
            onToggle={() => toggleSection("essentials")}
          />
          {!isSectionCollapsed("essentials") && <nav className="essentials" aria-label="Essentials">
            {filteredItems.essentials.map((essential) => (
              <FavoriteButton
                key={essential.id}
                favorite={essential}
                id={`sidebar-search-essential-${essential.id}`}
                isActive={isSidebarUrlActive(activeTab.url, essential.url)}
                isSearchSelected={activeSearchTarget?.type === "essential" && activeSearchTarget.id === essential.id}
                onContextMenu={(event, item) => onQuickEntryContextMenu(event, item, "essential")}
                onOpen={actions.openUrlInActiveWorkspace}
                onOpenInSplit={actions.openUrlInSplit}
                onPreview={actions.openGlance}
              />
            ))}
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
            isCollapsed={isSectionCollapsed("favorites")}
            title="Favorites"
            onToggle={() => toggleSection("favorites")}
          />
          {!isSectionCollapsed("favorites") && <nav
            className="favorites"
            aria-label="Favorites"
            data-drop-target={Boolean(draggingTabId)}
            onDragOver={(event) => {
              if (draggingTabId) {
                event.preventDefault();
                event.dataTransfer.dropEffect = "copy";
              }
            }}
            onDrop={onFavoriteDrop}
          >
            {filteredItems.favorites.map((favorite) => (
              <FavoriteButton
                key={favorite.id}
                draggable
                draggingFavoriteId={draggingFavoriteId}
                favorite={favorite}
                id={`sidebar-search-favorite-${favorite.id}`}
                isActive={isSidebarUrlActive(activeTab.url, favorite.url)}
                isSearchSelected={activeSearchTarget?.type === "favorite" && activeSearchTarget.id === favorite.id}
                onContextMenu={(event, item) => onQuickEntryContextMenu(event, item, "favorite")}
                onDragStart={onFavoriteDragStart}
                onDragEnd={() => setDraggingFavoriteId(null)}
                onDrop={onFavoriteReorderDrop}
                onOpen={actions.openUrlInActiveWorkspace}
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
                tab={tab}
                onOpenInSplit={actions.openUrlInSplit}
                onPreview={actions.openGlance}
                onRestore={actions.restoreClosedTab}
              />
            ))}
          </nav>}
        </section>
      )}

      <section className="sidebar-section tabs-section">
        <SidebarSectionHeader
          count={tabCount}
          isCollapsed={isSectionCollapsed("tabs")}
          title="Tabs"
          onToggle={() => toggleSection("tabs")}
        />
        {!isSectionCollapsed("tabs") && <nav className="tabs" aria-label="Tabs">
          {filteredItems.groupedTabs.map(({ group, tabs }) => (
            <TabGroupSection
              key={group.id}
              activeTab={activeTab}
              group={group}
              searchSelectedTabId={activeSearchTarget?.type === "tab" ? activeSearchTarget.id : undefined}
              splitTabIds={splitTabIds}
              tabs={tabs}
              draggingTabId={draggingTabId}
              onAssignTab={actions.assignTabToGroup}
              onClose={actions.closeTab}
              onContextMenu={onTabContextMenu}
              onDrop={onTabDrop}
              onPreview={actions.openGlance}
              onSelect={actions.selectTab}
              onSplit={actions.openTabInSplit}
              onToggle={() => actions.toggleTabGroupCollapsed(group.id)}
              onUpdate={actions.updateTabGroup}
              setDraggingTabId={setDraggingTabId}
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
    </>
  );
}
