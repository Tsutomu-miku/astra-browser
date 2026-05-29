import { getHostInitial, type ClosedTab } from "../../../../domain/browser";
import { openSidebarKeyboardContextMenu } from "../../model/sidebarKeyboardContextMenu";
import { SidebarItemActionHints } from "./SidebarItemActionHints";
import type { MouseEvent } from "react";

export function ClosedTabButton({
  closedIndex,
  onContextMenu,
  onOpenInSplit,
  onPreview,
  tab,
  onRestore
}: {
  closedIndex: number;
  onContextMenu: (event: MouseEvent, tab: ClosedTab, closedIndex: number) => void;
  onOpenInSplit: (url: string, title?: string) => void;
  onPreview: (url: string, title?: string) => void;
  tab: ClosedTab;
  onRestore: (closedIndex: number) => void;
}) {
  const title = tab.title || tab.url;

  return (
    <button
      className="closed-tab-button"
      type="button"
      title={`Restore ${title}`}
      onContextMenu={(event) => onContextMenu(event, tab, closedIndex)}
      onKeyDown={openSidebarKeyboardContextMenu}
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
      <span className="closed-tab-icon">{getHostInitial(tab.url)}</span>
      <span className="closed-tab-main">
        <span className="closed-tab-title">{title}</span>
        <span className="closed-tab-url">{tab.url}</span>
      </span>
      <span className="closed-tab-action">Restore</span>
      <SidebarItemActionHints />
    </button>
  );
}
