import {
  useEffect,
  useMemo,
  useState,
  type CSSProperties,
  type DragEvent,
  type FormEvent,
  type KeyboardEvent as ReactKeyboardEvent,
  type MouseEvent
} from "react";
import {
  FiChevronLeft,
  FiChevronRight,
  FiClock,
  FiColumns,
  FiDownload,
  FiLoader,
  FiMinimize2,
  FiPlus,
  FiSearch,
  FiSettings,
  FiSidebar,
  FiX
} from "react-icons/fi";

import { getHostInitial, type BrowserTab } from "../../domain/browser-core";
import { getGroupedTabs } from "../../domain/tab-groups";
import type { BrowserController } from "../../hooks/types";
import { buildOmniboxSuggestions, type OmniboxSuggestion } from "../../hooks/omniboxSuggestions";
import {
  clampOmniboxIndex,
  getNextOmniboxIndex,
  type OmniboxNavigationKey
} from "../../hooks/omniboxSelection";
import { TabContextMenu } from "./TabContextMenu";
import { FavoriteButton, TabGroupSection, TabRow } from "./SidebarItems";
import { filterSidebarItems } from "./sidebarFiltering";

interface TabMenuState {
  left: number;
  tab: BrowserTab;
  top: number;
}

export function Sidebar({ controller }: { controller: BrowserController }) {
  const { activeTab, activeWorkspace, actions, compactMode, setPanel, sidebarCollapsed, state } = controller;
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
    <aside className={`sidebar ${sidebarCollapsed || compactMode ? "is-collapsed" : ""} ${compactMode ? "is-compact-mode" : ""}`}>
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
          {sidebarCollapsed ? <FiChevronRight /> : <FiChevronLeft />}
        </button>
      </section>

      <section className="tab-stack">
        <header className="sidebar-header">
          <div>
            <p className="eyebrow">Workspace</p>
            <h1>{activeWorkspace.name}</h1>
          </div>
          <button className="icon-button" title="New tab" type="button" onClick={actions.newTab}><FiPlus /></button>
        </header>

        <SidebarAddress controller={controller} />

        {state.essentials.length > 0 && (
          <nav className="essentials" aria-label="Essentials">
            {state.essentials.map((essential) => (
              <FavoriteButton
                key={essential.id}
                favorite={essential}
                onOpen={actions.openUrlInActiveWorkspace}
                onPreview={actions.openGlance}
              />
            ))}
          </nav>
        )}

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
            <button className="icon-button" title="Clear tab search" type="button" onClick={() => setTabQuery("")}><FiX /></button>
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
                onClick={(event) => {
                  event.altKey ? actions.openGlance(tab.url, tab.title) : actions.selectTab(tab.id);
                }}
                onContextMenu={(event) => openTabMenu(event, tab)}
              >
                {tab.isLoading ? <FiLoader /> : getHostInitial(tab.url)}
              </button>
            ))}
          </nav>
        )}

        {filteredItems.favorites.length > 0 && (
          <nav className="favorites" aria-label="Favorites">
            {filteredItems.favorites.map((favorite) => (
              <FavoriteButton
                key={favorite.id}
                favorite={favorite}
                onOpen={actions.openUrlInActiveWorkspace}
                onPreview={actions.openGlance}
              />
            ))}
          </nav>
        )}

        <nav className="tabs" aria-label="Tabs">
          {filteredItems.groupedTabs.map(({ group, tabs }) => (
            <TabGroupSection
              key={group.id}
              activeTab={activeTab}
              group={group}
              splitTabIds={state.splitTabIds}
              tabs={tabs}
              draggingTabId={draggingTabId}
              onAssignTab={actions.assignTabToGroup}
              onClose={actions.closeTab}
              onContextMenu={openTabMenu}
              onDrop={handleTabDrop}
              onPreview={actions.openGlance}
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
              splitTabIds={state.splitTabIds}
              tab={tab}
              onClose={actions.closeTab}
              onContextMenu={openTabMenu}
              onDrop={handleTabDrop}
              onPreview={actions.openGlance}
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
        <button className="icon-button" title="Focus sidebar" type="button" onClick={actions.toggleSidebar}><FiSidebar /></button>
        <button className="icon-button" title="Compact mode" type="button" aria-pressed={compactMode} onClick={actions.toggleCompactMode}><FiMinimize2 /></button>
        <button className="icon-button" title="Split view" type="button" aria-pressed={state.splitMode} onClick={actions.toggleSplitMode}><FiColumns /></button>
        <button className="icon-button" title="History" type="button" onClick={() => setPanel("history")}><FiClock /></button>
        <button className="icon-button" title="Downloads" type="button" onClick={() => setPanel("downloads")}><FiDownload /></button>
        <button className="icon-button" title="Settings" type="button" onClick={() => setPanel("settings")}><FiSettings /></button>
      </footer>
      {tabMenu && (
        <TabContextMenu
          left={tabMenu.left}
          tab={tabMenu.tab}
          top={tabMenu.top}
          onClose={() => setTabMenu(null)}
          onCloseTab={actions.closeTab}
          onDuplicate={actions.duplicateTab}
          onOpenGlance={actions.openGlance}
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

function SidebarAddress({ controller }: { controller: BrowserController }) {
  const { actions, addressValue, compactMode, setAddressValue, state } = controller;
  const [suggestionsOpen, setSuggestionsOpen] = useState(false);
  const [activeSuggestionIndex, setActiveSuggestionIndex] = useState(0);
  const suggestions = useMemo(() => buildOmniboxSuggestions(state, addressValue), [addressValue, state]);
  const activeIndex = clampOmniboxIndex(activeSuggestionIndex, suggestions.length);

  useEffect(() => {
    setActiveSuggestionIndex((index) => clampOmniboxIndex(index, suggestions.length));
  }, [suggestions.length]);

  function submitAddress(event: FormEvent) {
    event.preventDefault();
    runSuggestion(suggestionsOpen ? suggestions[activeIndex] : undefined);
  }

  function onAddressKeyDown(event: ReactKeyboardEvent<HTMLInputElement>) {
    if (!suggestionsOpen && isOmniboxNavigationKey(event.key)) {
      setSuggestionsOpen(true);
    }

    if (isOmniboxNavigationKey(event.key)) {
      event.preventDefault();
      const key = event.key;
      setActiveSuggestionIndex((index) => getNextOmniboxIndex(index, suggestions.length, key));
    } else if (event.key === "Enter") {
      event.preventDefault();
      runSuggestion(suggestionsOpen ? suggestions[activeIndex] : undefined);
    } else if (event.key === "Escape") {
      setSuggestionsOpen(false);
    }
  }

  function onSuggestionPointerDown(event: MouseEvent, suggestion: OmniboxSuggestion) {
    event.preventDefault();
    runSuggestion(suggestion);
  }

  function runSuggestion(suggestion: OmniboxSuggestion | undefined) {
    switch (suggestion?.type) {
      case "tab":
        actions.selectTab(suggestion.tabId);
        break;
      case "essential":
      case "favorite":
      case "history":
        actions.navigateActiveTab(suggestion.url);
        break;
      case "navigate":
        actions.navigateActiveTab(suggestion.value);
        break;
      default:
        actions.navigateActiveTab(addressValue);
    }
    setSuggestionsOpen(false);
  }

  return (
    <div className="sidebar-address" data-compact={compactMode}>
      <form className="sidebar-address-form" onSubmit={submitAddress}>
        <FiSearch />
        <input
          id="sidebarAddressInput"
          autoComplete="off"
          inputMode="url"
          spellCheck={false}
          aria-label="Sidebar address"
          placeholder="Search or enter address"
          value={addressValue}
          onBlur={() => setSuggestionsOpen(false)}
          onChange={(event) => {
            setAddressValue(event.target.value);
            setSuggestionsOpen(true);
            setActiveSuggestionIndex(0);
          }}
          onFocus={() => setSuggestionsOpen(true)}
          onKeyDown={onAddressKeyDown}
          aria-activedescendant={suggestionsOpen && suggestions.length > 0 ? `sidebar-address-suggestion-${activeIndex}` : undefined}
        />
      </form>
      {suggestionsOpen && suggestions.length > 0 && (
        <div className="sidebar-omnibox-suggestions" role="listbox" aria-label="Sidebar address suggestions">
          {suggestions.map((suggestion, index) => (
            <button
              className="sidebar-omnibox-suggestion"
              id={`sidebar-address-suggestion-${index}`}
              key={suggestion.id}
              type="button"
              aria-selected={index === activeIndex}
              onMouseDown={(event) => onSuggestionPointerDown(event, suggestion)}
              onMouseEnter={() => setActiveSuggestionIndex(index)}
            >
              <span>{suggestion.title}</span>
              <small>{suggestion.subtitle}</small>
            </button>
          ))}
        </div>
      )}
    </div>
  );
}

function isOmniboxNavigationKey(key: string): key is OmniboxNavigationKey {
  return key === "ArrowDown" || key === "ArrowUp" || key === "End" || key === "Home";
}
