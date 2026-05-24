import { useEffect, useMemo, useState, type CSSProperties, type DragEvent, type MouseEvent } from "react";

import { getHostInitial, type BrowserTab } from "../../domain/browser-core";
import { getGroupedTabs } from "../../domain/tab-groups";
import type { BrowserController } from "../../hooks/types";
import { TabContextMenu } from "./TabContextMenu";
import { FavoriteButton, TabGroupSection, TabRow } from "./SidebarItems";
import { filterSidebarItems } from "./sidebarFiltering";

interface TabMenuState {
  left: number;
  tab: BrowserTab;
  top: number;
}

export function Sidebar({ controller }: { controller: BrowserController }) {
  const { activeTab, activeWorkspace, actions, setPanel, sidebarCollapsed, state } = controller;
  const [draggingTabId, setDraggingTabId] = useState<string | null>(null);
  const [draggingWorkspaceId, setDraggingWorkspaceId] = useState<string | null>(null);
  const [tabQuery, setTabQuery] = useState("");
  const [tabMenu, setTabMenu] = useState<TabMenuState | null>(null);
  const pinnedTabs = activeWorkspace.tabs.filter((tab) => tab.isPinned);
  const groupedTabs = getGroupedTabs(activeWorkspace);
  const groupedTabIds = new Set(groupedTabs.flatMap((entry) => entry.tabs.map((tab) => tab.id)));
  const regularTabs = activeWorkspace.tabs.filter((tab) => !tab.isPinned && !groupedTabIds.has(tab.id));
  const filteredItems = useMemo(() => filterSidebarItems({
    favorites: activeWorkspace.favorites,
    groupedTabs,
    pinnedTabs,
    regularTabs
  }, tabQuery), [activeWorkspace.favorites, groupedTabs, pinnedTabs, regularTabs, tabQuery]);

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
  }, [activeWorkspace.id]);

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
    <aside className={`sidebar ${sidebarCollapsed ? "is-collapsed" : ""}`}>
      <section className="traffic-space" aria-hidden="true" />
      <section className="workspace-strip" aria-label="Workspaces">
        {state.workspaces.map((workspace) => (
          <button
            className="workspace-button"
            key={workspace.id}
            style={{ "--accent": workspace.accent } as CSSProperties}
            title={workspace.name}
            type="button"
            draggable
            aria-current={workspace.id === state.activeWorkspaceId}
            data-dragging={draggingWorkspaceId === workspace.id}
            data-drop-target={Boolean(
              (draggingTabId && workspace.id !== activeWorkspace.id) ||
              (draggingWorkspaceId && workspace.id !== draggingWorkspaceId)
            )}
            onDragStart={(event) => {
              setDraggingWorkspaceId(workspace.id);
              event.dataTransfer.effectAllowed = "move";
              event.dataTransfer.setData("text/workspace-id", workspace.id);
            }}
            onDragEnd={() => setDraggingWorkspaceId(null)}
            onDragOver={(event) => {
              const isTabTarget = draggingTabId && workspace.id !== activeWorkspace.id;
              const isWorkspaceTarget = draggingWorkspaceId && workspace.id !== draggingWorkspaceId;
              if (isTabTarget || isWorkspaceTarget) {
                event.preventDefault();
                event.dataTransfer.dropEffect = "move";
              }
            }}
            onDrop={(event) => handleWorkspaceDrop(event, workspace.id)}
            onClick={() => actions.switchWorkspace(workspace.id)}
          >
            {workspace.name.slice(0, 1)}
          </button>
        ))}
        <button
          className="workspace-button sidebar-toggle"
          title={sidebarCollapsed ? "Expand sidebar" : "Collapse sidebar"}
          type="button"
          onClick={actions.toggleSidebar}
        >
          {sidebarCollapsed ? "›" : "‹"}
        </button>
      </section>

      <section className="tab-stack">
        <header className="sidebar-header">
          <div>
            <p className="eyebrow">Workspace</p>
            <h1>{activeWorkspace.name}</h1>
          </div>
          <button className="icon-button" title="New tab" type="button" onClick={actions.newTab}>+</button>
        </header>

        <div className="sidebar-search">
          <input
            autoComplete="off"
            spellCheck={false}
            aria-label="Search tabs and favorites"
            placeholder="Search tabs"
            value={tabQuery}
            onChange={(event) => setTabQuery(event.target.value)}
          />
          {tabQuery && (
            <button className="icon-button" title="Clear tab search" type="button" onClick={() => setTabQuery("")}>×</button>
          )}
        </div>

        {filteredItems.pinnedTabs.length > 0 && (
          <nav className="pinned-tabs" aria-label="Pinned tabs">
            {filteredItems.pinnedTabs.map((tab) => (
              <button
                className="pinned-tab-button"
                key={tab.id}
                title={tab.title || tab.url}
                type="button"
                aria-current={tab.id === activeTab.id}
                onClick={() => actions.selectTab(tab.id)}
                onContextMenu={(event) => openTabMenu(event, tab)}
              >
                {tab.isLoading ? "·" : getHostInitial(tab.url)}
              </button>
            ))}
          </nav>
        )}

        {filteredItems.favorites.length > 0 && (
          <nav className="favorites" aria-label="Favorites">
            {filteredItems.favorites.map((favorite) => (
              <FavoriteButton key={favorite.id} favorite={favorite} onOpen={actions.openUrlInActiveWorkspace} />
            ))}
          </nav>
        )}

        <nav className="tabs" aria-label="Tabs">
          {filteredItems.groupedTabs.map(({ group, tabs }) => (
            <TabGroupSection
              key={group.id}
              activeTab={activeTab}
              group={group}
              tabs={tabs}
              draggingTabId={draggingTabId}
              onAssignTab={actions.assignTabToGroup}
              onClose={actions.closeTab}
              onContextMenu={openTabMenu}
              onDrop={handleTabDrop}
              onSelect={actions.selectTab}
              onToggle={() => actions.toggleTabGroupCollapsed(group.id)}
              onUpdate={actions.updateTabGroup}
              setDraggingTabId={setDraggingTabId}
            />
          ))}
          {filteredItems.regularTabs.map((tab) => (
            <TabRow
              key={tab.id}
              activeTabId={activeTab.id}
              draggingTabId={draggingTabId}
              tab={tab}
              onClose={actions.closeTab}
              onContextMenu={openTabMenu}
              onDrop={handleTabDrop}
              onSelect={actions.selectTab}
              setDraggingTabId={setDraggingTabId}
            />
          ))}
          {filteredItems.isFiltering && !filteredItems.hasMatches && (
            <p className="sidebar-empty">No matching tabs</p>
          )}
        </nav>
      </section>

      <footer className="sidebar-footer">
        <button className="toolbar-button" type="button" onClick={actions.toggleSidebar}>Focus</button>
        <button className="toolbar-button" type="button" aria-pressed={state.splitMode} onClick={actions.toggleSplitMode}>Split</button>
        <button className="toolbar-button" type="button" onClick={() => setPanel("history")}>History</button>
        <button className="toolbar-button" type="button" onClick={() => setPanel("downloads")}>Downloads</button>
        <button className="toolbar-button" type="button" onClick={() => setPanel("settings")}>Settings</button>
      </footer>
      {tabMenu && (
        <TabContextMenu
          left={tabMenu.left}
          tab={tabMenu.tab}
          top={tabMenu.top}
          onClose={() => setTabMenu(null)}
          onCloseTab={actions.closeTab}
          onDuplicate={actions.duplicateTab}
          onOpenInSplit={actions.openTabInSplit}
          onSelect={actions.selectTab}
          onSleepTab={actions.sleepTab}
          onToggleMuted={actions.toggleTabMuted}
          onTogglePinned={actions.toggleTabPinned}
        />
      )}
    </aside>
  );
}
