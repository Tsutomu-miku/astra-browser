import type { BrowserTab } from "../../domain/browser-core";

interface TabContextMenuProps {
  left: number;
  onClose: () => void;
  onCloseTab: (tabId: string) => void;
  onDuplicate: (tabId: string) => void;
  onOpenInSplit: (tabId: string) => void;
  onSelect: (tabId: string) => void;
  onToggleMuted: (tabId: string) => void;
  onTogglePinned: (tabId: string) => void;
  tab: BrowserTab;
  top: number;
}

export function TabContextMenu({
  left,
  onClose,
  onCloseTab,
  onDuplicate,
  onOpenInSplit,
  onSelect,
  onToggleMuted,
  onTogglePinned,
  tab,
  top
}: TabContextMenuProps) {
  const run = (action: () => void) => {
    action();
    onClose();
  };

  return (
    <div
      className="tab-context-menu"
      role="menu"
      style={{ left, top }}
      onContextMenu={(event) => event.preventDefault()}
    >
      <button type="button" role="menuitem" onClick={() => run(() => onSelect(tab.id))}>Open</button>
      <button type="button" role="menuitem" onClick={() => run(() => onOpenInSplit(tab.id))}>Open in split view</button>
      <button type="button" role="menuitem" onClick={() => run(() => onDuplicate(tab.id))}>Duplicate</button>
      <button type="button" role="menuitem" onClick={() => run(() => onTogglePinned(tab.id))}>
        {tab.isPinned ? "Unpin" : "Pin"}
      </button>
      <button type="button" role="menuitem" onClick={() => run(() => onToggleMuted(tab.id))}>
        {tab.isMuted ? "Unmute" : "Mute"}
      </button>
      <span className="tab-context-menu-separator" />
      <button type="button" role="menuitem" className="danger" onClick={() => run(() => onCloseTab(tab.id))}>Close</button>
    </div>
  );
}
