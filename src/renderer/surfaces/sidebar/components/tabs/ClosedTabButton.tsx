import { type ClosedTab, type FaviconCache } from "../../../../domain/browser";
import { getClosedTabAccessibilityLabel } from "../../model/closedTabItemState";
import { runSidebarItemKeyboardActivation } from "../../model/sidebarItemActivation";
import { openSidebarKeyboardContextMenu } from "../../model/sidebarKeyboardContextMenu";
import { SidebarItemActionHints } from "./SidebarItemActionHints";
import { SidebarItemIcon } from "./SidebarItemIcon";
import type { DragEvent, MouseEvent } from "react";
import { FiRotateCcw } from "react-icons/fi";

export function ClosedTabButton({
  closedIndex,
  draggingClosedTabIndex,
  faviconCache,
  onContextMenu,
  onDragEnd,
  onDragStart,
  onOpenInSplit,
  onPreview,
  tab,
  onRestore
}: {
  closedIndex: number;
  draggingClosedTabIndex?: number | null;
  faviconCache?: FaviconCache;
  onContextMenu: (event: MouseEvent, tab: ClosedTab, closedIndex: number) => void;
  onDragEnd?: () => void;
  onDragStart?: (event: DragEvent<HTMLButtonElement>) => void;
  onOpenInSplit: (url: string, title?: string) => void;
  onPreview: (url: string, title?: string) => void;
  tab: ClosedTab;
  onRestore: (closedIndex: number) => void;
}) {
  const title = tab.title || tab.url;
  const isDragging = draggingClosedTabIndex === closedIndex;

  return (
    <button
      className="closed-tab-button"
      type="button"
      draggable={Boolean(onDragStart)}
      aria-label={getClosedTabAccessibilityLabel({ closedIndex, isDragging, tab })}
      data-dragging={isDragging}
      onContextMenu={(event) => onContextMenu(event, tab, closedIndex)}
      onDragStart={onDragStart}
      onDragEnd={onDragEnd}
      onKeyDown={(event) => {
        if (openSidebarKeyboardContextMenu(event)) return;
        runSidebarItemKeyboardActivation(event, {
          primary: () => onRestore(closedIndex),
          preview: () => onPreview(tab.url, tab.title),
          split: () => onOpenInSplit(tab.url, tab.title)
        });
      }}
      onClick={(event) => {
        if (event.altKey) {
          onPreview(tab.url, tab.title);
        } else if (event.shiftKey) {
          onOpenInSplit(tab.url, tab.title);
        } else {
          onRestore(closedIndex);
        }
      }}
    >
      <SidebarItemIcon className="closed-tab-icon" faviconCache={faviconCache} faviconUrl={tab.faviconUrl} url={tab.url} />
      <span className="closed-tab-main">
        <span className="closed-tab-title">{title}</span>
        <span className="closed-tab-url">{tab.url}</span>
      </span>
      <span className="closed-tab-action" aria-hidden="true">
        <FiRotateCcw />
      </span>
      <SidebarItemActionHints />
    </button>
  );
}
