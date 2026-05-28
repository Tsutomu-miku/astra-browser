import type { DragEvent, MouseEvent } from "react";
import { FiLoader } from "react-icons/fi";

import { getHostInitial, type BrowserTab } from "../../../../domain/browser";
import type { BrowserController } from "../../../../app/controller/types";
import { getTabStatusBadges } from "../../model/sidebarItemState";
import type { SidebarSearchTarget } from "../../sidebarFiltering";
import { SidebarItemActionHints } from "./SidebarItemActionHints";
import { SidebarSectionHeader } from "./SidebarItems";
import { SidebarTabStatusBadges } from "./SidebarTabStatusBadges";

export function SidebarPinnedTabs({
  actions,
  activeSearchTarget,
  activeTab,
  draggingTabId,
  isCollapsed = false,
  onTabContextMenu,
  onTabDrop,
  onPinDrop,
  onToggle,
  pinnedTabs,
  setDraggingTabId,
  splitTabIds
}: {
  actions: BrowserController["actions"];
  activeSearchTarget?: SidebarSearchTarget;
  activeTab: BrowserTab;
  draggingTabId: string | null;
  isCollapsed?: boolean;
  onTabContextMenu: (event: MouseEvent, tab: BrowserTab) => void;
  onTabDrop: (event: DragEvent<HTMLElement>, targetTabId: string) => void;
  onPinDrop: (event: DragEvent<HTMLElement>) => void;
  onToggle?: () => void;
  pinnedTabs: BrowserTab[];
  setDraggingTabId: (tabId: string | null) => void;
  splitTabIds: string[];
}) {
  if (pinnedTabs.length === 0 && !draggingTabId) return null;

  return (
    <section className="sidebar-section">
      <SidebarSectionHeader count={pinnedTabs.length} isCollapsed={isCollapsed} title="Pinned" onToggle={onToggle} />
      {!isCollapsed && <nav
        className="pinned-tabs"
        aria-label="Pinned tabs"
        data-drop-target={Boolean(draggingTabId)}
        onDragOver={(event) => {
          if (draggingTabId) {
            event.preventDefault();
            event.dataTransfer.dropEffect = "copy";
          }
        }}
        onDrop={onPinDrop}
      >
        {pinnedTabs.map((tab) => {
          const statusBadges = getTabStatusBadges(tab, splitTabIds);

          return (
            <button
              className="pinned-tab-button"
              key={tab.id}
              id={`sidebar-search-tab-${tab.id}`}
              title={tab.title || tab.url}
              type="button"
              aria-current={tab.id === activeTab.id}
              aria-selected={activeSearchTarget?.type === "tab" && activeSearchTarget.id === tab.id}
              draggable
              data-dragging={draggingTabId === tab.id}
              data-drop-target={Boolean(draggingTabId && draggingTabId !== tab.id)}
              onAuxClick={(event) => {
                if (event.button === 1) {
                  event.preventDefault();
                  actions.closeTab(tab.id);
                }
              }}
              onClick={(event) => {
                if (event.altKey) {
                  actions.openGlance(tab.url, tab.title);
                } else if (event.shiftKey) {
                  actions.openTabInSplit(tab.id);
                } else {
                  actions.selectTab(tab.id);
                }
              }}
              onContextMenu={(event) => onTabContextMenu(event, tab)}
              onDragStart={(event) => {
                setDraggingTabId(tab.id);
                event.dataTransfer.effectAllowed = "move";
                event.dataTransfer.setData("text/plain", tab.id);
              }}
              onDragEnd={() => setDraggingTabId(null)}
              onDragOver={(event) => {
                if (draggingTabId && draggingTabId !== tab.id) {
                  event.preventDefault();
                  event.dataTransfer.dropEffect = "move";
                }
              }}
              onDrop={(event) => onTabDrop(event, tab.id)}
            >
              <span className="pinned-tab-icon">{tab.isLoading ? <FiLoader /> : getHostInitial(tab.url)}</span>
              <SidebarTabStatusBadges badges={statusBadges} variant="pinned" />
              <SidebarItemActionHints />
            </button>
          );
        })}
        {pinnedTabs.length === 0 && draggingTabId && (
          <p className="sidebar-drop-empty">Drop to pin</p>
        )}
      </nav>}
    </section>
  );
}
