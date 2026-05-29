import { useRef } from "react";

import { handleMenuKeyboardNavigation, useMenuInitialFocus } from "../../../../common/context-menu/menuKeyboardNavigation";
import type { BrowserTab } from "../../../../domain/browser";
import type { MoveWorkspaceTarget, TabCleanupState, TabGroupMenuState } from "../../model/tabContextMenuState";

interface TabContextMenuProps {
  left: number;
  moveWorkspaceTargets: MoveWorkspaceTarget[];
  cleanupState: TabCleanupState;
  groupMenuState: TabGroupMenuState;
  onClose: () => void;
  onCloseTab: (tabId: string) => void;
  onCloseOtherTabs: (tabId: string) => void;
  onCloseTabsToLeft: (tabId: string) => void;
  onCloseTabsToRight: (tabId: string) => void;
  onCopyText: (text: string) => void;
  onDuplicate: (tabId: string) => void;
  onGroupTab: (tabId: string) => void;
  onMoveToGroup: (tabId: string, groupId: string) => void;
  onMoveToWorkspace: (tabId: string, workspaceId: string) => void;
  onOpenGlance: (url: string, title?: string) => void;
  onOpenInSplit: (tabId: string) => void;
  onSelect: (tabId: string) => void;
  onSleepTab: (tabId: string) => void;
  onToggleEssential: (tabId: string) => void;
  onToggleFavorite: (tabId: string) => void;
  onToggleMuted: (tabId: string) => void;
  onTogglePinned: (tabId: string) => void;
  onUngroupTab: (tabId: string) => void;
  tabIsEssential: boolean;
  tabIsFavorite: boolean;
  tab: BrowserTab;
  top: number;
}

export function TabContextMenu({
  left,
  moveWorkspaceTargets,
  cleanupState,
  groupMenuState,
  onClose,
  onCloseTab,
  onCloseOtherTabs,
  onCloseTabsToLeft,
  onCloseTabsToRight,
  onCopyText,
  onDuplicate,
  onGroupTab,
  onMoveToGroup,
  onMoveToWorkspace,
  onOpenGlance,
  onOpenInSplit,
  onSelect,
  onSleepTab,
  onToggleEssential,
  onToggleFavorite,
  onToggleMuted,
  onTogglePinned,
  onUngroupTab,
  tabIsEssential,
  tabIsFavorite,
  tab,
  top
}: TabContextMenuProps) {
  const menuRef = useRef<HTMLDivElement | null>(null);
  useMenuInitialFocus(menuRef);
  const run = (action: () => void) => {
    action();
    onClose();
  };

  return (
    <div
      ref={menuRef}
      className="tab-context-menu"
      role="menu"
      style={{ left, top }}
      onContextMenu={(event) => event.preventDefault()}
      onKeyDown={handleMenuKeyboardNavigation}
    >
      <button type="button" role="menuitem" onClick={() => run(() => onSelect(tab.id))}>Open</button>
      <button type="button" role="menuitem" onClick={() => run(() => onOpenGlance(tab.url, tab.title))}>Preview in Glance</button>
      <button type="button" role="menuitem" onClick={() => run(() => onOpenInSplit(tab.id))}>Open in split view</button>
      <button type="button" role="menuitem" onClick={() => run(() => onDuplicate(tab.id))}>Duplicate</button>
      <button type="button" role="menuitem" onClick={() => run(() => onCopyText(tab.url))}>Copy URL</button>
      <button type="button" role="menuitem" onClick={() => run(() => onCopyText(tab.title || tab.url))}>Copy title</button>
      <button
        type="button"
        role="menuitem"
        onClick={() => run(() => tab.isSleeping ? onSelect(tab.id) : onSleepTab(tab.id))}
      >
        {tab.isSleeping ? "Wake tab" : "Sleep tab"}
      </button>
      {(groupMenuState.canCreateGroup || groupMenuState.canUngroup || groupMenuState.moveGroupTargets.length > 0) && (
        <>
          <span className="tab-context-menu-separator" />
          {groupMenuState.canCreateGroup && (
            <button type="button" role="menuitem" onClick={() => run(() => onGroupTab(tab.id))}>New group from tab</button>
          )}
          {groupMenuState.canUngroup && (
            <button type="button" role="menuitem" onClick={() => run(() => onUngroupTab(tab.id))}>Ungroup tab</button>
          )}
          {groupMenuState.moveGroupTargets.map((group) => (
            <button
              key={group.id}
              type="button"
              role="menuitem"
              onClick={() => run(() => onMoveToGroup(tab.id, group.id))}
            >
              Move to {group.name}
            </button>
          ))}
        </>
      )}
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
      <button
        type="button"
        role="menuitem"
        disabled={!cleanupState.canCloseOtherTabs}
        onClick={() => run(() => onCloseOtherTabs(tab.id))}
      >
        Close other tabs
      </button>
      <button
        type="button"
        role="menuitem"
        disabled={!cleanupState.canCloseTabsToLeft}
        onClick={() => run(() => onCloseTabsToLeft(tab.id))}
      >
        Close tabs to the left
      </button>
      <button
        type="button"
        role="menuitem"
        disabled={!cleanupState.canCloseTabsToRight}
        onClick={() => run(() => onCloseTabsToRight(tab.id))}
      >
        Close tabs to the right
      </button>
      <span className="tab-context-menu-separator" />
      <button type="button" role="menuitem" className="danger" onClick={() => run(() => onCloseTab(tab.id))}>Close</button>
    </div>
  );
}
