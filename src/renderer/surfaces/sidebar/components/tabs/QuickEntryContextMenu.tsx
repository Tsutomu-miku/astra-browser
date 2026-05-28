import type { Favorite } from "../../../../domain/browser";

export function QuickEntryContextMenu({
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
  item: Favorite;
  kind: "essential" | "favorite";
  left: number;
  onClose: () => void;
  onOpen: (url: string, title?: string) => void;
  onOpenInSplit: (url: string, title?: string) => void;
  onPreview: (url: string, title?: string) => void;
  onRemove: (url: string) => void;
  top: number;
}) {
  const label = kind === "essential" ? "Essential" : "Favorite";
  const run = (action: () => void) => {
    action();
    onClose();
  };

  return (
    <div
      className="tab-context-menu quick-entry-context-menu"
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
      <span className="tab-context-menu-separator" />
      <button type="button" role="menuitem" className="danger" onClick={() => run(() => onRemove(item.url))}>
        Remove {label}
      </button>
    </div>
  );
}
