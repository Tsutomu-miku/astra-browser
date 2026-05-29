import {
  useEffect,
  useMemo,
  useState,
  type CSSProperties,
  type DragEvent,
  type KeyboardEvent as ReactKeyboardEvent
} from "react";

import { getPointerDropPlacement, type DropAxis } from "../../common/drag-drop/dropPlacement";
import { readSidebarTabDragPayload } from "../../common/drag-drop/sidebarDragPayload";
import { isListNavigationKey } from "../../common/navigation/listNavigation";
import { getMemorySaverState } from "../../common/memory/memorySaverState";
import type { BrowserController } from "../../app/controller/types";
import { SidebarAddress } from "./components/chrome/SidebarAddress";
import { SidebarFooter } from "./components/chrome/SidebarFooter";
import { SidebarHeader } from "./components/chrome/SidebarHeader";
import { SidebarResizeHandle } from "./components/chrome/SidebarResizeHandle";
import { SidebarSearchBox } from "./components/chrome/SidebarSearchBox";
import { SidebarContextMenus } from "./components/tabs/SidebarContextMenus";
import { SidebarSections } from "./components/tabs/SidebarSections";
import { useSidebarContextMenus } from "./components/tabs/useSidebarContextMenus";
import { WorkspaceStrip } from "./components/workspaces/WorkspaceStrip";
import { useSidebarQuickEntryDrag } from "./hooks/useSidebarQuickEntryDrag";
import { useSidebarWorkspaceDrag } from "./hooks/useSidebarWorkspaceDrag";
import { handleSidebarFocusNavigation } from "./model/sidebarFocusNavigation";
import { scrollSidebarSearchTargetIntoView } from "./model/sidebarSearchTargetDom";
import { getSidebarTabDropIntent, getSidebarTabsAreaDropIntent } from "./model/sidebarTabDropIntent";
import { getSidebarTabFolders } from "./model/sidebarTabFolders";
import {
  clampSidebarSearchIndex,
  filterSidebarItems,
  getNextSidebarSearchIndex,
  getSidebarSearchTargets,
  type SidebarSearchTarget
} from "./sidebarFiltering";
import { getSidebarSearchOpenIntent, type SidebarOpenIntent } from "./sidebarOpenIntent";

