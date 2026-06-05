import { useEffect, useRef, useState, type DragEvent, type KeyboardEvent, type MouseEvent } from "react";
import { FiX } from "react-icons/fi";

import { type DropAxis, type DropZonePlacement } from "../../../../common/drag-drop/dropPlacement";
import { writeSidebarTabDragPayload } from "../../../../common/drag-drop/sidebarDragPayload";
import { type BrowserTab, type Favorite } from "../../../../domain/browser";
import type { FaviconCache } from "../../../../domain/browser";
import { getQuickEntryAccessibilityLabel, type QuickEntryKind } from "../../model/quickEntryItemState";
import { readSidebarTabDragEventId } from "../../model/sidebarDragSources";
import { getSidebarTabAccessibilityLabel, getTabStatusBadges, type TabStatusBadge } from "../../model/sidebarItemState";
import { runSidebarItemKeyboardActivation, runSidebarItemPointerActivation } from "../../model/sidebarItemActivation";
import { openSidebarKeyboardContextMenu } from "../../model/sidebarKeyboardContextMenu";
import { acceptSidebarRowReorderDrag, acceptSidebarTabRowDrag, clearSidebarRowReorderDrop, resolveSidebarRowReorderDrop, resolveSidebarTabRowDrop } from "../../model/sidebarRowReorderDrop";
import { isCloseTabKey } from "../../model/sidebarTabKeyboard";
import { SidebarItemActionHints } from "../common/SidebarItemActionHints";
import { SidebarItemIcon } from "../common/SidebarItemIcon";
import { SidebarTabStatusBadges } from "../common/SidebarTabStatusBadges";

export type TabDropInfo = {
  draggedTabId: string;
  placement: DropZonePlacement;
  targetTabId: string;
};

export function TabRow({
  activeTabId,
  draggingTabId,
  faviconCache,
  id,
  labelKind = "tab",
  onClose,
  onContextMenu,
  onDrop,
  onGroupTab,
  onPreview,
  onRenameTab,
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
  onDrop: (event: DragEvent<HTMLElement>, targetTabId: string, axis?: DropAxis) => void;
  onGroupTab?: (sourceTabId: string, targetTabId: string) => void;
  onPreview: (url: string, title?: string) => void;
  onRenameTab?: (tabId: string, customTitle: string | undefined) => void;
  onSelect: (tabId: string) => void;
  onSplit: (tabId: string) => void;
  dropAxis?: DropAxis;
  setDraggingTabId: (tabId: string | null) => void;
  isSearchSelected?: boolean;
  splitTabIds: string[];
  tab: BrowserTab;
}) {
  const statusBadges = getTabStatusBadges(tab, splitTabIds, tab.id === activeTabId);
  const displayTitle = tab.customTitle ?? tab.title ?? tab.url;
  const tabLabel = getSidebarTabAccessibilityLabel({
    isActive: tab.id === activeTabId,
    kind: labelKind,
    statusBadges,
    tab: { ...tab, title: displayTitle }
  });
  const [isRenaming, setIsRenaming] = useState(false);
  const [renameDraft, setRenameDraft] = useState(displayTitle);
  const renameInputRef = useRef<HTMLInputElement | null>(null);
  useEffect(() => {
    if (isRenaming) {
      renameInputRef.current?.focus();
      renameInputRef.current?.select();
    }
  }, [isRenaming]);
  const commitRename = () => {
    if (!isRenaming) return;
    const trimmed = renameDraft.trim();
    onRenameTab?.(tab.id, trimmed ? trimmed : undefined);
    setIsRenaming(false);
  };
  const cancelRename = () => {
    if (!isRenaming) return;
    setRenameDraft(displayTitle);
    setIsRenaming(false);
  };
  const startTabDrag = (event: DragEvent<HTMLElement>) => {
    if (isRenaming) {
      event.preventDefault();
      return;
    }
    setDraggingTabId(tab.id);
    writeSidebarTabDragPayload(event.dataTransfer, tab.id);
    event.dataTransfer.effectAllowed = "move";
  };
  const iconStatus = tab.isLoading ? "loading" : tab.isSleeping ? "sleeping" : undefined;
  const readDragId = (event: DragEvent<HTMLElement>) => readSidebarTabDragEventId({ draggingTabId }, event.dataTransfer);

  return (
    <div
      className={`tab-row ${tab.isSleeping ? "is-sleeping" : ""} ${splitTabIds.includes(tab.id) ? "is-split-tab" : ""}`}
      id={id}
      aria-current={tab.id === activeTabId}
      aria-selected={isSearchSelected}
      draggable
      data-dragging={draggingTabId === tab.id}
      data-tab-id={tab.id}
      data-in-group={Boolean(tab.groupId)}
      onDragStart={startTabDrag}
      onDragEnd={() => {
        setDraggingTabId(null);
      }}
      onDragOver={(event) => {
        acceptSidebarTabRowDrag(event, {
          axis: dropAxis,
          readDragId,
          targetId: tab.id
        });
      }}
      onDragLeave={clearSidebarRowReorderDrop}
      onDrop={(event) => {
        const drop = resolveSidebarTabRowDrop(event, {
          axis: dropAxis,
          readDragId,
          targetId: tab.id
        });
        if (!drop) return;
        if (drop.placement === "onto" && onGroupTab) {
          event.preventDefault();
          event.stopPropagation();
          onGroupTab(drop.draggedId, tab.id);
          return;
        }
        onDrop(event, tab.id);
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
          {isRenaming ? (
            <input
              ref={renameInputRef}
              className="tab-title-input"
              type="text"
              value={renameDraft}
              onClick={(event) => event.stopPropagation()}
              onDoubleClick={(event) => event.stopPropagation()}
              onMouseDown={(event) => event.stopPropagation()}
              onDragStart={(event) => {
                event.preventDefault();
                event.stopPropagation();
              }}
              onChange={(event) => setRenameDraft(event.target.value)}
              onKeyDown={(event) => {
                if (event.key === "Enter") {
                  event.preventDefault();
                  event.stopPropagation();
                  commitRename();
                } else if (event.key === "Escape") {
                  event.preventDefault();
                  event.stopPropagation();
                  cancelRename();
                }
              }}
              onBlur={commitRename}
            />
          ) : (
            <span
              className={`tab-title ${tab.customTitle ? "is-custom-title" : ""}`}
              onDoubleClick={(event) => {
                if (!onRenameTab) return;
                event.preventDefault();
                event.stopPropagation();
                setRenameDraft(tab.customTitle ?? tab.title ?? tab.url);
                setIsRenaming(true);
              }}
            >
              {displayTitle}
            </span>
          )}
          <SidebarTabStatusBadges badges={statusBadges} />
        </span>
      </button>
      <span className="tab-row-actions">
        <SidebarItemActionHints />
        <button
          className="tab-close"
          type="button"
          aria-label={`Close ${displayTitle}`}
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
        acceptSidebarRowReorderDrag(event, {
          axis: dropAxis,
          readDragId: getDraggedQuickEntryId,
          targetId: favorite.id
        });
      }}
      onDragLeave={clearSidebarRowReorderDrop}
      onDrop={(event) => {
        if (resolveSidebarRowReorderDrop(event, {
          readDragId: getDraggedQuickEntryId,
          targetId: favorite.id
        })) onDrop?.(event, favorite.id, dropAxis);
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
