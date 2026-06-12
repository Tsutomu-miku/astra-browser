import { useEffect, useRef, useState, type DragEvent, type KeyboardEvent, type MouseEvent } from "react";
import { FiX } from "react-icons/fi";

import { clearDropTargetActive, type DropAxis, type DropZonePlacement } from "../../../../common/drag-drop/dropPlacement";
import { writeSidebarTabDragPayload } from "../../../../common/drag-drop/sidebarDragPayload";
import { type BrowserTab } from "../../../../domain/browser";
import type { FaviconCache } from "../../../../domain/browser";
import { readSidebarTabDragEventId } from "../../model/sidebarDragSources";
import { runSidebarItemKeyboardActivation, runSidebarItemPointerActivation } from "../../model/sidebarItemActivation";
import { openSidebarKeyboardContextMenu } from "../../model/sidebarKeyboardContextMenu";
import { acceptSidebarTabRowDrag, clearSidebarRowReorderDrop, resolveSidebarTabRowDrop } from "../../model/sidebarRowReorderDrop";
import { isCloseTabKey } from "../../model/sidebarTabKeyboard";
import { SidebarItemIcon } from "../common/SidebarItemIcon";

export type SplitTabDropInfo = {
  draggedTabId: string;
  placement: DropZonePlacement;
  targetTabId: string;
};

/**
 * Arc-style split tab row — two tabs displayed side-by-side in the content area
 * but represented as a single row in the sidebar. Shows two favicons and the
 * primary tab's title.
 */
