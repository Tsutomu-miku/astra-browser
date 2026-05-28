import type { ClosedTab } from "../../../../domain/browser";

export function ClosedTabContextMenu({
  closedIndex,
  left,
  onClose,
  onCopyText,
  onOpenInSplit,
  onPreview,
  onRestore,
  tab,
  top
}: {
  closedIndex: number;
  left: number;
  onClose: () => void;
  onCopyText: (text: string) => void;
  onOpenInSplit: (url: string, title?: string) => void;
  onPreview: (url: string, title?: string) => void;
  onRestore: (closedIndex: number) => void;
  tab: ClosedTab;
  top: number;
}) {
  const title = tab.title || tab.url;
  const run = (action: () => void) => {
    action();
    onClose();
  };

  return (
    <div
      className="tab-context-menu closed-tab-context-menu"
      role="menu"
      style={{ left, top }}
      onContextMenu={(event) => event.preventDefault()}
    >
      <button type="button" role="menuitem" onClick={() => run(() => onRestore(closedIndex))}>Restore</button>
      <button type="button" role="menuitem" onClick={() => run(() => onPreview(tab.url, tab.title))}>Preview in Glance</button>
      <button type="button" role="menuitem" onClick={() => run(() => onOpenInSplit(tab.url, tab.title))}>Open in split view</button>
      <button type="button" role="menuitem" onClick={() => run(() => onCopyText(tab.url))}>Copy URL</button>
      <button type="button" role="menuitem" onClick={() => run(() => onCopyText(title))}>Copy title</button>
    </div>
  );
}
