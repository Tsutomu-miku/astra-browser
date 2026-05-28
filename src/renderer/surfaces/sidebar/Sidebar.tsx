import {
  useEffect,
  useMemo,
  useState,
  type DragEvent,
  type KeyboardEvent as ReactKeyboardEvent
} from "react";

import { isListNavigationKey } from "../../common/navigation/listNavigation";
import { getMemorySaverState } from "../../common/memory/memorySaverState";
import { getGroupedTabs } from "../../domain/tabs/groups";
import type { BrowserController } from "../../app/controller/types";
import { SidebarAddress } from "./components/chrome/SidebarAddress";
import { SidebarFooter } from "./components/chrome/SidebarFooter";
import { SidebarHeader } from "./components/chrome/SidebarHeader";
import { SidebarSearchBox } from "./components/chrome/SidebarSearchBox";
import { SidebarContextMenus } from "./components/tabs/SidebarContextMenus";
import { SidebarSections } from "./components/tabs/SidebarSections";
import { useSidebarContextMenus } from "./components/tabs/useSidebarContextMenus";
import { WorkspaceStrip } from "./components/workspaces/WorkspaceStrip";
import { useSidebarQuickEntryDrag } from "./hooks/useSidebarQuickEntryDrag";
import { useSidebarWorkspaceDrag } from "./hooks/useSidebarWorkspaceDrag";
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
  const [tabQuery, setTabQuery] = useState("");
  const [activeSearchIndex, setActiveSearchIndex] = useState(0);
  const {
    closedTabMenu,
    closeMenus,
    openClosedTabMenu,
    openQuickEntryMenu,
    openTabGroupMenu,
    openTabMenu,
    quickEntryMenu,
    tabGroupMenu,
    tabMenu
  } = useSidebarContextMenus();
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
    clearWorkspaceDrag,
    draggingGroupId,
    draggingTabId,
    draggingWorkspaceId,
    handleNewWorkspaceDrop,
    handleWorkspaceDragOver,
    handleWorkspaceDragStart,
    handleWorkspaceDrop,
    setDraggingGroupId,
    setDraggingTabId
  } = useSidebarWorkspaceDrag({ actions, activeWorkspaceId: activeWorkspace.id });
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
      actions.navigateActiveTab(intent.url);
    }
  }

  return (
    <aside className={`sidebar ${sidebarCollapsed || compactMode ? "is-collapsed" : ""} ${compactMode ? "is-compact-mode" : ""} ${floatingSidebarOpen ? "is-floating-open" : ""} ${compactChromePeeking ? "is-peeking-chrome" : ""}`}>
      <section className="traffic-space" aria-hidden="true" />
      <WorkspaceStrip
        activeWorkspaceId={activeWorkspace.id}
        compactMode={compactMode}
        draggingGroupId={draggingGroupId}
        draggingTabId={draggingTabId}
        draggingWorkspaceId={draggingWorkspaceId}
        floatingSidebarOpen={floatingSidebarOpen}
        sidebarCollapsed={sidebarCollapsed}
        workspaces={state.workspaces}
        onDragEnd={clearWorkspaceDrag}
        onDragOver={handleWorkspaceDragOver}
        onDragStart={handleWorkspaceDragStart}
        onDrop={handleWorkspaceDrop}
        onDeleteWorkspace={actions.deleteWorkspace}
        onNewWorkspace={actions.addWorkspace}
        onNewWorkspaceDrop={handleNewWorkspaceDrop}
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
          draggingGroupId={draggingGroupId}
          draggingTabId={draggingTabId}
          filteredItems={filteredItems}
          onEssentialDragStart={handleEssentialDragStart}
          onEssentialDrop={handleEssentialDrop}
          onEssentialReorderDrop={handleEssentialReorderDrop}
          onFavoriteDragStart={handleFavoriteDragStart}
          onFavoriteDrop={handleFavoriteDrop}
          onFavoriteReorderDrop={handleFavoriteReorderDrop}
          onClosedTabContextMenu={openClosedTabMenu}
          onTabGroupContextMenu={openTabGroupMenu}
          splitTabIds={state.splitTabIds}
          onQuickEntryContextMenu={openQuickEntryMenu}
          onPinDrop={handlePinDrop}
          onTabContextMenu={openTabMenu}
          onTabDrop={handleTabDrop}
          setDraggingEssentialId={setDraggingEssentialId}
          setDraggingFavoriteId={setDraggingFavoriteId}
          setDraggingGroupId={setDraggingGroupId}
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
      <SidebarContextMenus
        actions={actions}
        activeWorkspace={activeWorkspace}
        closedTabMenu={closedTabMenu}
        closeMenus={closeMenus}
        quickEntryMenu={quickEntryMenu}
        state={state}
        tabGroupMenu={tabGroupMenu}
        tabMenu={tabMenu}
      />
    </aside>
  );
}
