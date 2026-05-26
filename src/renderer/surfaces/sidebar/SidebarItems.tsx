import type { CSSProperties, DragEvent, MouseEvent } from "react";
import { FiLoader, FiMoon, FiX } from "react-icons/fi";

import { getHostInitial, type BrowserTab, type Favorite, type TabGroup } from "../../domain/browser-core";

export function TabGroupSection({
  activeTab,
  draggingTabId,
  group,
  onAssignTab,
  onClose,
  onContextMenu,
  onDrop,
  onPreview,
  onSelect,
  onToggle,
  onUpdate,
  searchSelectedTabId,
  setDraggingTabId,
  splitTabIds,
  tabs
}: {
  activeTab: BrowserTab;
  draggingTabId: string | null;
  group: TabGroup;
  onAssignTab: (tabId: string, groupId: string) => void;
  onClose: (tabId: string) => void;
  onContextMenu: (event: MouseEvent, tab: BrowserTab) => void;
  onDrop: (event: DragEvent<HTMLDivElement>, targetTabId: string) => void;
  onPreview: (url: string, title?: string) => void;
  onSelect: (tabId: string) => void;
  onToggle: () => void;
  onUpdate: (groupId: string, patch: Partial<Pick<TabGroup, "name" | "color">>) => void;
  searchSelectedTabId?: string;
  setDraggingTabId: (tabId: string | null) => void;
  splitTabIds: string[];
  tabs: BrowserTab[];
}) {
  const hasActiveTab = tabs.some((tab) => tab.id === activeTab.id);

  return (
    <section className="tab-group" style={{ "--group-color": group.color } as CSSProperties}>
      <div
        className="tab-group-header"
        data-drop-target={Boolean(draggingTabId)}
        onDragOver={(event) => {
          if (draggingTabId) event.preventDefault();
        }}
        onDrop={(event) => {
          event.preventDefault();
          const tabId = draggingTabId || event.dataTransfer.getData("text/plain");
          if (tabId) onAssignTab(tabId, group.id);
          setDraggingTabId(null);
        }}
      >
        <button
          className="tab-group-toggle"
          type="button"
          aria-expanded={!group.isCollapsed}
          title={group.isCollapsed ? "Expand group" : "Collapse group"}
          onClick={onToggle}
        >
          <span className="tab-group-dot" />
        </button>
        <input
          className="tab-group-title-input"
          aria-label="Tab group name"
          value={group.name}
          onChange={(event) => onUpdate(group.id, { name: event.target.value })}
        />
        <span className="tab-group-count">{tabs.length}</span>
        <input
          className="tab-group-color"
          aria-label="Tab group color"
          type="color"
          value={group.color}
          onChange={(event) => onUpdate(group.id, { color: event.target.value })}
        />
      </div>
      {(!group.isCollapsed || hasActiveTab) && tabs.map((tab) => (
        <TabRow
          key={tab.id}
          activeTabId={activeTab.id}
          draggingTabId={draggingTabId}
          isSearchSelected={searchSelectedTabId === tab.id}
          splitTabIds={splitTabIds}
          tab={tab}
          onClose={onClose}
          onContextMenu={onContextMenu}
          onDrop={onDrop}
          onPreview={onPreview}
          onSelect={onSelect}
          setDraggingTabId={setDraggingTabId}
        />
      ))}
    </section>
  );
}

export function TabRow({
  activeTabId,
  draggingTabId,
  onClose,
  onContextMenu,
  onDrop,
  onPreview,
  onSelect,
  setDraggingTabId,
  isSearchSelected = false,
  splitTabIds,
  tab
}: {
  activeTabId: string;
  draggingTabId: string | null;
  onClose: (tabId: string) => void;
  onContextMenu: (event: MouseEvent, tab: BrowserTab) => void;
  onDrop: (event: DragEvent<HTMLDivElement>, targetTabId: string) => void;
  onPreview: (url: string, title?: string) => void;
  onSelect: (tabId: string) => void;
  setDraggingTabId: (tabId: string | null) => void;
  isSearchSelected?: boolean;
  splitTabIds: string[];
  tab: BrowserTab;
}) {
  return (
    <div
      className={`tab-row ${tab.isSleeping ? "is-sleeping" : ""} ${splitTabIds.includes(tab.id) ? "is-split-tab" : ""}`}
      aria-current={tab.id === activeTabId}
      aria-selected={isSearchSelected}
      draggable
      data-dragging={draggingTabId === tab.id}
      onDragStart={(event) => {
        setDraggingTabId(tab.id);
        event.dataTransfer.effectAllowed = "move";
        event.dataTransfer.setData("text/plain", tab.id);
      }}
      onDragEnd={() => setDraggingTabId(null)}
      onDragOver={(event) => {
        if (draggingTabId && draggingTabId !== tab.id) event.preventDefault();
      }}
      onDrop={(event) => onDrop(event, tab.id)}
      onContextMenu={(event) => onContextMenu(event, tab)}
    >
      <button
        className="tab-button"
        type="button"
        onClick={(event) => {
          event.altKey ? onPreview(tab.url, tab.title) : onSelect(tab.id);
        }}
      >
        <span className="tab-favicon">{tab.isSleeping ? <FiMoon /> : tab.isLoading ? <FiLoader /> : getHostInitial(tab.url)}</span>
        <span className="tab-title">{tab.title || tab.url}</span>
      </button>
      <button
        className="tab-close"
        type="button"
        title="Close tab"
        aria-label={`Close ${tab.title || tab.url}`}
        onClick={() => onClose(tab.id)}
      >
        <FiX />
      </button>
    </div>
  );
}

export function FavoriteButton({
  favorite,
  id,
  isSearchSelected = false,
  onOpen,
  onPreview
}: {
  favorite: Favorite;
  id?: string;
  isSearchSelected?: boolean;
  onOpen: (url: string, title?: string) => void;
  onPreview: (url: string, title?: string) => void;
}) {
  return (
    <button
      className="favorite-button"
      id={id}
      type="button"
      title={favorite.url}
      aria-selected={isSearchSelected}
      onClick={(event) => {
        event.altKey ? onPreview(favorite.url, favorite.title) : onOpen(favorite.url, favorite.title);
      }}
    >
      <span className="favorite-icon">{getHostInitial(favorite.url)}</span>
      <span className="favorite-title">{favorite.title}</span>
    </button>
  );
}
