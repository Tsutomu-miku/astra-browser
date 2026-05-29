import { isEssential, isFavorite, type BrowserState, type Workspace } from "../../../../domain/browser";
import type { BrowserController } from "../../../../app/controller/types";
import { getMoveWorkspaceTargets, getTabCleanupState, getTabGroupMenuState } from "../../model/tabContextMenuState";
import { ClosedTabContextMenu } from "./ClosedTabContextMenu";
import { QuickEntryContextMenu } from "./QuickEntryContextMenu";
import { TabContextMenu } from "./TabContextMenu";
import { TabGroupContextMenu } from "./TabGroupContextMenu";
import type { ClosedTabMenuState, QuickEntryMenuState, TabGroupMenuState, TabMenuState } from "./useSidebarContextMenus";

export function SidebarContextMenus({
  actions,
  activeWorkspace,
  closedTabMenu,
  closeMenus,
  quickEntryMenu,
  state,
  tabGroupMenu,
  tabMenu
}: {
  actions: BrowserController["actions"];
  activeWorkspace: Workspace;
  closedTabMenu: ClosedTabMenuState | null;
  closeMenus: () => void;
  quickEntryMenu: QuickEntryMenuState | null;
  state: BrowserState;
  tabGroupMenu: TabGroupMenuState | null;
  tabMenu: TabMenuState | null;
}) {
  const tabGroupMenuGroup = tabGroupMenu
    ? activeWorkspace.tabGroups.find((group) => group.id === tabGroupMenu.groupId)
    : undefined;
  const tabGroupTabs = tabGroupMenuGroup
    ? activeWorkspace.tabs.filter((tab) => tab.groupId === tabGroupMenuGroup.id && !tab.isPinned)
    : [];
  const protectedTabIds = new Set([activeWorkspace.activeTabId, ...state.splitTabIds].filter(Boolean));
  const tabGroupMenuTabCount = tabGroupMenuGroup
    ? tabGroupTabs.length
    : 0;
  const canSleepTabGroup = tabGroupTabs.some((tab) => !tab.isSleeping && !protectedTabIds.has(tab.id));

  return (
    <>
      {tabMenu && (
        <TabContextMenu
          left={tabMenu.left}
          cleanupState={getTabCleanupState(activeWorkspace.tabs, tabMenu.tab.id)}
          groupMenuState={getTabGroupMenuState(activeWorkspace.tabGroups, tabMenu.tab)}
          moveWorkspaceTargets={getMoveWorkspaceTargets(state.workspaces, activeWorkspace.id)}
          tab={tabMenu.tab}
          top={tabMenu.top}
          onClose={closeMenus}
          onCloseTab={actions.closeTab}
          onCloseOtherTabs={actions.closeOtherTabs}
          onCloseTabsToLeft={actions.closeTabsToLeft}
          onCloseTabsToRight={actions.closeTabsToRight}
          onCopyText={actions.copyText}
          onDuplicate={actions.duplicateTab}
          onGroupTab={actions.groupTab}
          onMoveToGroup={actions.assignTabToGroup}
          onMoveToWorkspace={actions.moveTabToWorkspace}
          onOpenGlance={actions.openGlance}
          onOpenInSplit={actions.openTabInSplit}
          onSelect={actions.selectTab}
          onSleepTab={actions.sleepTab}
          onToggleEssential={actions.toggleTabEssential}
          onToggleFavorite={actions.toggleTabFavorite}
          onToggleMuted={actions.toggleTabMuted}
          onTogglePinned={actions.toggleTabPinned}
          onUngroupTab={actions.ungroupTab}
          tabIsEssential={isEssential(state, tabMenu.tab.url)}
          tabIsFavorite={isFavorite(activeWorkspace, tabMenu.tab.url)}
        />
      )}
      {quickEntryMenu && (
        <QuickEntryContextMenu
          item={quickEntryMenu.item}
          kind={quickEntryMenu.kind}
          left={quickEntryMenu.left}
          moveWorkspaceTargets={quickEntryMenu.kind === "favorite" ? getMoveWorkspaceTargets(state.workspaces, activeWorkspace.id) : []}
          top={quickEntryMenu.top}
          onClose={closeMenus}
          onCopyText={actions.copyText}
          onMoveToNewWorkspace={quickEntryMenu.kind === "favorite" ? actions.moveWorkspaceFavoriteToNewWorkspace : undefined}
          onMoveToWorkspace={quickEntryMenu.kind === "favorite" ? actions.moveWorkspaceFavoriteToWorkspace : undefined}
          onOpen={actions.navigateActiveTab}
          onOpenInSplit={actions.openUrlInSplit}
          onPreview={actions.openGlance}
          onRemove={quickEntryMenu.kind === "essential" ? actions.removeEssential : actions.removeWorkspaceFavorite}
        />
      )}
      {tabGroupMenu && tabGroupMenuGroup && (
        <TabGroupContextMenu
          canSleepGroup={canSleepTabGroup}
          group={tabGroupMenuGroup}
          left={tabGroupMenu.left}
          moveWorkspaceTargets={getMoveWorkspaceTargets(state.workspaces, activeWorkspace.id)}
          tabCount={tabGroupMenuTabCount}
          top={tabGroupMenu.top}
          onClose={closeMenus}
          onCloseGroup={actions.closeTabGroup}
          onDuplicateGroup={actions.duplicateTabGroup}
          onMoveToNewWorkspace={actions.moveTabGroupToNewWorkspace}
          onMoveToWorkspace={actions.moveTabGroupToWorkspace}
          onSleepGroup={actions.sleepTabGroup}
          onToggleCollapsed={actions.toggleTabGroupCollapsed}
          onUngroupGroup={actions.ungroupTabGroup}
          onUpdate={actions.updateTabGroup}
        />
      )}
      {closedTabMenu && (
        <ClosedTabContextMenu
          closedIndex={closedTabMenu.closedIndex}
          left={closedTabMenu.left}
          moveWorkspaceTargets={getMoveWorkspaceTargets(state.workspaces, activeWorkspace.id)}
          tab={closedTabMenu.tab}
          top={closedTabMenu.top}
          onClose={closeMenus}
          onCopyText={actions.copyText}
          onOpenInSplit={actions.openUrlInSplit}
          onPreview={actions.openGlance}
          onRestore={actions.restoreClosedTab}
          onRestoreToNewWorkspace={actions.restoreClosedTabToNewWorkspace}
          onRestoreToWorkspace={actions.restoreClosedTabToWorkspace}
        />
      )}
    </>
  );
}
