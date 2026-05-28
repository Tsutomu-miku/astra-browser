import type { HistoryEntry } from "../../../../domain/browser";

export function HistoryEntryContextMenu({
  entry,
  left,
  onClose,
  onOpen,
  onOpenInSplit,
  onPreview,
  onRemove,
  top
}: {
  entry: HistoryEntry;
  left: number;
  onClose: () => void;
  onOpen: (url: string, title?: string) => void;
  onOpenInSplit: (url: string, title?: string) => void;
  onPreview: (url: string, title?: string) => void;
  onRemove: (historyId: string) => void;
  top: number;
}) {
  const run = (action: () => void) => {
    action();
    onClose();
  };

  return (
    <div
      className="history-context-menu"
      role="menu"
      style={{ left, top }}
      onContextMenu={(event) => event.preventDefault()}
    >
      <button type="button" role="menuitem" onClick={() => run(() => onOpen(entry.url, entry.title))}>
        Open
      </button>
      <button type="button" role="menuitem" onClick={() => run(() => onPreview(entry.url, entry.title))}>
        Preview in Glance
      </button>
      <button type="button" role="menuitem" onClick={() => run(() => onOpenInSplit(entry.url, entry.title))}>
        Open in split view
      </button>
      <span className="history-context-menu-separator" />
      <button type="button" role="menuitem" className="danger" onClick={() => run(() => onRemove(entry.id))}>
        Remove History
      </button>
    </div>
  );
}
