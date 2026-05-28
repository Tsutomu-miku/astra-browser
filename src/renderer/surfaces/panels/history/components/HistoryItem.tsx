import type { MouseEvent } from "react";
import { FiX } from "react-icons/fi";

import type { HistoryEntry } from "../../../../domain/browser";
import { formatHistoryTime } from "../model/historyFormat";

export function HistoryItem({
  entry,
  onContextMenu,
  onOpen,
  onOpenInSplit,
  onPreview,
  onRemove
}: {
  entry: HistoryEntry;
  onContextMenu: (event: MouseEvent, entry: HistoryEntry) => void;
  onOpen: (url: string, title?: string) => void;
  onOpenInSplit: (url: string, title?: string) => void;
  onPreview: (url: string, title?: string) => void;
  onRemove: (historyId: string) => void;
}) {
  function openEntry(event: MouseEvent) {
    if (event.altKey) {
      onPreview(entry.url, entry.title);
    } else if (event.shiftKey) {
      onOpenInSplit(entry.url, entry.title);
    } else {
      onOpen(entry.url, entry.title);
    }
  }

  return (
    <article className="history-item" onContextMenu={(event) => onContextMenu(event, entry)}>
      <button className="history-open" type="button" onClick={openEntry}>
        <span className="history-title">{entry.title}</span>
        <span className="history-url">{entry.url}</span>
        <span className="history-action-hints" aria-hidden="true">
          Alt Glance · Shift Split
        </span>
      </button>
      <span className="history-time">{formatHistoryTime(entry.visitedAt)}</span>
      <button className="history-remove" type="button" title="Remove history entry" onClick={() => onRemove(entry.id)}><FiX /></button>
    </article>
  );
}
