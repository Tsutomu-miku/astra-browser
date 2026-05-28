import { isEssential, isFavorite, type BrowserState, type Workspace } from "../../../../domain/browser";
import type { BrowserController } from "../../../../app/controller/types";
import { getMoveWorkspaceTargets, getTabCleanupState, getTabGroupMenuState } from "../../model/tabContextMenuState";
import { ClosedTabContextMenu } from "./ClosedTabContextMenu";
import { QuickEntryContextMenu } from "./QuickEntryContextMenu";
import { TabContextMenu } from "./TabContextMenu";
import type { ClosedTabMenuState, QuickEntryMenuState, TabMenuState } from "./useSidebarContextMenus";

export function SidebarContextMenus({
  actions,
  activeWorkspace,
  closedTabMenu,
  closeMenus,
  quickEntryMenu,
  state,
  tabMenu
}: {
  actions: BrowserController["actions"];
  activeWorkspace: Workspace;
  closedTabMenu: ClosedTabMenuState | null;
  closeMenus: () => void;
  quickEntryMenu: QuickEntryMenuState | null;
  state: BrowserState;
  tabMenu: TabMenuState | null;
}) {
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
          top={quickEntryMenu.top}
          onClose={closeMenus}
          onCopyText={actions.copyText}
          onOpen={actions.openUrlInActiveWorkspace}
          onOpenInSplit={actions.openUrlInSplit}
          onPreview={actions.openGlance}
          onRemove={quickEntryMenu.kind === "essential" ? actions.removeEssential : actions.removeWorkspaceFavorite}
        />
      )}
      {closedTabMenu && (
        <ClosedTabContextMenu
          closedIndex={closedTabMenu.closedIndex}
          left={closedTabMenu.left}
          tab={closedTabMenu.tab}
          top={closedTabMenu.top}
          onClose={closeMenus}
          onCopyText={actions.copyText}
          onOpenInSplit={actions.openUrlInSplit}
          onPreview={actions.openGlance}
          onRestore={actions.restoreClosedTab}
        />
      )}
    </>
  );
}