export function Sidebar({ controller }: { controller: BrowserController }) {
  const { activeTab, activeWorkspace, actions, compactMode, compactSidebarPeeking, floatingSidebarOpen, setPanel, setSidebarWidth, sidebarCollapsed, sidebarWidth, state } = controller;
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
  const {
    groupedTabs,
    pinnedTabs,
    regularTabs
  } = useMemo(() => getSidebarTabFolders(activeWorkspace), [activeWorkspace]);
  const memorySaver = useMemo(() => getMemorySaverState(activeWorkspace, state), [activeWorkspace, state]);
  const filteredItems = useMemo(() => filterSidebarItems({
    essentials: state.essentials,
    favorites: activeWorkspace.favorites,
    groupedTabs,
    pinnedTabs,
    regularTabs,
    workspaceTabs: activeWorkspace.tabs
  }, tabQuery), [activeWorkspace.favorites, activeWorkspace.tabs, groupedTabs, pinnedTabs, regularTabs, state.essentials, tabQuery]);
  const searchTargets = useMemo(() => getSidebarSearchTargets(filteredItems), [filteredItems]);
  const activeSearchTarget = filteredItems.isFiltering
    ? searchTargets[clampSidebarSearchIndex(activeSearchIndex, searchTargets.length)]
    : undefined;
  const {
    clearWorkspaceDrag,
    draggingClosedTabIndex,
    draggingGroupId,
    draggingTabId,
    draggingWorkspaceId,
    handleNewWorkspaceDrop,
    handleWorkspaceDragOver,
    handleWorkspaceDragStart,
    handleWorkspaceDrop,
    setDraggingClosedTabIndex,
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

  const handleTabDrop = (event: DragEvent<HTMLElement>, targetTabId: string, axis: DropAxis = "vertical") => {
    event.preventDefault();
    event.stopPropagation();
    const tabId = getDroppedTabId(event);
    if (!tabId || tabId === targetTabId) {
      setDraggingTabId(null);
      return;
    }

    const placement = getPointerDropPlacement(event.currentTarget, event, axis);
    removeDraggedFavoriteLocation(event);
    placeTab(tabId, targetTabId, placement);
    setDraggingTabId(null);
  };

  const placeTab = (tabId: string, targetTabId: string, placement: "before" | "after") => {
    const draggedTab = activeWorkspace.tabs.find((candidate) => candidate.id === tabId);
    const targetTab = activeWorkspace.tabs.find((candidate) => candidate.id === targetTabId);
    const intent = getSidebarTabDropIntent(draggedTab, targetTab);
    if (intent.type === "pinToPinnedPosition") {
      actions.pinTabToPinnedPosition(tabId, targetTabId, placement);
    } else if (intent.type === "unpinToRegularPosition") {
      actions.unpinTabToRegularPosition(tabId, targetTabId, placement);
    } else {
      actions.reorderTab(tabId, targetTabId, placement);
    }
  };

  const handleTabsDrop = (event: DragEvent<HTMLElement>) => {
    const tabId = getDroppedTabId(event);
    if (!tabId) return;

    const draggedTab = activeWorkspace.tabs.find((candidate) => candidate.id === tabId);
    const intent = getSidebarTabsAreaDropIntent(draggedTab);
    const favoriteId = getDroppedFavoriteId(event);
    if (intent.type !== "unpinToRegularEnd" && !favoriteId) return;

    event.preventDefault();
    if (favoriteId) removeFavoriteLocation(favoriteId);
    if (intent.type === "unpinToRegularEnd") actions.unpinTabToRegularEnd(tabId);
    setDraggingTabId(null);
  };

  const getDroppedTabId = (event: DragEvent<HTMLElement>) => draggingTabId || readSidebarTabDragPayload(event.dataTransfer);
  const getDroppedFavoriteId = (event: DragEvent<HTMLElement>) => draggingFavoriteId || event.dataTransfer.getData("text/favorite-id");
  const removeFavoriteLocation = (favoriteId: string) => {
    actions.removeWorkspaceFavorite(favoriteId);
    setDraggingFavoriteId(null);
  };
  const removeDraggedFavoriteLocation = (event: DragEvent<HTMLElement>) => {
    const favoriteId = getDroppedFavoriteId(event);
    if (favoriteId) removeFavoriteLocation(favoriteId);
  };

  const handlePinDrop = (event: DragEvent<HTMLElement>) => {
    const tabId = getDroppedTabId(event);
    const tab = activeWorkspace.tabs.find((candidate) => candidate.id === tabId);
    if (!tab) return;

    event.preventDefault();
    removeDraggedFavoriteLocation(event);
    if (!tab.isPinned) actions.toggleTabPinned(tab.id);
    setDraggingTabId(null);
  };

  const handleWorkspaceDragOverWithFavorites = (event: DragEvent<HTMLButtonElement>, workspaceId: string) => {
    if (draggingFavoriteId && workspaceId !== activeWorkspace.id) {
      event.preventDefault();
      event.dataTransfer.dropEffect = "move";
      return;
    }

    handleWorkspaceDragOver(event, workspaceId);
  };

  const handleWorkspaceDropWithFavorites = (event: DragEvent<HTMLButtonElement>, workspaceId: string) => {
    const favoriteId = draggingFavoriteId || event.dataTransfer.getData("text/favorite-id");
    if (favoriteId && workspaceId !== activeWorkspace.id) {
      event.preventDefault();
      actions.moveWorkspaceFavoriteToWorkspace(favoriteId, workspaceId);
      setDraggingFavoriteId(null);
      return;
    }

    handleWorkspaceDrop(event, workspaceId);
  };

  const openWorkspaceSettings = (workspaceId: string) => {
    actions.switchWorkspace(workspaceId);
    setPanel("settings");
  };

  const handleNewWorkspaceDropWithFavorites = (event: DragEvent<HTMLButtonElement>) => {
    const favoriteId = draggingFavoriteId || event.dataTransfer.getData("text/favorite-id");
    if (favoriteId) {
      event.preventDefault();
      actions.moveWorkspaceFavoriteToNewWorkspace(favoriteId);
      setDraggingFavoriteId(null);
      return;
    }

    handleNewWorkspaceDrop(event);
  };

  useEffect(() => {
    setTabQuery("");
    setActiveSearchIndex(0);
  }, [activeWorkspace.id]);

  useEffect(() => {
    setActiveSearchIndex((index) => clampSidebarSearchIndex(index, searchTargets.length));
  }, [searchTargets.length]);

  useEffect(() => {
    if (!activeSearchTarget) return;
    scrollSidebarSearchTargetIntoView(activeSearchTarget);
  }, [activeSearchTarget]);

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
    } else if (intent.type === "openUrl") {
      actions.openUrlInActiveWorkspace(intent.url, intent.title);
    } else {
      actions.navigateActiveTab(intent.url);
    }
  }

  return (
    <aside
      className={`sidebar ${sidebarCollapsed || compactMode ? "is-collapsed" : ""} ${compactMode ? "is-compact-mode" : ""} ${floatingSidebarOpen ? "is-floating-open" : ""} ${compactSidebarPeeking ? "is-peeking-chrome" : ""}`}
      style={{ "--sidebar-width": `${sidebarWidth}px` } as CSSProperties}
    >
      <section className="traffic-space" aria-hidden="true" />
      <WorkspaceStrip
        activeWorkspaceId={activeWorkspace.id}
        compactMode={compactMode}
        draggingGroupId={draggingGroupId}
        draggingClosedTabIndex={draggingClosedTabIndex}
        draggingFavoriteId={draggingFavoriteId}
        draggingTabId={draggingTabId}
        draggingWorkspaceId={draggingWorkspaceId}
        floatingSidebarOpen={floatingSidebarOpen}
        sidebarCollapsed={sidebarCollapsed}
        workspaces={state.workspaces}
        onDragEnd={clearWorkspaceDrag}
        onDragOver={handleWorkspaceDragOverWithFavorites}
        onDragStart={handleWorkspaceDragStart}
        onDrop={handleWorkspaceDropWithFavorites}
        onDeleteWorkspace={actions.deleteWorkspace}
        onNewWorkspace={actions.addWorkspace}
        onNewWorkspaceDrop={handleNewWorkspaceDropWithFavorites}
        onOpenSettings={openWorkspaceSettings}
        onSelect={actions.switchWorkspace}
        onToggleSidebar={actions.toggleSidebar}
        onUpdateWorkspace={actions.updateWorkspaceById}
      />

      <section className="tab-stack" onKeyDown={handleSidebarFocusNavigation}>
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
          draggingClosedTabIndex={draggingClosedTabIndex}
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
          onTabLocationDrop={removeDraggedFavoriteLocation}
          onTabsDrop={handleTabsDrop}
          setDraggingEssentialId={setDraggingEssentialId}
          setDraggingFavoriteId={setDraggingFavoriteId}
          setDraggingClosedTabIndex={setDraggingClosedTabIndex}
          setDraggingGroupId={setDraggingGroupId}
          setDraggingTabId={setDraggingTabId}
          workspaceTabs={activeWorkspace.tabs}
        />
      </section>

      <SidebarFooter
        actions={actions}
        activeTabId={activeTab.id}
        closedTabs={activeWorkspace.closedTabs}
        compactMode={compactMode}
        draggingClosedTabIndex={draggingClosedTabIndex}
        draggingEssentialId={draggingEssentialId}
        draggingFavoriteId={draggingFavoriteId}
        draggingTabId={draggingTabId}
        essentials={state.essentials}
        favorites={activeWorkspace.favorites}
        floatingSidebarOpen={floatingSidebarOpen}
        memorySaver={memorySaver}
        setPanel={setPanel}
        setDraggingClosedTabIndex={setDraggingClosedTabIndex}
        setDraggingEssentialId={setDraggingEssentialId}
        setDraggingFavoriteId={setDraggingFavoriteId}
        setDraggingTabId={setDraggingTabId}
        splitLayout={controller.splitLayout}
        splitMode={state.splitMode}
        tabs={activeWorkspace.tabs}
      />
      <SidebarResizeHandle
        isCollapsed={sidebarCollapsed || compactMode}
        width={sidebarWidth}
        onResize={setSidebarWidth}
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
