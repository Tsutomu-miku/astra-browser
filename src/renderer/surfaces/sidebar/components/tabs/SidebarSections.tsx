import type { DragEvent, MouseEvent } from "react";

import type { BrowserTab, Favorite } from "../../../../domain/browser";
import type { BrowserController } from "../../../../app/controller/types";
import { isSidebarUrlActive } from "../../model/sidebarItemState";
import type { SidebarFilterResult, SidebarSearchTarget } from "../../sidebarFiltering";
import { FavoriteButton, SidebarSectionHeader, TabGroupSection, TabRow } from "./SidebarItems";
import { SidebarPinnedTabs } from "./SidebarPinnedTabs";

export function SidebarSections({
  actions,
  activeSearchTarget,
  activeTab,
  draggingTabId,
  filteredItems,
  onTabContextMenu,
  onTabDrop,
  onQuickEntryContextMenu,
  setDraggingTabId,
  splitTabIds
}: {
  actions: BrowserController["actions"];
  activeSearchTarget?: SidebarSearchTarget;
  activeTab: BrowserTab;
  draggingTabId: string | null;
  filteredItems: SidebarFilterResult;
  onQuickEntryContextMenu: (event: MouseEvent, item: Favorite, kind: "essential" | "favorite") => void;
  onTabContextMenu: (event: MouseEvent, tab: BrowserTab) => void;
  onTabDrop: (event: DragEvent<HTMLElement>, targetTabId: string) => void;
  setDraggingTabId: (tabId: string | null) => void;
  splitTabIds: string[];
}) {
  const tabCount = filteredItems.groupedTabs.reduce((total, entry) => total + entry.tabs.length, 0) + filteredItems.regularTabs.length;

  return (
    <>
      {filteredItems.essentials.length > 0 && (
        <section className="sidebar-section">
          <SidebarSectionHeader count={filteredItems.essentials.length} title="Essentials" />
          <nav className="essentials" aria-label="Essentials">
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
          </nav>
        </section>
      )}

      <SidebarPinnedTabs
        actions={actions}
        activeSearchTarget={activeSearchTarget}
        activeTab={activeTab}
        draggingTabId={draggingTabId}
        pinnedTabs={filteredItems.pinnedTabs}
        splitTabIds={splitTabIds}
        onTabContextMenu={onTabContextMenu}
        onTabDrop={onTabDrop}
        setDraggingTabId={setDraggingTabId}
      />

      {filteredItems.favorites.length > 0 && (
        <section className="sidebar-section">
          <SidebarSectionHeader count={filteredItems.favorites.length} title="Favorites" />
          <nav className="favorites" aria-label="Favorites">
            {filteredItems.favorites.map((favorite) => (
              <FavoriteButton
                key={favorite.id}
                favorite={favorite}
                id={`sidebar-search-favorite-${favorite.id}`}
                isActive={isSidebarUrlActive(activeTab.url, favorite.url)}
                isSearchSelected={activeSearchTarget?.type === "favorite" && activeSearchTarget.id === favorite.id}
                onContextMenu={(event, item) => onQuickEntryContextMenu(event, item, "favorite")}
                onOpen={actions.openUrlInActiveWorkspace}
                onOpenInSplit={actions.openUrlInSplit}
                onPreview={actions.openGlance}
              />
            ))}
          </nav>
        </section>
      )}

      <section className="sidebar-section tabs-section">
        <SidebarSectionHeader count={tabCount} title="Tabs" />
        <nav className="tabs" aria-label="Tabs">
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
        </nav>
      </section>
    </>
  );
}
