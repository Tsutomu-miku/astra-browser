import { useRef } from "react";
import {
  FiArrowRight,
  FiBookmark,
  FiColumns,
  FiCopy,
  FiEye,
  FiFolder,
  FiFolderMinus,
  FiFolderPlus,
  FiGrid,
  FiLink,
  FiMoon,
  FiStar,
  FiSun,
  FiType,
  FiVolume2,
  FiVolumeX,
  FiX
} from "react-icons/fi";

import { handleMenuKeyboardNavigation, useMenuInitialFocus } from "../../../../common/context-menu/menuKeyboardNavigation";
import type { BrowserTab } from "../../../../domain/browser";
import type { MoveWorkspaceTarget, TabCleanupState, TabGroupMenuState } from "../../model/tabContextMenuState";
import { SidebarMenuItem } from "./SidebarMenuItem";

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
      <SidebarMenuItem icon={FiArrowRight} role="menuitem" onClick={() => run(() => onSelect(tab.id))}>Open</SidebarMenuItem>
      <SidebarMenuItem icon={FiEye} role="menuitem" onClick={() => run(() => onOpenGlance(tab.url, tab.title))}>Preview in Glance</SidebarMenuItem>
      <SidebarMenuItem icon={FiColumns} role="menuitem" onClick={() => run(() => onOpenInSplit(tab.id))}>Open in split view</SidebarMenuItem>
      <SidebarMenuItem icon={FiCopy} role="menuitem" onClick={() => run(() => onDuplicate(tab.id))}>Duplicate</SidebarMenuItem>
      <SidebarMenuItem icon={FiLink} role="menuitem" onClick={() => run(() => onCopyText(tab.url))}>Copy URL</SidebarMenuItem>
      <SidebarMenuItem icon={FiType} role="menuitem" onClick={() => run(() => onCopyText(tab.title || tab.url))}>Copy title</SidebarMenuItem>
      <SidebarMenuItem
        icon={tab.isSleeping ? FiSun : FiMoon}
        role="menuitem"
        onClick={() => run(() => tab.isSleeping ? onSelect(tab.id) : onSleepTab(tab.id))}
      >
        {tab.isSleeping ? "Wake tab" : "Sleep tab"}
      </SidebarMenuItem>
      {(groupMenuState.canCreateGroup || groupMenuState.canUngroup || groupMenuState.moveGroupTargets.length > 0) && (
        <>
          <span className="tab-context-menu-separator" />
          {groupMenuState.canCreateGroup && (
            <SidebarMenuItem icon={FiFolderPlus} role="menuitem" onClick={() => run(() => onGroupTab(tab.id))}>New group from tab</SidebarMenuItem>
          )}
          {groupMenuState.canUngroup && (
            <SidebarMenuItem icon={FiFolderMinus} role="menuitem" onClick={() => run(() => onUngroupTab(tab.id))}>Ungroup tab</SidebarMenuItem>
          )}
          {groupMenuState.moveGroupTargets.map((group) => (
            <SidebarMenuItem
              key={group.id}
              icon={FiFolder}
              role="menuitem"
              onClick={() => run(() => onMoveToGroup(tab.id, group.id))}
            >
              Move to {group.name}
            </SidebarMenuItem>
          ))}
        </>
      )}
      {moveWorkspaceTargets.length > 0 && (
        <>
          <span className="tab-context-menu-separator" />
          {moveWorkspaceTargets.map((workspace) => (
            <SidebarMenuItem
              key={workspace.id}
              icon={FiGrid}
              role="menuitem"
              onClick={() => run(() => onMoveToWorkspace(tab.id, workspace.id))}
            >
              Move to {workspace.name}
            </SidebarMenuItem>
          ))}
        </>
      )}
      <SidebarMenuItem icon={FiStar} role="menuitem" onClick={() => run(() => onToggleFavorite(tab.id))}>
        {tabIsFavorite ? "Remove favorite" : "Add favorite"}
      </SidebarMenuItem>
      <SidebarMenuItem icon={FiBookmark} role="menuitem" onClick={() => run(() => onToggleEssential(tab.id))}>
        {tabIsEssential ? "Remove essential" : "Add essential"}
      </SidebarMenuItem>
      <SidebarMenuItem icon={FiGrid} role="menuitem" onClick={() => run(() => onTogglePinned(tab.id))}>
        {tab.isPinned ? "Unpin" : "Pin"}
      </SidebarMenuItem>
      <SidebarMenuItem icon={tab.isMuted ? FiVolume2 : FiVolumeX} role="menuitem" onClick={() => run(() => onToggleMuted(tab.id))}>
        {tab.isMuted ? "Unmute" : "Mute"}
      </SidebarMenuItem>
      <span className="tab-context-menu-separator" />
      <SidebarMenuItem
        icon={FiX}
        role="menuitem"
        disabled={!cleanupState.canCloseOtherTabs}
        onClick={() => run(() => onCloseOtherTabs(tab.id))}
      >
        Close other tabs
      </SidebarMenuItem>
      <SidebarMenuItem
        icon={FiX}
        role="menuitem"
        disabled={!cleanupState.canCloseTabsToLeft}
        onClick={() => run(() => onCloseTabsToLeft(tab.id))}
      >
        Close tabs to the left
      </SidebarMenuItem>
      <SidebarMenuItem
        icon={FiX}
        role="menuitem"
        disabled={!cleanupState.canCloseTabsToRight}
        onClick={() => run(() => onCloseTabsToRight(tab.id))}
      >
        Close tabs to the right
      </SidebarMenuItem>
      <span className="tab-context-menu-separator" />
      <SidebarMenuItem icon={FiX} role="menuitem" className="danger" onClick={() => run(() => onCloseTab(tab.id))}>Close</SidebarMenuItem>
    </div>
  );
}
