import type { CSSProperties, DragEvent, MouseEvent } from "react";

import { getHostInitial, type BrowserTab, type Favorite, type TabGroup } from "../../domain/browser-core";

export function TabGroupSection({
  activeTab,
  draggingTabId,
  group,
  onAssignTab,
  onClose,
  onContextMenu,
  onDrop,
  onSelect,
  onToggle,
  onUpdate,
  setDraggingTabId,
  tabs
}: {
  activeTab: BrowserTab;
  draggingTabId: string | null;
  group: TabGroup;
  onAssignTab: (tabId: string, groupId: string) => void;
  onClose: (tabId: string) => void;
  onContextMenu: (event: MouseEvent, tab: BrowserTab) => void;
  onDrop: (event: DragEvent<HTMLDivElement>, targetTabId: string) => void;
  onSelect: (tabId: string) => void;
  onToggle: () => void;
  onUpdate: (groupId: string, patch: Partial<Pick<TabGroup, "name" | "color">>) => void;
  setDraggingTabId: (tabId: string | null) => void;
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
          tab={tab}
          onClose={onClose}
          onContextMenu={onContextMenu}
          onDrop={onDrop}
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
  onSelect,
  setDraggingTabId,
  tab
}: {
  activeTabId: string;
  draggingTabId: string | null;
  onClose: (tabId: string) => void;
  onContextMenu: (event: MouseEvent, tab: BrowserTab) => void;
  onDrop: (event: DragEvent<HTMLDivElement>, targetTabId: string) => void;
  onSelect: (tabId: string) => void;
  setDraggingTabId: (tabId: string | null) => void;
  tab: BrowserTab;
}) {
  return (
    <div
      className={`tab-row ${tab.isSleeping ? "is-sleeping" : ""}`}
      aria-current={tab.id === activeTabId}
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
      <button className="tab-button" type="button" onClick={() => onSelect(tab.id)}>
        <span className="tab-favicon">{tab.isSleeping ? "z" : tab.isLoading ? "." : getHostInitial(tab.url)}</span>
        <span className="tab-title">{tab.title || tab.url}</span>
      </button>
      <button
        className="tab-close"
        type="button"
        title="Close tab"
        aria-label={`Close ${tab.title || tab.url}`}
        onClick={() => onClose(tab.id)}
      >
        x
      </button>
    </div>
  );
}

export function FavoriteButton({ favorite, onOpen }: { favorite: Favorite; onOpen: (url: string, title?: string) => void }) {
  return (
    <button className="favorite-button" type="button" title={favorite.url} onClick={() => onOpen(favorite.url, favorite.title)}>
      <span className="favorite-icon">{getHostInitial(favorite.url)}</span>
      <span className="favorite-title">{favorite.title}</span>
    </button>
  );
}
