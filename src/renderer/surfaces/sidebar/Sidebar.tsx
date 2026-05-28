import {
  useEffect,
  useMemo,
  useState,
  type DragEvent,
  type KeyboardEvent as ReactKeyboardEvent
} from "react";

import { isListNavigationKey } from "../../common/navigation/listNavigation";
import { getMemorySaverState } from "../../common/memory/memorySaverState";
import { isEssential, isFavorite } from "../../domain/browser";
import { getGroupedTabs } from "../../domain/tabs/groups";
import type { BrowserController } from "../../app/controller/types";
import { SidebarAddress } from "./components/chrome/SidebarAddress";
import { SidebarFooter } from "./components/chrome/SidebarFooter";
import { SidebarHeader } from "./components/chrome/SidebarHeader";
import { SidebarSearchBox } from "./components/chrome/SidebarSearchBox";
import { QuickEntryContextMenu } from "./components/tabs/QuickEntryContextMenu";
import { SidebarSections } from "./components/tabs/SidebarSections";
import { TabContextMenu } from "./components/tabs/TabContextMenu";
import { useSidebarContextMenus } from "./components/tabs/useSidebarContextMenus";
import { WorkspaceStrip } from "./components/workspaces/WorkspaceStrip";
import { useSidebarQuickEntryDrag } from "./hooks/useSidebarQuickEntryDrag";
import { getMoveWorkspaceTargets, getTabCleanupState, getTabGroupMenuState } from "./model/tabContextMenuState";
import {
  clampSidebarSearchIndex,
  filterSidebarItems,
  getNextSidebarSearchIndex,
  getSidebarSearchTargets,
  type SidebarSearchTarget
} from "./sidebarFiltering";
import { getSidebarSearchOpenIntent, type SidebarOpenIntent } from "./sidebarOpenIntent";

