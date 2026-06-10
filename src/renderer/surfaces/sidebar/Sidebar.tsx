import {
  useCallback,
  useEffect,
  useMemo,
  useRef,
  useState,
  type CSSProperties,
  type DragEvent,
  type KeyboardEvent as ReactKeyboardEvent
} from "react";

import { scrollElementNearEdge } from "../../common/drag-drop/edgeAutoScroll";
import { isListNavigationKey } from "../../common/navigation/listNavigation";
import { getMemorySaverState } from "../../common/memory/memorySaverState";
import {
  DEFAULT_SIDEBAR_SECTION_COLLAPSED,
  toggleSidebarSectionCollapsed,
  type SidebarSectionId
} from "../../common/sidebar/sidebarSections";
import type { BrowserController } from "../../app/controller/types";
import { loadBrowserUiState, saveBrowserUiState } from "../../platform/persistence/browserUiStorage";
import { SidebarResizeHandle } from "./components/chrome/SidebarResizeHandle";
import { SidebarSearchBox } from "./components/chrome/SidebarSearchBox";
import { SidebarContextMenus } from "./components/tabs/SidebarContextMenus";
import { SidebarSections } from "./components/tabs/SidebarSections";
import { useSidebarContextMenus } from "./components/tabs/useSidebarContextMenus";
import { WorkspaceStrip } from "./components/workspaces/WorkspaceStrip";
import { useSidebarDropHandlers } from "./hooks/useSidebarDropHandlers";
import { focusCurrentOrFirstSidebarItem, handleSidebarFocusNavigation, scrollCurrentSidebarItemIntoView } from "./model/sidebarFocusNavigation";
import { scrollSidebarSearchTargetIntoView } from "./model/sidebarSearchTargetDom";
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
  const { activeTab, activeWorkspace, actions, compactMode, compactSidebarPeeking, floatingSidebarOpen, setPanel, setSidebarWidth, sidebarCollapsed, sidebarWidth, splitLayout, state } = controller;
  const tabStackRef = useRef<HTMLElement | null>(null);
  const [tabQuery, setTabQuery] = useState("");
  const [activeSearchIndex, setActiveSearchIndex] = useState(0);
  const [sidebarSectionCollapsed, setSidebarSectionCollapsed] = useState(() => (
    loadBrowserUiState().sidebarSectionCollapsed ?? DEFAULT_SIDEBAR_SECTION_COLLAPSED
  ));
  const {
    openClosedTabMenu,
    closeMenus,
    openQuickEntryMenu,
    openTabGroupMenu,
    openTabMenu,
    quickEntryMenu,
    tabGroupMenu,
    tabMenu,
    closedTabMenu
  } = useSidebarContextMenus();
  const {
    favoriteItems,
    groupedTabs,
    regularTabs
  } = useMemo(() => getSidebarTabFolders(activeWorkspace), [activeWorkspace]);
  const memorySaver = useMemo(() => getMemorySaverState(activeWorkspace, state), [activeWorkspace, state]);
  const filteredItems = useMemo(() => filterSidebarItems({
    essentials: state.essentials,
    favorites: favoriteItems,
    groupedTabs,
    pinnedTabs: [],
    regularTabs,
    workspaceTabs: activeWorkspace.tabs
  }, tabQuery), [favoriteItems, groupedTabs, regularTabs, state.essentials, tabQuery]);
  const searchTargets = useMemo(() => getSidebarSearchTargets(filteredItems), [filteredItems]);
  const activeSearchTarget = filteredItems.isFiltering
    ? searchTargets[clampSidebarSearchIndex(activeSearchIndex, searchTargets.length)]
    : undefined;

  const drop = useSidebarDropHandlers(controller);

  const handleToggleSidebarSection = useCallback((sectionId: SidebarSectionId) => {
    setSidebarSectionCollapsed((current) => {
      const next = toggleSidebarSectionCollapsed(current, sectionId);
      saveBrowserUiState({ sidebarSectionCollapsed: next });
      return next;
    });
  }, []);

  const openWorkspaceSettings = (workspaceId: string) => {
    actions.switchWorkspace(workspaceId);
    setPanel("settings");
  };

  const handleSidebarScrollAreaDragOver = (event: DragEvent<HTMLDivElement>) => {
    if (!drop.hasScrollableSidebarDrag(event)) return;
    if (scrollElementNearEdge(event.currentTarget, event.clientY)) {
      event.preventDefault();
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

  useEffect(() => {
    if (tabQuery) return;
    scrollCurrentSidebarItemIntoView(tabStackRef.current);
  }, [activeTab.id, activeWorkspace.id, tabQuery]);

  const runSidebarIntent = useCallback((intent: SidebarOpenIntent) => {
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
  }, [actions]);

  const runSearchTarget = useCallback((target: SidebarSearchTarget, modifiers: { altKey: boolean; shiftKey: boolean }) => {
    runSidebarIntent(getSidebarSearchOpenIntent(target, modifiers));
  }, [runSidebarIntent]);

  function onSearchKeyDown(event: ReactKeyboardEvent<HTMLInputElement>) {
    if (isListNavigationKey(event.key)) {
      event.preventDefault();
      const navKey = event.key;
      setActiveSearchIndex((index) => getNextSidebarSearchIndex(index, searchTargets.length, navKey));
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
    } else if (event.key === "Escape") {
      event.preventDefault();
      focusCurrentOrFirstSidebarItem(tabStackRef.current);
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
        draggingGroupId={drop.draggingGroupId}
        draggingClosedTabIndex={drop.draggingClosedTabIndex}
        draggingFavoriteId={drop.draggingFavoriteId}
        draggingTabId={drop.draggingTabId}
        draggingWorkspaceId={drop.draggingWorkspaceId}
        floatingSidebarOpen={floatingSidebarOpen}
        memorySaver={memorySaver}
        sidebarCollapsed={sidebarCollapsed}
        splitLayout={splitLayout}
        splitMode={state.splitMode}
        workspaces={state.workspaces}
        onDragEnd={drop.clearWorkspaceDrag}
        onDragOver={drop.handleWorkspaceDragOver}
        onDragStart={drop.handleWorkspaceDragStart}
        onDrop={drop.handleWorkspaceDrop}
        onDeleteWorkspace={actions.deleteWorkspace}
        onNewWorkspace={actions.addWorkspace}
        onNewWorkspaceDrop={drop.handleNewWorkspaceDrop}
        onOpenSettings={openWorkspaceSettings}
        onSelect={actions.switchWorkspace}
        onSetPanel={setPanel}
        onSetSplitLayout={actions.setSplitLayout}
        onSleepInactiveTabs={actions.sleepInactiveTabs}
        onToggleCompactMode={actions.toggleCompactMode}
        onToggleSidebar={actions.toggleSidebar}
        onToggleSplitMode={actions.toggleSplitMode}
        onUpdateWorkspace={actions.updateWorkspaceById}
      />

      <section className="tab-stack" ref={tabStackRef} onKeyDown={handleSidebarFocusNavigation}>
        <SidebarSearchBox
          activeSearchTarget={activeSearchTarget}
          query={tabQuery}
          resultCount={filteredItems.isFiltering ? searchTargets.length : undefined}
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
        <div className="sidebar-scroll-area" onDragOver={handleSidebarScrollAreaDragOver}>
          <SidebarSections
            actions={actions}
            activeSearchTarget={activeSearchTarget}
            activeTab={activeTab}
            collapsedSections={sidebarSectionCollapsed}
            draggingEssentialId={drop.draggingEssentialId}
            faviconCache={state.faviconCache}
            draggingFavoriteId={drop.draggingFavoriteId}
            draggingGroupId={drop.draggingGroupId}
            draggingTabId={drop.draggingTabId}
            filteredItems={filteredItems}
            onEssentialDragStart={drop.handleEssentialDragStart}
            onEssentialDrop={drop.handleEssentialDrop}
            onEssentialReorderDrop={drop.handleEssentialReorderDrop}
            onFavoriteDragStart={drop.handleFavoriteDragStart}
            onFavoriteDrop={drop.handleFavoritesDrop}
            onFavoriteReorderDrop={drop.handleFavoriteReorderDrop}
            onFavoriteTabDrop={drop.handleFavoriteTabDrop}
            onTabGroupContextMenu={openTabGroupMenu}
            splitTabIds={state.splitTabIds}
            onQuickEntryContextMenu={openQuickEntryMenu}
            onRenameGroup={(groupId, name) => actions.updateTabGroup(groupId, { name })}
            onRenameTab={(tabId, customTitle) => actions.updateTab(tabId, { customTitle })}
            onTabContextMenu={openTabMenu}
            onTabDrop={drop.handleTabDrop}
            onTabGroupCreate={drop.handleTabGroupCreate}
            onTabsDrop={drop.handleTabsDrop}
            onToggleSection={handleToggleSidebarSection}
            setDraggingEssentialId={drop.setDraggingEssentialId}
            setDraggingFavoriteId={drop.setDraggingFavoriteId}
            setDraggingGroupId={drop.setDraggingGroupId}
            setDraggingTabId={drop.setDraggingTabId}
            workspaceTabs={activeWorkspace.tabs}
          />
        </div>
      </section>

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
