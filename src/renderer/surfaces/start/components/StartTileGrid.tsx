import type { MouseEvent } from "react";

import { getReadableUrlTitle, type Favorite } from "../../../domain/browser";
import { StartEntryActionHints } from "./StartEntryActionHints";

export function StartTileGrid({
  emptyText,
  items,
  kind,
  onContextMenu,
  onOpen
}: {
  emptyText: string;
  items: Favorite[];
  kind: "essential" | "favorite";
  onContextMenu: (event: MouseEvent, item: Favorite, kind: "essential" | "favorite") => void;
  onOpen: (event: MouseEvent, item: Favorite, kind: "essential" | "favorite") => void;
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
          <span className="start-tile-icon">{getReadableUrlTitle(item.url).slice(0, 1).toUpperCase()}</span>
          <span className="start-tile-title">{item.title}</span>
          <StartEntryActionHints />
        </button>
      ))}
    </div>
  );
}