export function Sidebar({ controller }: { controller: BrowserController }) {
  const { activeTab, activeWorkspace, actions, compactChromePeeking, compactMode, floatingSidebarOpen, setPanel, sidebarCollapsed, state } = controller;
  const [draggingTabId, setDraggingTabId] = useState<string | null>(null);
  const [draggingWorkspaceId, setDraggingWorkspaceId] = useState<string | null>(null);
  const [tabQuery, setTabQuery] = useState("");
  const [activeSearchIndex, setActiveSearchIndex] = useState(0);
  const { closeMenus, openQuickEntryMenu, openTabMenu, quickEntryMenu, setQuickEntryMenu, tabMenu } = useSidebarContextMenus();
  const pinnedTabs = activeWorkspace.tabs.filter((tab) => tab.isPinned);
  const groupedTabs = getGroupedTabs(activeWorkspace);
  const groupedTabIds = new Set(groupedTabs.flatMap((entry) => entry.tabs.map((tab) => tab.id)));
  const regularTabs = activeWorkspace.tabs.filter((tab) => !tab.isPinned && !groupedTabIds.has(tab.id));
  const memorySaver = useMemo(() => getMemorySaverState(activeWorkspace, state), [activeWorkspace, state]);
  const filteredItems = useMemo(() => filterSidebarItems({
    essentials: state.essentials,
    favorites: activeWorkspace.favorites,
    groupedTabs,
    pinnedTabs,
    regularTabs
  }, tabQuery), [activeWorkspace.favorites, groupedTabs, pinnedTabs, regularTabs, state.essentials, tabQuery]);
  const searchTargets = useMemo(() => getSidebarSearchTargets(filteredItems), [filteredItems]);
  const activeSearchTarget = filteredItems.isFiltering
    ? searchTargets[clampSidebarSearchIndex(activeSearchIndex, searchTargets.length)]
    : undefined;
  const {
    draggingEssentialId,
    draggingFavoriteId,
    handleEssentialDragStart,
    handleEssentialDrop,
    handleEssentialReorderDrop,
    handleFavoriteDragStart,
    handleFavoriteDrop,
    handleFavoriteReorderDrop,
    setDraggingEssentialId,
    setDraggingFavoriteId
  } = useSidebarQuickEntryDrag({ actions, activeWorkspace, draggingTabId, setDraggingTabId, state });

  const handleTabDrop = (event: DragEvent<HTMLElement>, targetTabId: string) => {
    event.preventDefault();
    if (!draggingTabId || draggingTabId === targetTabId) {
      setDraggingTabId(null);
      return;
    }

    const rect = event.currentTarget.getBoundingClientRect();
    const placement = event.clientY > rect.top + rect.height / 2 ? "after" : "before";
    actions.reorderTab(draggingTabId, targetTabId, placement);
    setDraggingTabId(null);
  };

  const getDroppedTabId = (event: DragEvent<HTMLElement>) => draggingTabId || event.dataTransfer.getData("text/plain");

  const handlePinDrop = (event: DragEvent<HTMLElement>) => {
    const tabId = getDroppedTabId(event);
    const tab = activeWorkspace.tabs.find((candidate) => candidate.id === tabId);
    if (!tab) return;

    event.preventDefault();
    if (!tab.isPinned) actions.toggleTabPinned(tab.id);
    setDraggingTabId(null);
  };

  useEffect(() => {
    setTabQuery("");
    setActiveSearchIndex(0);
  }, [activeWorkspace.id]);

  useEffect(() => {
    setActiveSearchIndex((index) => clampSidebarSearchIndex(index, searchTargets.length));
  }, [searchTargets.length]);

  function onSearchKeyDown(event: ReactKeyboardEvent<HTMLInputElement>) {
    if (isListNavigationKey(event.key)) {
      event.preventDefault();
      const key = event.key;
      setActiveSearchIndex((index) => getNextSidebarSearchIndex(index, searchTargets.length, key));
    } else if (event.key === "Enter") {
      event.preventDefault();
      if (activeSearchTarget) runSearchTarget(activeSearchTarget, {
        altKey: event.altKey,
        shiftKey: event.shiftKey
      });
    } else if (event.key === "Escape" && tabQuery) {
      event.preventDefault();
      setTabQuery("");
      setActiveSearchIndex(0);
    }
  }

  function runSearchTarget(target: SidebarSearchTarget, modifiers: { altKey: boolean; shiftKey: boolean }) {
    runSidebarIntent(getSidebarSearchOpenIntent(target, modifiers));
  }

  function runSidebarIntent(intent: SidebarOpenIntent) {
    if (intent.type === "preview") {
      actions.openGlance(intent.url, intent.title);
    } else if (intent.type === "splitTab") {
      actions.openTabInSplit(intent.tabId);
    } else if (intent.type === "splitUrl") {
      actions.openUrlInSplit(intent.url, intent.title);
    } else if (intent.type === "selectTab") {
      actions.selectTab(intent.tabId);
    } else {
      actions.openUrlInActiveWorkspace(intent.url, intent.title);
    }
  }

  const handleWorkspaceDragStart = (event: DragEvent<HTMLButtonElement>, workspaceId: string) => {
    setDraggingWorkspaceId(workspaceId);
    event.dataTransfer.effectAllowed = "move";
    event.dataTransfer.setData("text/workspace-id", workspaceId);
  };

  const handleWorkspaceDragOver = (event: DragEvent<HTMLButtonElement>, workspaceId: string) => {
    const isTabTarget = draggingTabId && workspaceId !== activeWorkspace.id;
    const isWorkspaceTarget = draggingWorkspaceId && workspaceId !== draggingWorkspaceId;
    if (isTabTarget || isWorkspaceTarget) {
      event.preventDefault();
      event.dataTransfer.dropEffect = "move";
    }
  };

  const handleWorkspaceDrop = (event: DragEvent<HTMLButtonElement>, workspaceId: string) => {
    event.preventDefault();
    if (draggingWorkspaceId && draggingWorkspaceId !== workspaceId) {
      const rect = event.currentTarget.getBoundingClientRect();
      const placement = event.clientY > rect.top + rect.height / 2 ? "after" : "before";
      actions.reorderWorkspace(draggingWorkspaceId, workspaceId, placement);
      setDraggingWorkspaceId(null);
      return;
    }

    const tabId = draggingTabId || event.dataTransfer.getData("text/plain");
    if (tabId && workspaceId !== activeWorkspace.id) {
      actions.moveTabToWorkspace(tabId, workspaceId);
    }
    setDraggingTabId(null);
  };

  return (
    <aside className={`sidebar ${sidebarCollapsed || compactMode ? "is-collapsed" : ""} ${compactMode ? "is-compact-mode" : ""} ${floatingSidebarOpen ? "is-floating-open" : ""} ${compactChromePeeking ? "is-peeking-chrome" : ""}`}>
      <section className="traffic-space" aria-hidden="true" />
      <WorkspaceStrip
        activeWorkspaceId={activeWorkspace.id}
        compactMode={compactMode}
        draggingTabId={draggingTabId}
        draggingWorkspaceId={draggingWorkspaceId}
        floatingSidebarOpen={floatingSidebarOpen}
        sidebarCollapsed={sidebarCollapsed}
        workspaces={state.workspaces}
        onDragEnd={() => setDraggingWorkspaceId(null)}
        onDragOver={handleWorkspaceDragOver}
        onDragStart={handleWorkspaceDragStart}
        onDrop={handleWorkspaceDrop}
        onDeleteWorkspace={actions.deleteWorkspace}
        onNewWorkspace={actions.addWorkspace}
        onSelect={actions.switchWorkspace}
        onToggleSidebar={actions.toggleSidebar}
        onUpdateWorkspace={actions.updateWorkspaceById}
      />

      <section className="tab-stack">
        <SidebarHeader workspaceName={activeWorkspace.name} onNewTab={actions.newTab} />
        <SidebarAddress controller={controller} />
        <SidebarSearchBox
          activeSearchTarget={activeSearchTarget}
          query={tabQuery}
          onClear={() => {
            setTabQuery("");
            setActiveSearchIndex(0);
          }}
          onKeyDown={onSearchKeyDown}
          onQueryChange={(query) => {
            setTabQuery(query);
            setActiveSearchIndex(0);
          }}
        />
        <SidebarSections
          actions={actions}
          activeSearchTarget={activeSearchTarget}
          activeTab={activeTab}
          closedTabs={activeWorkspace.closedTabs}
          draggingEssentialId={draggingEssentialId}
          draggingFavoriteId={draggingFavoriteId}
          draggingTabId={draggingTabId}
          filteredItems={filteredItems}
          onEssentialDragStart={handleEssentialDragStart}
          onEssentialDrop={handleEssentialDrop}
          onEssentialReorderDrop={handleEssentialReorderDrop}
          onFavoriteDragStart={handleFavoriteDragStart}
          onFavoriteDrop={handleFavoriteDrop}
          onFavoriteReorderDrop={handleFavoriteReorderDrop}
          splitTabIds={state.splitTabIds}
          onQuickEntryContextMenu={openQuickEntryMenu}
          onPinDrop={handlePinDrop}
          onTabContextMenu={openTabMenu}
          onTabDrop={handleTabDrop}
          setDraggingEssentialId={setDraggingEssentialId}
          setDraggingFavoriteId={setDraggingFavoriteId}
          setDraggingTabId={setDraggingTabId}
        />
      </section>

      <SidebarFooter
        actions={actions}
        activeTabId={activeTab.id}
        compactMode={compactMode}
        draggingTabId={draggingTabId}
        floatingSidebarOpen={floatingSidebarOpen}
        memorySaver={memorySaver}
        setPanel={setPanel}
        setDraggingTabId={setDraggingTabId}
        splitLayout={controller.splitLayout}
        splitMode={state.splitMode}
      />
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
          onClose={() => setQuickEntryMenu(null)}
          onCopyText={actions.copyText}
          onOpen={actions.openUrlInActiveWorkspace}
          onOpenInSplit={actions.openUrlInSplit}
          onPreview={actions.openGlance}
          onRemove={quickEntryMenu.kind === "essential" ? actions.removeEssential : actions.removeWorkspaceFavorite}
        />
      )}
    </aside>
  );
}
