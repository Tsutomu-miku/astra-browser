import { type DragEvent, type KeyboardEvent, type MouseEvent } from "react";
import { FiX } from "react-icons/fi";

import { clearDropPlacement, updateDropPlacement, type DropAxis } from "../../../../common/drag-drop/dropPlacement";
import { writeSidebarTabDragPayload } from "../../../../common/drag-drop/sidebarDragPayload";
import { type BrowserTab, type Favorite } from "../../../../domain/browser";
import type { FaviconCache } from "../../../../domain/browser";
import { getQuickEntryAccessibilityLabel, type QuickEntryKind } from "../../model/quickEntryItemState";
import { readSidebarTabDragEventId } from "../../model/sidebarDragSources";
import { getSidebarTabAccessibilityLabel, getTabStatusBadges, type TabStatusBadge } from "../../model/sidebarItemState";
import { runSidebarItemKeyboardActivation, runSidebarItemPointerActivation } from "../../model/sidebarItemActivation";
import { openSidebarKeyboardContextMenu } from "../../model/sidebarKeyboardContextMenu";
import { isCloseTabKey } from "../../model/sidebarTabKeyboard";
import { SidebarItemActionHints } from "../common/SidebarItemActionHints";
import { SidebarItemIcon } from "../common/SidebarItemIcon";
import { SidebarTabStatusBadges } from "../common/SidebarTabStatusBadges";

export function TabRow({
  activeTabId,
  draggingTabId,
  faviconCache,
  id,
  labelKind = "tab",
  onClose,
  onContextMenu,
  onDrop,
  onPreview,
  onSelect,
  onSplit,
  dropAxis = "vertical",
  setDraggingTabId,
  isSearchSelected = false,
  splitTabIds,
  tab
}: {
  activeTabId: string;
  draggingTabId: string | null;
  faviconCache?: FaviconCache;
  id?: string;
  labelKind?: "favorite tab" | "pinned tab" | "tab";
  onClose: (tabId: string) => void;
  onContextMenu: (event: MouseEvent, tab: BrowserTab) => void;
  onDrop: (event: DragEvent<HTMLElement>, targetTabId: string) => void;
  onPreview: (url: string, title?: string) => void;
  onSelect: (tabId: string) => void;
  onSplit: (tabId: string) => void;
  dropAxis?: DropAxis;
  setDraggingTabId: (tabId: string | null) => void;
  isSearchSelected?: boolean;
  splitTabIds: string[];
  tab: BrowserTab;
}) {
  const statusBadges = getTabStatusBadges(tab, splitTabIds);
  const tabLabel = getSidebarTabAccessibilityLabel({
    isActive: tab.id === activeTabId,
    kind: labelKind,
    statusBadges,
    tab
  });
  const startTabDrag = (event: DragEvent<HTMLElement>) => {
    setDraggingTabId(tab.id);
    writeSidebarTabDragPayload(event.dataTransfer, tab.id);
    event.dataTransfer.effectAllowed = "move";
  };
  const iconStatus = tab.isLoading ? "loading" : tab.isSleeping ? "sleeping" : undefined;

  return (
    <div
      className={`tab-row ${tab.isSleeping ? "is-sleeping" : ""} ${splitTabIds.includes(tab.id) ? "is-split-tab" : ""}`}
      id={id}
      aria-current={tab.id === activeTabId}
      aria-selected={isSearchSelected}
      draggable
      data-dragging={draggingTabId === tab.id}
      data-tab-id={tab.id}
      onDragStart={startTabDrag}
      onDragEnd={() => {
        setDraggingTabId(null);
      }}
      onDragOver={(event) => {
        const draggedTabId = readSidebarTabDragEventId({ draggingTabId }, event.dataTransfer);
        if (draggedTabId && draggedTabId !== tab.id) {
          event.preventDefault();
          event.dataTransfer.dropEffect = "move";
          updateDropPlacement(event.currentTarget, event, dropAxis);
        }
      }}
      onDragLeave={(event) => clearDropPlacement(event.currentTarget)}
      onDrop={(event) => {
        clearDropPlacement(event.currentTarget);
        const draggedTabId = readSidebarTabDragEventId({ draggingTabId }, event.dataTransfer);
        if (draggedTabId && draggedTabId !== tab.id) onDrop(event, tab.id);
      }}
      onContextMenu={(event) => onContextMenu(event, tab)}
    >
      <button
        className="tab-button"
        type="button"
        aria-label={tabLabel}
        draggable={false}
        onAuxClick={(event) => {
          if (event.button === 1) {
            event.preventDefault();
            event.stopPropagation();
            onClose(tab.id);
          }
        }}
        onClick={(event) => {
          runSidebarItemPointerActivation(event, {
            primary: () => onSelect(tab.id),
            preview: () => onPreview(tab.url, tab.title),
            split: () => onSplit(tab.id)
          });
        }}
        onKeyDown={(event: KeyboardEvent<HTMLButtonElement>) => {
          if (openSidebarKeyboardContextMenu(event)) return;
          if (runSidebarItemKeyboardActivation(event, {
            primary: () => onSelect(tab.id),
            preview: () => onPreview(tab.url, tab.title),
            split: () => onSplit(tab.id)
          })) return;
          if (isCloseTabKey(event.key)) {
            event.preventDefault();
            event.stopPropagation();
            onClose(tab.id);
          }
        }}
      >
        <SidebarItemIcon className="tab-favicon" faviconCache={faviconCache} faviconUrl={tab.faviconUrl} status={iconStatus} url={tab.url} />
        <span className="tab-title-stack">
          <span className="tab-title">{tab.title || tab.url}</span>
          <SidebarTabStatusBadges badges={statusBadges} />
        </span>
      </button>
      <span className="tab-row-actions">
        <SidebarItemActionHints />
        <button
          className="tab-close"
          type="button"
          aria-label={`Close ${tab.title || tab.url}`}
          tabIndex={-1}
          draggable={false}
          onDragStart={(event) => {
            event.preventDefault();
            event.stopPropagation();
          }}
          onClick={(event) => {
            event.stopPropagation();
            onClose(tab.id);
          }}
        >
          <FiX />
        </button>
      </span>
    </div>
  );
}