export function SplitTabRow({
  activeTabId,
  draggingTabId,
  faviconCache,
  id,
  isActive = false,
  onClose,
  onContextMenu,
  onDrop,
  onPreview,
  onSelect,
  onSwapPanes,
  dropAxis = "vertical",
  primaryTab,
  secondaryTab,
  setDraggingTabId,
  isSearchSelected = false
}: {
  activeTabId: string;
  draggingTabId: string | null;
  faviconCache?: FaviconCache;
  id?: string;
  /** Whether the split view itself is active (showing in content area). */
  isActive?: boolean;
  onClose: (tabId: string) => void;
  onContextMenu: (event: MouseEvent, tab: BrowserTab) => void;
  onDrop: (event: DragEvent<HTMLElement>, targetTabId: string, axis?: DropAxis) => void;
  onPreview: (url: string, title?: string) => void;
  onSelect: (tabId: string) => void;
  onSwapPanes: () => void;
  dropAxis?: DropAxis;
  primaryTab: BrowserTab;
  secondaryTab: BrowserTab;
  setDraggingTabId: (tabId: string | null) => void;
  isSearchSelected?: boolean;
}) {
  const displayTitle = primaryTab.customTitle ?? primaryTab.title ?? primaryTab.url;
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
    // Renaming a split tab renames the primary tab
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
    // Dragging a split tab drags the primary tab for reordering
    setDraggingTabId(primaryTab.id);
    writeSidebarTabDragPayload(event.dataTransfer, primaryTab.id);
    event.dataTransfer.effectAllowed = "move";
  };

  const startSecondaryDrag = (event: DragEvent<HTMLElement>) => {
    if (isRenaming) {
      event.preventDefault();
      return;
    }
    // Dragging the secondary favicon tears it out of the split
    event.stopPropagation();
    setDraggingTabId(secondaryTab.id);
    writeSidebarTabDragPayload(event.dataTransfer, secondaryTab.id);
    event.dataTransfer.effectAllowed = "move";
  };

  const primaryIconStatus = primaryTab.isLoading ? "loading" : primaryTab.isSleeping ? "sleeping" : undefined;
  const secondaryIconStatus = secondaryTab.isLoading ? "loading" : secondaryTab.isSleeping ? "sleeping" : undefined;
  const readDragId = (event: DragEvent<HTMLElement>) => readSidebarTabDragEventId({ draggingTabId }, event.dataTransfer);

  return (
    <div
      className="tab-row split-tab-row"
      id={id}
      aria-current={isActive}
      aria-selected={isSearchSelected}
      draggable
      data-dragging={draggingTabId === primaryTab.id}
      data-drop-target={draggingTabId !== null && draggingTabId !== primaryTab.id && draggingTabId !== secondaryTab.id}
      data-tab-id={primaryTab.id}
      onDragStart={startTabDrag}
      onDragEnd={() => {
        setDraggingTabId(null);
      }}
      onDragOver={(event) => {
        const result = acceptSidebarTabRowDrag(event, {
          axis: dropAxis,
          crossFolder: () => false,
          readDragId,
          targetId: primaryTab.id
        });
        if (result) {
          const ancestor = event.currentTarget.closest(".sidebar-section, .essentials") as HTMLElement | null;
          if (ancestor) clearDropTargetActive(ancestor);
          event.stopPropagation();
        }
      }}
      onDragLeave={(event) => {
        const container = event.currentTarget;
        const next = event.relatedTarget as Node | null;
        if (next && container.contains(next)) return;
        clearSidebarRowReorderDrop(event);
      }}
      onDrop={(event) => {
        const drop = resolveSidebarTabRowDrop(event, {
          axis: dropAxis,
          readDragId,
          targetId: primaryTab.id
        });
        clearSidebarRowReorderDrop(event);
        if (!drop) return;
        // Dropping onto a split tab: replace the secondary pane
        onDrop(event, primaryTab.id);
      }}
      onContextMenu={(event) => onContextMenu(event, primaryTab)}
    >
      <button
        className="tab-button split-tab-button"
        type="button"
        aria-label={`${displayTitle}, split view, ${primaryTab.title} and ${secondaryTab.title}`}
        draggable={false}
        onClick={(event) => {
          runSidebarItemPointerActivation(event, {
            primary: () => onSelect(primaryTab.id),
            preview: () => onPreview(primaryTab.url, primaryTab.title),
            split: () => onSwapPanes()
          });
        }}
        onKeyDown={(event: KeyboardEvent<HTMLButtonElement>) => {
          if (openSidebarKeyboardContextMenu(event)) return;
          if (runSidebarItemKeyboardActivation(event, {
            primary: () => onSelect(primaryTab.id),
            preview: () => onPreview(primaryTab.url, primaryTab.title),
            split: () => onSwapPanes()
          })) return;
          if (isCloseTabKey(event.key)) {
            event.preventDefault();
            event.stopPropagation();
            // Closing a split tab: close the secondary pane, keep primary
            onClose(secondaryTab.id);
          }
        }}
      >
        <span className="split-tab-favicons">
          <SidebarItemIcon
            className="split-tab-favicon split-tab-favicon-primary"
            faviconCache={faviconCache}
            faviconUrl={primaryTab.faviconUrl}
            status={primaryIconStatus}
            url={primaryTab.url}
          />
          <span
            className="split-tab-favicon split-tab-favicon-secondary"
            draggable
            title={`Drag to tear off ${secondaryTab.title}`}
            onDragStart={startSecondaryDrag}
          >
            <SidebarItemIcon
              className="split-tab-favicon-icon"
              faviconCache={faviconCache}
              faviconUrl={secondaryTab.faviconUrl}
              status={secondaryIconStatus}
              url={secondaryTab.url}
            />
          </span>
        </span>
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
              className={`tab-title ${primaryTab.customTitle ? "is-custom-title" : ""}`}
              onDoubleClick={(event) => {
                event.preventDefault();
                event.stopPropagation();
                setRenameDraft(primaryTab.customTitle ?? primaryTab.title ?? primaryTab.url);
                setIsRenaming(true);
              }}
            >
              {displayTitle}
            </span>
          )}
          <span className="split-tab-secondary-title">{secondaryTab.title || secondaryTab.url}</span>
        </span>
      </button>
      <span className="tab-row-actions">
        <button
          className="tab-close"
          type="button"
          aria-label={`Close split view`}
          tabIndex={-1}
          draggable={false}
          onDragStart={(event) => {
            event.preventDefault();
            event.stopPropagation();
          }}
          onClick={(event) => {
            event.stopPropagation();
            // Close the secondary pane (exit split mode)
            onClose(secondaryTab.id);
          }}
        >
          <FiX />
        </button>
      </span>
    </div>
  );
}
