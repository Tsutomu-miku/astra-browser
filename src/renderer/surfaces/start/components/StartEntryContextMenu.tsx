import type { StartEntryContextMenuItem, StartEntryContextMenuKind } from "./useStartEntryContextMenu";

export function StartEntryContextMenu({
  item,
  kind,
  left,
  onClose,
  onOpen,
  onOpenInSplit,
  onPreview,
  onRemove,
  top
}: {
  item: StartEntryContextMenuItem;
  kind: StartEntryContextMenuKind;
  left: number;
  onClose: () => void;
  onOpen: (url: string, title?: string) => void;
  onOpenInSplit: (url: string, title?: string) => void;
  onPreview: (url: string, title?: string) => void;
  onRemove: (item: StartEntryContextMenuItem, kind: StartEntryContextMenuKind) => void;
  top: number;
}) {
  const label = kind === "essential" ? "Essential" : kind === "favorite" ? "Favorite" : "History";
  const run = (action: () => void) => {
    action();
    onClose();
  };

  return (
    <div
      className="start-context-menu"
      role="menu"
      style={{ left, top }}
      onContextMenu={(event) => event.preventDefault()}
    >
      <button type="button" role="menuitem" onClick={() => run(() => onOpen(item.url, item.title))}>
        Open
      </button>
      <button type="button" role="menuitem" onClick={() => run(() => onPreview(item.url, item.title))}>
        Preview in Glance
      </button>
      <button type="button" role="menuitem" onClick={() => run(() => onOpenInSplit(item.url, item.title))}>
        Open in split view
      </button>
      <span className="start-context-menu-separator" />
      <button type="button" role="menuitem" className="danger" onClick={() => run(() => onRemove(item, kind))}>
        Remove {label}
      </button>
    </div>
  );
}
