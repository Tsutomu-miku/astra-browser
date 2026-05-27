import type { BrowserTab } from "../../../domain/browser-core";
import type { MoveWorkspaceTarget } from "../model/tabContextMenuState";

interface TabContextMenuProps {
  left: number;
  moveWorkspaceTargets: MoveWorkspaceTarget[];
  onClose: () => void;
  onCloseTab: (tabId: string) => void;
  onDuplicate: (tabId: string) => void;
  onMoveToWorkspace: (tabId: string, workspaceId: string) => void;
  onOpenGlance: (url: string, title?: string) => void;
  onOpenInSplit: (tabId: string) => void;
  onSelect: (tabId: string) => void;
  onSleepTab: (tabId: string) => void;
  onToggleEssential: (tabId: string) => void;
  onToggleFavorite: (tabId: string) => void;
  onToggleMuted: (tabId: string) => void;
  onTogglePinned: (tabId: string) => void;
  tabIsEssential: boolean;
  tabIsFavorite: boolean;
  tab: BrowserTab;
  top: number;
}

export function TabContextMenu({
  left,
  moveWorkspaceTargets,
  onClose,
  onCloseTab,
  onDuplicate,
  onMoveToWorkspace,
  onOpenGlance,
  onOpenInSplit,
  onSelect,
  onSleepTab,
  onToggleEssential,
  onToggleFavorite,
  onToggleMuted,
  onTogglePinned,
  tabIsEssential,
  tabIsFavorite,
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
      <button type="button" role="menuitem" onClick={() => run(() => onOpenGlance(tab.url, tab.title))}>Preview in Glance</button>
      <button type="button" role="menuitem" onClick={() => run(() => onOpenInSplit(tab.id))}>Open in split view</button>
      <button type="button" role="menuitem" onClick={() => run(() => onDuplicate(tab.id))}>Duplicate</button>
      <button type="button" role="menuitem" onClick={() => run(() => onSleepTab(tab.id))}>Sleep tab</button>
      {moveWorkspaceTargets.length > 0 && (
        <>
          <span className="tab-context-menu-separator" />
          {moveWorkspaceTargets.map((workspace) => (
            <button
              key={workspace.id}
              type="button"
              role="menuitem"
              onClick={() => run(() => onMoveToWorkspace(tab.id, workspace.id))}
            >
              Move to {workspace.name}
            </button>
          ))}
        </>
      )}
      <button type="button" role="menuitem" onClick={() => run(() => onToggleFavorite(tab.id))}>
        {tabIsFavorite ? "Remove favorite" : "Add favorite"}
      </button>
      <button type="button" role="menuitem" onClick={() => run(() => onToggleEssential(tab.id))}>
        {tabIsEssential ? "Remove essential" : "Add essential"}
      </button>
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