export function FavoriteButton({
  draggable = false,
  draggingQuickEntryId = null,
  faviconCache,
  favorite,
  id,
  isActive = false,
  isSearchSelected = false,
  kind = "favorite",
  tabId,
  tabLabel,
  tabStatusBadges = [],
  onCloseTab,
  onDragEnd,
  onDragStart,
  onDrop,
  onOpen,
  onOpenInSplit,
  onOpenTabInSplit,
  onContextMenu,
  onPreview,
  dropAxis = "vertical"
}: {
  draggable?: boolean;
  draggingQuickEntryId?: string | null;
  faviconCache?: FaviconCache;
  favorite: Favorite;
  id?: string;
  isActive?: boolean;
  isSearchSelected?: boolean;
  kind?: QuickEntryKind;
  tabId?: string;
  tabLabel?: string;
  tabStatusBadges?: TabStatusBadge[];
  onCloseTab?: (tabId: string) => void;
  onContextMenu?: (event: MouseEvent, favorite: Favorite) => void;
  onDragEnd?: () => void;
  onDragStart?: (event: DragEvent<HTMLElement>, favoriteId: string) => void;
  onDrop?: (event: DragEvent<HTMLElement>, targetFavoriteId: string, axis: DropAxis) => void;
  onOpen: (url: string, title?: string) => void;
  onOpenInSplit: (url: string, title?: string) => void;
  onOpenTabInSplit?: (tabId: string) => void;
  onPreview: (url: string, title?: string) => void;
  dropAxis?: DropAxis;
}) {
  const getDraggedQuickEntryId = (event: DragEvent<HTMLElement>) => (
    draggingQuickEntryId || event.dataTransfer.getData(getQuickEntryDragDataKey(kind))
  );
  const isDragging = draggable && draggingQuickEntryId === favorite.id;
  const isDropTarget = draggable && Boolean(draggingQuickEntryId && draggingQuickEntryId !== favorite.id);
  const quickEntryLabel = getQuickEntryAccessibilityLabel({
    entry: favorite,
    isActive,
    isDragging,
    isDropTarget,
    isSearchSelected,
    kind
  });
  const openSplit = () => {
    if (tabId && onOpenTabInSplit) {
      onOpenTabInSplit(tabId);
      return;
    }

    onOpenInSplit(favorite.url, favorite.title);
  };

  return (
    <button
      className="favorite-button"
      id={id}
      type="button"
      aria-label={tabLabel ?? quickEntryLabel}
      aria-current={isActive}
      aria-selected={isSearchSelected}
      draggable={draggable}
      data-dragging={isDragging}
      data-drop-target={isDropTarget}
      onDragStart={draggable && onDragStart ? (event) => onDragStart(event, favorite.id) : undefined}
      onDragEnd={draggable ? onDragEnd : undefined}
      onAuxClick={(event) => {
        if (event.button === 1 && tabId && onCloseTab) {
          event.preventDefault();
          event.stopPropagation();
          onCloseTab(tabId);
        }
      }}
      onDragOver={(event) => {
        const draggedQuickEntryId = getDraggedQuickEntryId(event);
        if (draggedQuickEntryId && draggedQuickEntryId !== favorite.id) {
          event.preventDefault();
          event.dataTransfer.dropEffect = "move";
          updateDropPlacement(event.currentTarget, event, dropAxis);
        }
      }}
      onDragLeave={(event) => clearDropPlacement(event.currentTarget)}
      onDrop={(event) => {
        clearDropPlacement(event.currentTarget);
        const draggedQuickEntryId = getDraggedQuickEntryId(event);
        if (draggedQuickEntryId && draggedQuickEntryId !== favorite.id) onDrop?.(event, favorite.id, dropAxis);
      }}
      onContextMenu={onContextMenu ? (event) => onContextMenu(event, favorite) : undefined}
      onKeyDown={(event) => {
        if (onContextMenu && openSidebarKeyboardContextMenu(event)) return;
        if (runSidebarItemKeyboardActivation(event, {
          primary: () => onOpen(favorite.url, favorite.title),
          preview: () => onPreview(favorite.url, favorite.title),
          split: openSplit
        })) return;
        if (tabId && onCloseTab && isCloseTabKey(event.key)) {
          event.preventDefault();
          event.stopPropagation();
          onCloseTab(tabId);
        }
      }}
      onClick={(event) => {
        runSidebarItemPointerActivation(event, {
          primary: () => onOpen(favorite.url, favorite.title),
          preview: () => onPreview(favorite.url, favorite.title),
          split: openSplit
        });
      }}
    >
      <SidebarItemIcon className="favorite-icon" faviconCache={faviconCache} url={favorite.url} />
      {tabStatusBadges.length > 0 ? (
        <span className="favorite-title-stack">
          <span className="favorite-title">{favorite.title}</span>
          <SidebarTabStatusBadges badges={tabStatusBadges} />
        </span>
      ) : (
        <span className="favorite-title">{favorite.title}</span>
      )}
      <SidebarItemActionHints />
    </button>
  );
}

function getQuickEntryDragDataKey(kind: QuickEntryKind): "text/essential-id" | "text/favorite-id" {
  return kind === "essential" ? "text/essential-id" : "text/favorite-id";
}
