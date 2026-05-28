import type { DragEvent, KeyboardEvent, MouseEvent } from "react";
import { FiChevronDown, FiChevronRight, FiLoader, FiMoon, FiX } from "react-icons/fi";

import { getHostInitial, type BrowserTab, type Favorite } from "../../../../domain/browser";
import { getTabStatusBadges } from "../../model/sidebarItemState";
import { isCloseTabKey } from "../../model/sidebarTabKeyboard";
import { SidebarItemActionHints } from "./SidebarItemActionHints";
import { SidebarTabStatusBadges } from "./SidebarTabStatusBadges";

export function SidebarSectionHeader({
  count,
  isCollapsed = false,
  onToggle,
  title
}: {
  count: number;
  isCollapsed?: boolean;
  onToggle?: () => void;
  title: string;
}) {
  const content = (
    <>
      <span className="sidebar-section-title">
        {onToggle && (isCollapsed ? <FiChevronRight /> : <FiChevronDown />)}
        <span>{title}</span>
      </span>
      <span className="sidebar-section-count">{count}</span>
    </>
  );

  return (
    <header className="sidebar-section-header">
      {onToggle ? (
        <button
          className="sidebar-section-header-button"
          type="button"
          aria-expanded={!isCollapsed}
          onClick={onToggle}
        >
          {content}
        </button>
      ) : content}
    </header>
  );
}

export function TabRow({
  activeTabId,
  draggingTabId,
  onClose,
  onContextMenu,
  onDrop,
  onPreview,
  onSelect,
  onSplit,
  setDraggingTabId,
  isSearchSelected = false,
  splitTabIds,
  tab
}: {
  activeTabId: string;
  draggingTabId: string | null;
  onClose: (tabId: string) => void;
  onContextMenu: (event: MouseEvent, tab: BrowserTab) => void;
  onDrop: (event: DragEvent<HTMLElement>, targetTabId: string) => void;
  onPreview: (url: string, title?: string) => void;
  onSelect: (tabId: string) => void;
  onSplit: (tabId: string) => void;
  setDraggingTabId: (tabId: string | null) => void;
  isSearchSelected?: boolean;
  splitTabIds: string[];
  tab: BrowserTab;
}) {
  const statusBadges = getTabStatusBadges(tab, splitTabIds);

  return (
    <div
      className={`tab-row ${tab.isSleeping ? "is-sleeping" : ""} ${splitTabIds.includes(tab.id) ? "is-split-tab" : ""}`}
      aria-current={tab.id === activeTabId}
      aria-selected={isSearchSelected}
      draggable
      data-dragging={draggingTabId === tab.id}
      onDragStart={(event) => {
        setDraggingTabId(tab.id);
        event.dataTransfer.effectAllowed = "move";
        event.dataTransfer.setData("text/plain", tab.id);
      }}
      onDragEnd={() => setDraggingTabId(null)}
      onDragOver={(event) => {
        if (draggingTabId && draggingTabId !== tab.id) event.preventDefault();
      }}
      onDrop={(event) => onDrop(event, tab.id)}
      onContextMenu={(event) => onContextMenu(event, tab)}
    >
      <button
        className="tab-button"
        type="button"
        onAuxClick={(event) => {
          if (event.button === 1) {
            event.preventDefault();
            onClose(tab.id);
          }
        }}
        onClick={(event) => {
          if (event.altKey) {
            onPreview(tab.url, tab.title);
          } else if (event.shiftKey) {
            onSplit(tab.id);
          } else {
            onSelect(tab.id);
          }
        }}
        onKeyDown={(event: KeyboardEvent<HTMLButtonElement>) => {
          if (isCloseTabKey(event.key)) {
            event.preventDefault();
            onClose(tab.id);
          }
        }}
      >
        <span className="tab-favicon">{tab.isSleeping ? <FiMoon /> : tab.isLoading ? <FiLoader /> : getHostInitial(tab.url)}</span>
        <span className="tab-title-stack">
          <span className="tab-title">{tab.title || tab.url}</span>
          <SidebarTabStatusBadges badges={statusBadges} />
        </span>
        <SidebarItemActionHints />
      </button>
      <button
        className="tab-close"
        type="button"
        title="Close tab"
        aria-label={`Close ${tab.title || tab.url}`}
        onClick={() => onClose(tab.id)}
      >
        <FiX />
      </button>
    </div>
  );
}

export function FavoriteButton({
  draggable = false,
  draggingQuickEntryId = null,
  favorite,
  id,
  isActive = false,
  isSearchSelected = false,
  onDragEnd,
  onDragStart,
  onDrop,
  onOpen,
  onOpenInSplit,
  onContextMenu,
  onPreview
}: {
  draggable?: boolean;
  draggingQuickEntryId?: string | null;
  favorite: Favorite;
  id?: string;
  isActive?: boolean;
  isSearchSelected?: boolean;
  onContextMenu?: (event: MouseEvent, favorite: Favorite) => void;
  onDragEnd?: () => void;
  onDragStart?: (event: DragEvent<HTMLButtonElement>, favoriteId: string) => void;
  onDrop?: (event: DragEvent<HTMLElement>, targetFavoriteId: string) => void;
  onOpen: (url: string, title?: string) => void;
  onOpenInSplit: (url: string, title?: string) => void;
  onPreview: (url: string, title?: string) => void;
}) {
  return (
    <button
      className="favorite-button"
      id={id}
      type="button"
      title={favorite.url}
      aria-current={isActive}
      aria-selected={isSearchSelected}
      draggable={draggable}
      data-dragging={draggable && draggingQuickEntryId === favorite.id}
      data-drop-target={draggable && Boolean(draggingQuickEntryId && draggingQuickEntryId !== favorite.id)}
      onDragStart={draggable && onDragStart ? (event) => onDragStart(event, favorite.id) : undefined}
      onDragEnd={draggable ? onDragEnd : undefined}
      onDragOver={(event) => {
        if (draggingQuickEntryId && draggingQuickEntryId !== favorite.id) {
          event.preventDefault();
          event.dataTransfer.dropEffect = "move";
        }
      }}
      onDrop={(event) => {
        if (draggingQuickEntryId && draggingQuickEntryId !== favorite.id) onDrop?.(event, favorite.id);
      }}
      onContextMenu={onContextMenu ? (event) => onContextMenu(event, favorite) : undefined}
      onClick={(event) => {
        if (event.altKey) {
          onPreview(favorite.url, favorite.title);
        } else if (event.shiftKey) {
          onOpenInSplit(favorite.url, favorite.title);
        } else {
          onOpen(favorite.url, favorite.title);
        }
      }}
    >
      <span className="favorite-icon">{getHostInitial(favorite.url)}</span>
      <span className="favorite-title">{favorite.title}</span>
      <SidebarItemActionHints />
    </button>
  );
}
