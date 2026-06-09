import { type DragEvent, type MouseEvent } from "react";

import { type DropAxis } from "../../../../common/drag-drop/dropPlacement";
import { type BrowserTab, type Favorite } from "../../../../domain/browser";
import type { FaviconCache } from "../../../../domain/browser";
import { getQuickEntryAccessibilityLabel, type QuickEntryKind } from "../../model/quickEntryItemState";
import { readSidebarTabDragEventId } from "../../model/sidebarDragSources";
import { type TabStatusBadge } from "../../model/sidebarItemState";
import { runSidebarItemKeyboardActivation, runSidebarItemPointerActivation } from "../../model/sidebarItemActivation";
import { openSidebarKeyboardContextMenu } from "../../model/sidebarKeyboardContextMenu";
import { acceptSidebarRowReorderDrag, clearSidebarRowReorderDrop, resolveSidebarRowReorderDrop } from "../../model/sidebarRowReorderDrop";
import { isCloseTabKey } from "../../model/sidebarTabKeyboard";
import { SidebarItemActionHints } from "../common/SidebarItemActionHints";
import { SidebarItemIcon } from "../common/SidebarItemIcon";
import { SidebarTabStatusBadges } from "../common/SidebarTabStatusBadges";

export function FavoriteButton({
  draggable = false,
  draggingQuickEntryId = null,
  draggingTabId = null,
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
  onTabDrop,
  dropAxis = "vertical"
}: {
  draggable?: boolean;
  draggingQuickEntryId?: string | null;
  draggingTabId?: string | null;
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
  onTabDrop?: (event: DragEvent<HTMLElement>, targetFavoriteId: string, axis: DropAxis) => void;
  dropAxis?: DropAxis;
}) {
  const getDraggedQuickEntryId = (event: DragEvent<HTMLElement>) => (
    draggingQuickEntryId || event.dataTransfer.getData(getQuickEntryDragDataKey(kind))
  );
  const getDraggedTabId = (event: DragEvent<HTMLElement>) => (
    draggingTabId || readSidebarTabDragEventId({ draggingTabId }, event.dataTransfer)
  );
  const readAnyDragId = (event: DragEvent<HTMLElement>) => (
    getDraggedQuickEntryId(event) || getDraggedTabId(event)
  );
  const isDragging = draggable && draggingQuickEntryId === favorite.id;
  const isDropTarget = draggable && Boolean(
    (draggingQuickEntryId && draggingQuickEntryId !== favorite.id) ||
    (draggingTabId && draggingTabId !== tabId)
  );
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
        acceptSidebarRowReorderDrag(event, {
          axis: dropAxis,
          readDragId: readAnyDragId,
          targetId: favorite.id
        });
      }}
      onDragLeave={clearSidebarRowReorderDrop}
      onDrop={(event) => {
        const quickEntryId = resolveSidebarRowReorderDrop(event, {
          readDragId: getDraggedQuickEntryId,
          targetId: favorite.id
        });
        if (quickEntryId) {
          onDrop?.(event, favorite.id, dropAxis);
          return;
        }

        const droppedTabId = getDraggedTabId(event);
        if (droppedTabId && droppedTabId !== tabId) {
          event.preventDefault();
          event.stopPropagation();
          onTabDrop?.(event, favorite.id, dropAxis);
        }
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

// Re-export TabRow type helpers that some callers import from this file.
export type { BrowserTab, Favorite };
