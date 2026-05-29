import {
  useEffect,
  useMemo,
  useState,
  type CSSProperties,
  type DragEvent,
  type KeyboardEvent as ReactKeyboardEvent
} from "react";

import { getPointerDropPlacement, type DropAxis } from "../../common/drag-drop/dropPlacement";
import { isListNavigationKey } from "../../common/navigation/listNavigation";
import { getMemorySaverState } from "../../common/memory/memorySaverState";
import type { BrowserController } from "../../app/controller/types";
import type { TabFolder } from "../../domain/tabs";
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
import { readSidebarTabDragEventId, type SidebarDragState } from "./model/sidebarDragSources";
import { scrollSidebarSearchTargetIntoView } from "./model/sidebarSearchTargetDom";
import { getSidebarTabFolders } from "./model/sidebarTabFolders";
import { getSidebarNewWorkspaceDropIntent, getSidebarWorkspaceDropIntent, type SidebarWorkspaceDropIntent } from "./model/sidebarWorkspaceDropIntent";
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
    handleWorkspaceDragStart,
    setDraggingClosedTabIndex,
    setDraggingGroupId,
    setDraggingTabId,
    setDraggingWorkspaceId
  } = useSidebarWorkspaceDrag();
  const {
    draggingEssentialId,
    draggingFavoriteId,
    handleEssentialDragStart,
    handleEssentialDrop,
    handleEssentialReorderDrop,
    handleFavoriteDragStart,
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
    placeTab(tabId, targetTabId, placement);
    setDraggingTabId(null);
    setDraggingFavoriteId(null);
  };

  const placeTab = (tabId: string, targetTabId: string, placement: "before" | "after") => {
    actions.moveTabToFolderPosition(tabId, targetTabId, placement);
  };

  const handleTabFolderDrop = (event: DragEvent<HTMLElement>, folder: TabFolder) => {
    const tabId = getDroppedTabId(event);
    if (!tabId) return;

    event.preventDefault();
    event.stopPropagation();
    actions.moveTabToFolderEnd(tabId, folder);
    setDraggingTabId(null);
    setDraggingFavoriteId(null);
  };

  const handleFavoritesDrop = (event: DragEvent<HTMLElement>) => {
    handleTabFolderDrop(event, { type: "favorites" });
  };

  const handleTabsDrop = (event: DragEvent<HTMLElement>) => {
    handleTabFolderDrop(event, { type: "tabs" });
  };

  const getDroppedTabId = (event: DragEvent<HTMLElement>) => readSidebarTabDragEventId({ draggingTabId }, event.dataTransfer);

  const handlePinDrop = (event: DragEvent<HTMLElement>) => {
    handleTabFolderDrop(event, { type: "pinned" });
  };

  const handleWorkspaceDragOver = (event: DragEvent<HTMLButtonElement>, workspaceId: string) => {
    const intent = getSidebarWorkspaceDropIntent({
      ...getWorkspaceDragState(),
      activeWorkspaceId: activeWorkspace.id,
      targetWorkspaceId: workspaceId
    }, (type) => event.dataTransfer.getData(type));
    if (intent) {
      event.preventDefault();
      event.dataTransfer.dropEffect = "move";
    }
  };

  const handleWorkspaceDrop = (event: DragEvent<HTMLButtonElement>, workspaceId: string) => {
    const intent = getSidebarWorkspaceDropIntent({
      ...getWorkspaceDragState(),
      activeWorkspaceId: activeWorkspace.id,
      targetWorkspaceId: workspaceId
    }, (type) => event.dataTransfer.getData(type));
    if (!intent) return;

    event.preventDefault();
    runWorkspaceDropIntent(intent, event, workspaceId);
    clearSidebarDropState();
  };

  const openWorkspaceSettings = (workspaceId: string) => {
    actions.switchWorkspace(workspaceId);
    setPanel("settings");
  };

  const handleNewWorkspaceDrop = (event: DragEvent<HTMLButtonElement>) => {
    const intent = getSidebarNewWorkspaceDropIntent(
      getWorkspaceDragState(),
      (type) => event.dataTransfer.getData(type)
    );
    if (!intent) return;

    event.preventDefault();
    runNewWorkspaceDropIntent(intent);
    clearSidebarDropState();
  };

  const getWorkspaceDragState = (): Required<SidebarDragState> => ({
    draggingClosedTabIndex,
    draggingEssentialId,
    draggingFavoriteId,
    draggingGroupId,
    draggingTabId,
    draggingWorkspaceId
  });

  const clearSidebarDropState = () => {
    setDraggingClosedTabIndex(null);
    setDraggingEssentialId(null);
    setDraggingFavoriteId(null);
    setDraggingGroupId(null);
    setDraggingTabId(null);
    setDraggingWorkspaceId(null);
  };

  const runWorkspaceDropIntent = (
    intent: SidebarWorkspaceDropIntent,
    event: DragEvent<HTMLButtonElement>,
    workspaceId: string
  ) => {
    if (intent.type === "workspace") {
      actions.reorderWorkspace(intent.workspaceId, workspaceId, getPointerDropPlacement(event.currentTarget, event, "vertical"));
    } else if (intent.type === "closedTab") {
      actions.restoreClosedTabToWorkspace(intent.closedTabIndex, workspaceId);
    } else if (intent.type === "favorite") {
      actions.moveWorkspaceFavoriteToWorkspace(intent.favoriteId, workspaceId);
    } else if (intent.type === "group") {
      actions.moveTabGroupToWorkspace(intent.groupId, workspaceId);
    } else {
      actions.moveTabToWorkspace(intent.tabId, workspaceId);
    }
  };

  const runNewWorkspaceDropIntent = (intent: Exclude<SidebarWorkspaceDropIntent, { type: "workspace" }>) => {
    if (intent.type === "closedTab") {
      actions.restoreClosedTabToNewWorkspace(intent.closedTabIndex);
    } else if (intent.type === "favorite") {
      actions.moveWorkspaceFavoriteToNewWorkspace(intent.favoriteId);
    } else if (intent.type === "group") {
      actions.moveTabGroupToNewWorkspace(intent.groupId);
    } else {
      actions.moveTabToNewWorkspace(intent.tabId);
    }
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
        onDragOver={handleWorkspaceDragOver}
        onDragStart={handleWorkspaceDragStart}
        onDrop={handleWorkspaceDrop}
        onDeleteWorkspace={actions.deleteWorkspace}
        onNewWorkspace={actions.addWorkspace}
        onNewWorkspaceDrop={handleNewWorkspaceDrop}
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
          onFavoriteDrop={handleFavoritesDrop}
          onFavoriteReorderDrop={handleFavoriteReorderDrop}
          onClosedTabContextMenu={openClosedTabMenu}
          onTabGroupContextMenu={openTabGroupMenu}
          splitTabIds={state.splitTabIds}
          onQuickEntryContextMenu={openQuickEntryMenu}
          onPinDrop={handlePinDrop}
          onTabContextMenu={openTabMenu}
          onTabDrop={handleTabDrop}
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
