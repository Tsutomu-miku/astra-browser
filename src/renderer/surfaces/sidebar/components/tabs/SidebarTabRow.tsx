import { useEffect, useRef, useState, type DragEvent, type KeyboardEvent, type MouseEvent } from "react";
import { FiX } from "react-icons/fi";

import { type DropAxis, type DropZonePlacement } from "../../../../common/drag-drop/dropPlacement";
import { writeSidebarTabDragPayload } from "../../../../common/drag-drop/sidebarDragPayload";
import { type BrowserTab } from "../../../../domain/browser";
import type { FaviconCache } from "../../../../domain/browser";
import { readSidebarTabDragEventId } from "../../model/sidebarDragSources";
import { getSidebarTabAccessibilityLabel, getTabStatusBadges, type TabStatusBadge } from "../../model/sidebarItemState";
import { runSidebarItemKeyboardActivation, runSidebarItemPointerActivation } from "../../model/sidebarItemActivation";
import { openSidebarKeyboardContextMenu } from "../../model/sidebarKeyboardContextMenu";
import { acceptSidebarTabRowDrag, clearSidebarRowReorderDrop, resolveSidebarTabRowDrop } from "../../model/sidebarRowReorderDrop";
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
  tab,
  isCrossFolderDrag
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
  /** If true or returns true for the dragged tab, before/after drop indicators render as a full highlight instead of an underline. */
  isCrossFolderDrag?: boolean | ((draggedTabId: string) => boolean);
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
  const crossFolderFor = (draggedId: string) => {
    if (typeof isCrossFolderDrag === "boolean") return isCrossFolderDrag;
    if (typeof isCrossFolderDrag === "function") return isCrossFolderDrag(draggedId);
    return false;
  };

  return (
    <div
      className={`tab-row ${tab.isSleeping ? "is-sleeping" : ""} ${splitTabIds.includes(tab.id) ? "is-split-tab" : ""}`}
      id={id}
      aria-current={tab.id === activeTabId}
      aria-selected={isSearchSelected}
      draggable
      data-dragging={draggingTabId === tab.id}
      data-drop-target={draggingTabId !== null && draggingTabId !== tab.id}
      data-tab-id={tab.id}
      data-in-group={Boolean(tab.groupId)}
      onDragStart={startTabDrag}
      onDragEnd={() => {
        setDraggingTabId(null);
      }}
      onDragOver={(event) => {
        acceptSidebarTabRowDrag(event, {
          axis: dropAxis,
          crossFolder: crossFolderFor,
          readDragId,
          targetId: tab.id
        });
      }}
      onDragLeave={(event) => {
        // 只有当指针真正离开 tab row（而不是进入其内部子元素）时才清空指示器，
        // 避免子元素冒泡导致的 dragLeave 提前清空 placement，进而让 drop 事件错误
        // fallback 到 "onto" 产生意外的 group。
        const container = event.currentTarget;
        const next = event.relatedTarget as Node | null;
        if (next && container.contains(next)) return;
        clearSidebarRowReorderDrop(event);
      }}
      onDrop={(event) => {
        const drop = resolveSidebarTabRowDrop(event, {
          axis: dropAxis,
          readDragId,
          targetId: tab.id
        });
        clearSidebarRowReorderDrop(event);
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
