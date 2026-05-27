import type { MouseEvent } from "react";

import { getReadableUrlTitle, type Favorite } from "../../../domain/browser-core";
import { StartEntryActionHints } from "./StartEntryActionHints";

export function StartTileGrid({
  emptyText,
  items,
  onOpen
}: {
  emptyText: string;
  items: Favorite[];
  onOpen: (event: MouseEvent, url: string, title?: string) => void;
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
          title={item.url}
          onClick={(event) => onOpen(event, item.url, item.title)}
        >
          <span className="start-tile-icon">{getReadableUrlTitle(item.url).slice(0, 1).toUpperCase()}</span>
          <span className="start-tile-title">{item.title}</span>
          <StartEntryActionHints />
        </button>
      ))}
    </div>
  );
}
