import type { MouseEvent } from "react";

import { BrowserItemIcon } from "../../../common/icons/BrowserItemIcon";
import type { FaviconCache } from "../../../domain/browser";
import { StartEntryActionHints } from "./StartEntryActionHints";

export interface StartTileItem {
  id: string;
  title: string;
  url: string;
  tabId?: string;
}

export function StartTileGrid({
  emptyText,
  faviconCache,
  items,
  kind,
  onContextMenu,
  onOpen
}: {
  emptyText: string;
  faviconCache?: FaviconCache;
  items: StartTileItem[];
  kind: "essential" | "favorite";
  onContextMenu: (event: MouseEvent, item: StartTileItem, kind: "essential" | "favorite") => void;
  onOpen: (event: MouseEvent, item: StartTileItem, kind: "essential" | "favorite") => void;
}) {
  return (
    <div className="start-tile-grid">
      {items.length === 0 ? (
        <p className="start-empty">{emptyText}</p>
      ) : items.map((item) => (
        <button
          className="start-tile"
          key={item.id}
          type="button"
          aria-label={`${item.title}, ${kind}, ${item.url}`}
          onContextMenu={(event) => onContextMenu(event, item, kind)}
          onClick={(event) => onOpen(event, item, kind)}
        >
          <BrowserItemIcon className="start-tile-icon" faviconCache={faviconCache} url={item.url} />
          <span className="start-tile-title">{item.title}</span>
          <StartEntryActionHints />
        </button>
      ))}
    </div>
  );
}
