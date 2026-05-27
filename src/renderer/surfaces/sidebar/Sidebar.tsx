import {
  useEffect,
  useMemo,
  useState,
  type DragEvent,
  type KeyboardEvent as ReactKeyboardEvent,
  type MouseEvent
} from "react";

import { isListNavigationKey } from "../../common/navigation/listNavigation";
import { isEssential, isFavorite, type BrowserTab } from "../../domain/browser-core";
import { getGroupedTabs } from "../../domain/tab-groups";
import type { BrowserController } from "../../app/controller/types";
import { SidebarAddress } from "./components/SidebarAddress";
import { SidebarFooter } from "./components/SidebarFooter";
import { SidebarHeader } from "./components/SidebarHeader";
import { SidebarSearchBox } from "./components/SidebarSearchBox";
import { SidebarSections } from "./components/SidebarSections";
import { TabContextMenu } from "./components/TabContextMenu";
import { WorkspaceStrip } from "./components/WorkspaceStrip";
import { getMoveWorkspaceTargets, getTabCleanupState, getTabGroupMenuState } from "./model/tabContextMenuState";
import {
  clampSidebarSearchIndex,
  filterSidebarItems,
  getNextSidebarSearchIndex,
  getSidebarSearchTargets,
  type SidebarSearchTarget
} from "./sidebarFiltering";
import { getSidebarSearchOpenIntent, type SidebarOpenIntent } from "./sidebarOpenIntent";

interface TabMenuState {
  left: number;
  tab: BrowserTab;
  top: number;
}

export function Sidebar({ controller }: { controller: BrowserController }) {
  const { activeTab, activeWorkspace, actions, compactChromePeeking, compactMode, floatingSidebarOpen, setPanel, sidebarCollapsed, state } = controller;
  const [draggingTabId, setDraggingTabId] = useState<string | null>(null);
  const [draggingWorkspaceId, setDraggingWorkspaceId] = useState<string | null>(null);
  const [tabQuery, setTabQuery] = useState("");
  const [activeSearchIndex, setActiveSearchIndex] = useState(0);
  const [tabMenu, setTabMenu] = useState<TabMenuState | null>(null);
  const pinnedTabs = activeWorkspace.tabs.filter((tab) => tab.isPinned);
  const groupedTabs = getGroupedTabs(activeWorkspace);
  const groupedTabIds = new Set(groupedTabs.flatMap((entry) => entry.tabs.map((tab) => tab.id)));
  const regularTabs = activeWorkspace.tabs.filter((tab) => !tab.isPinned && !groupedTabIds.has(tab.id));
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

  const handleTabDrop = (event: DragEvent<HTMLDivElement>, targetTabId: string) => {
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

  const openTabMenu = (event: MouseEvent, tab: BrowserTab) => {
    event.preventDefault();
    setTabMenu({
      left: Math.min(event.clientX, window.innerWidth - 190),
      tab,
      top: Math.min(event.clientY, window.innerHeight - 260)
    });
  };

  useEffect(() => {
    if (!tabMenu) return;

    const close = () => setTabMenu(null);
    const closeOnEscape = (event: KeyboardEvent) => {
      if (event.key === "Escape") close();
    };

    window.addEventListener("click", close);
    window.addEventListener("blur", close);
    window.addEventListener("keydown", closeOnEscape);
    window.addEventListener("scroll", close, true);
    return () => {
      window.removeEventListener("click", close);
      window.removeEventListener("blur", close);
      window.removeEventListener("keydown", closeOnEscape);
      window.removeEventListener("scroll", close, true);
    };
  }, [tabMenu]);

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
        draggingTabId={draggingTabId}
        draggingWorkspaceId={draggingWorkspaceId}
        sidebarCollapsed={sidebarCollapsed}
        workspaces={state.workspaces}
        onDragEnd={() => setDraggingWorkspaceId(null)}
        onDragOver={handleWorkspaceDragOver}
        onDragStart={handleWorkspaceDragStart}
        onDrop={handleWorkspaceDrop}
        onNewWorkspace={actions.addWorkspace}
        onSelect={actions.switchWorkspace}
        onToggleSidebar={actions.toggleSidebar}
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
          draggingTabId={draggingTabId}
          filteredItems={filteredItems}
          splitTabIds={state.splitTabIds}
          onTabContextMenu={openTabMenu}
          onTabDrop={handleTabDrop}
          setDraggingTabId={setDraggingTabId}
        />
      </section>

      <SidebarFooter actions={actions} compactMode={compactMode} setPanel={setPanel} splitMode={state.splitMode} />
      {tabMenu && (
        <TabContextMenu
          left={tabMenu.left}
          cleanupState={getTabCleanupState(activeWorkspace.tabs, tabMenu.tab.id)}
          groupMenuState={getTabGroupMenuState(activeWorkspace.tabGroups, tabMenu.tab)}
          moveWorkspaceTargets={getMoveWorkspaceTargets(state.workspaces, activeWorkspace.id)}
          tab={tabMenu.tab}
          top={tabMenu.top}
          onClose={() => setTabMenu(null)}
          onCloseTab={actions.closeTab}
          onCloseOtherTabs={actions.closeOtherTabs}
          onCloseTabsToLeft={actions.closeTabsToLeft}
          onCloseTabsToRight={actions.closeTabsToRight}
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
    </aside>
  );
}
