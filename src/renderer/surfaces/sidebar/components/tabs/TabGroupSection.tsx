/* eslint-disable max-lines -- Legacy Electron prototype migration reference; direct Chromium work must not expand this file. */
import { useEffect, useRef, useState, type CSSProperties, type DragEvent, type KeyboardEvent, type MouseEvent } from "react";
import { FiFolder } from "react-icons/fi";

import { getDisclosureKeyboardToggleIntent } from "../../../../common/disclosure/disclosureKeyboard";
import { clearDropTargetActive, type DropAxis } from "../../../../common/drag-drop/dropPlacement";
import type { BrowserTab, FaviconCache, SplitTab, TabGroup } from "../../../../domain/browser";
import { SIDEBAR_DRAG_DATA } from "../../model/sidebarDragSources";
import { openSidebarKeyboardContextMenu } from "../../model/sidebarKeyboardContextMenu";
import { clearSidebarRowReorderDrop } from "../../model/sidebarRowReorderDrop";
import { acceptSidebarTabGroupHeaderDrag, resolveSidebarTabGroupHeaderDrop } from "../../model/sidebarTabGroupHeaderDrop";
import { getSidebarSearchTargetElementId } from "../../sidebarFiltering";
import { SidebarItemIcon } from "../common/SidebarItemIcon";
import { TabRow } from "./SidebarItems";
import { SplitTabRow } from "./SplitTabRow";

export function TabGroupSection({
  activeTab,
  activeSplitId,
  draggingGroupId,
  draggingTabId,
  faviconCache,
  group,
  isCrossFolderDrag,
  onClose,
  onContextMenu,
  onGroupDrop,
  onGroupContextMenu,
  onDrop,
  onGroupTab,
  onMoveTabToGroupFolder,
  onPreview,
  onRenameGroup,
  onRenameTab,
  onSelect,
  onSplit,
  onSwapSplitPanes,
  onToggle,
  searchSelectedTabId,
  setDraggingGroupId,
  setDraggingTabId,
  splitTabs,
  workspaceTabs,
  tabs
}: {
  activeTab: BrowserTab;
  activeSplitId: string | null;
  draggingGroupId: string | null;
  draggingTabId: string | null;
  faviconCache?: FaviconCache;
  group: TabGroup;
  isCrossFolderDrag?: (targetTab: BrowserTab) => (draggedId: string) => boolean;
  onClose: (tabId: string) => void;
  onContextMenu: (event: MouseEvent, tab: BrowserTab) => void;
  onGroupDrop: (event: DragEvent<HTMLElement>, targetGroupId: string) => void;
  onGroupContextMenu: (event: MouseEvent, group: TabGroup) => void;
  onDrop: (event: DragEvent<HTMLElement>, targetTabId: string, axis?: DropAxis) => void;
  onGroupTab?: (sourceTabId: string, targetTabId: string) => void;
  onMoveTabToGroupFolder: (tabId: string, groupId: string) => void;
  onPreview: (url: string, title?: string) => void;
  onRenameGroup?: (groupId: string, customName: string | undefined) => void;
  onRenameTab?: (tabId: string, customTitle: string | undefined) => void;
  onSelect: (tabId: string) => void;
  onSplit: (tabId: string) => void;
  onSwapSplitPanes?: (splitId: string) => void;
  onToggle: () => void;
  searchSelectedTabId?: string;
  setDraggingGroupId: (groupId: string | null) => void;
  setDraggingTabId: (tabId: string | null) => void;
  splitTabs: SplitTab[];
  workspaceTabs: BrowserTab[];
  tabs: BrowserTab[];
}) {
  const hasActiveTab = tabs.some((tab) => tab.id === activeTab.id);
  const tabGroupHeaderDropState = { draggingGroupId, draggingTabId };

  // Inline rename state
  const [isRenaming, setIsRenaming] = useState(false);
  const [renameDraft, setRenameDraft] = useState(group.name);
  const renameInputRef = useRef<HTMLInputElement | null>(null);
  useEffect(() => {
    if (isRenaming) {
      renameInputRef.current?.focus();
      renameInputRef.current?.select();
    }
  }, [isRenaming]);
  const commitRename = () => {
    if (!isRenaming) return;
    const trimmed = renameDraft.trim();
    onRenameGroup?.(group.id, trimmed ? trimmed : undefined);
    setIsRenaming(false);
  };
  const cancelRename = () => {
    if (!isRenaming) return;
    setRenameDraft(group.name);
    setIsRenaming(false);
  };

  const handleToggleKeyDown = (event: KeyboardEvent<HTMLButtonElement>) => {
    if (openSidebarKeyboardContextMenu(event)) return;

    const intent = getDisclosureKeyboardToggleIntent(event.key, group.isCollapsed);
    if (!intent) return;

    event.preventDefault();
    event.stopPropagation();
    onToggle();
  };

  const findSplitForTab = (tabId: string): SplitTab | undefined => {
    return splitTabs.find((s) => s.primaryTabId === tabId || s.secondaryTabId === tabId);
  };

  const renderTabOrSplit = (tab: BrowserTab) => {
    const split = findSplitForTab(tab.id);
    if (split && split.secondaryTabId === tab.id) {
      return null; // secondary tab rendered as part of primary's SplitTabRow
    }
    if (split && split.primaryTabId === tab.id) {
      const secondaryTab = workspaceTabs.find((t) => t.id === split.secondaryTabId);
      if (!secondaryTab) return null;
      return (
        <SplitTabRow
          key={`split-${split.id}`}
          activeTabId={activeTab.id}
          draggingTabId={draggingTabId}
          faviconCache={faviconCache}
          id={getSidebarSearchTargetElementId({ type: "tab", id: tab.id, title: tab.title || tab.url, url: tab.url })}
          isActive={activeSplitId === split.id}
          isSearchSelected={searchSelectedTabId === tab.id}
          primaryTab={tab}
          secondaryTab={secondaryTab}
          onClose={onClose}
          onContextMenu={onContextMenu}
          onDrop={onDrop}
          onPreview={onPreview}
          onSelect={() => onSelect(split.primaryTabId)}
          onSwapPanes={() => onSwapSplitPanes?.(split.id)}
          setDraggingTabId={setDraggingTabId}
        />
      );
    }
    return (
      <TabRow
        key={tab.id}
        activeTabId={activeTab.id}
        draggingTabId={draggingTabId}
        faviconCache={faviconCache}
        id={getSidebarSearchTargetElementId({ type: "tab", id: tab.id, title: tab.title || tab.url, url: tab.url })}
        isCrossFolderDrag={isCrossFolderDrag?.(tab)}
        isSearchSelected={searchSelectedTabId === tab.id}
        splitTabIds={[]}
        tab={tab}
        onClose={onClose}
        onContextMenu={onContextMenu}
        onDrop={onDrop}
        onGroupTab={onGroupTab}
        onPreview={onPreview}
        onRenameTab={onRenameTab}
        onSelect={onSelect}
        onSplit={onSplit}
        setDraggingTabId={setDraggingTabId}
      />
    );
  };

  return (
    <section
      className="tab-group"
      style={{ "--group-color": group.color } as CSSProperties}
      data-collapsed={group.isCollapsed}
      data-empty={tabs.length === 0}
      data-collapsed-preview={group.isCollapsed && !hasActiveTab && tabs.length > 0}
    >
      <div
        className="tab-group-header"
        draggable
        data-collapsed={group.isCollapsed}
        data-group-id={group.id}
        data-dragging={draggingGroupId === group.id}
        data-drop-target={Boolean(draggingGroupId && draggingGroupId !== group.id)}
        onContextMenu={(event) => onGroupContextMenu(event, group)}
        onDragStart={(event) => {
          if (isRenaming) {
            event.preventDefault();
            return;
          }
          setDraggingGroupId(group.id);
          event.dataTransfer.effectAllowed = "move";
          event.dataTransfer.setData(SIDEBAR_DRAG_DATA.groupId, group.id);
        }}
        onDragEnd={() => setDraggingGroupId(null)}
        onDragOver={(event) => {
          const intent = acceptSidebarTabGroupHeaderDrag(event, tabGroupHeaderDropState, group.id);
          if (intent) {
            const ancestor = event.currentTarget.closest(".sidebar-section, .essentials") as HTMLElement | null;
            if (ancestor) clearDropTargetActive(ancestor);
            event.stopPropagation();
          }
        }}
        onDragLeave={(event) => {
          const container = event.currentTarget;
          const next = event.relatedTarget as Node | null;
          if (next && container.contains(next)) return;
          clearSidebarRowReorderDrop(event);
        }}
        onKeyDown={(event) => {
          if (event.key === "F2" && onRenameGroup && !isRenaming) {
            event.preventDefault();
            event.stopPropagation();
            setRenameDraft(group.name);
            setIsRenaming(true);
          }
        }}
        onDrop={(event) => {
          const intent = resolveSidebarTabGroupHeaderDrop(event, tabGroupHeaderDropState, group.id);
          if (!intent) return;

          event.stopPropagation();

          if (intent.type === "group") {
            onGroupDrop(event, group.id);
            return;
          }
          if (intent.type === "currentGroup") {
            setDraggingGroupId(null);
            return;
          }

          onMoveTabToGroupFolder(intent.tabId, group.id);
          setDraggingTabId(null);
        }}
      >
        <button
          className="tab-group-toggle"
          type="button"
          aria-expanded={!group.isCollapsed}
          aria-label={`${group.isCollapsed ? "Expand" : "Collapse"} tab group ${group.name}`}
          onClick={onToggle}
          onDoubleClick={onToggle}
          onKeyDown={handleToggleKeyDown}
        >
          <FiFolder className="tab-group-folder-icon" />
        </button>
        <span className="tab-group-title">
          {isRenaming ? (
            <input
              ref={renameInputRef}
              className="tab-group-title-input"
              type="text"
              value={renameDraft}
              onClick={(event) => event.stopPropagation()}
              onDoubleClick={(event) => event.stopPropagation()}
              onMouseDown={(event) => event.stopPropagation()}
              onDragStart={(event) => {
                event.preventDefault();
                event.stopPropagation();
              }}
              onChange={(event) => setRenameDraft(event.target.value)}
              onKeyDown={(event) => {
                if (event.key === "Enter") {
                  event.preventDefault();
                  event.stopPropagation();
                  commitRename();
                } else if (event.key === "Escape") {
                  event.preventDefault();
                  event.stopPropagation();
                  cancelRename();
                }
              }}
              onBlur={commitRename}
            />
          ) : (
            <span
              onDoubleClick={(event) => {
                if (!onRenameGroup) return;
                event.preventDefault();
                event.stopPropagation();
                setRenameDraft(group.name);
                setIsRenaming(true);
              }}
            >
              {group.name}
            </span>
          )}
        </span>
        <span className="tab-group-count">{tabs.length}</span>
      </div>
      {(!group.isCollapsed || hasActiveTab) && tabs.map(renderTabOrSplit)}
      {group.isCollapsed && !hasActiveTab && tabs.length > 0 && (
        <div className="tab-group-collapsed-preview" aria-hidden="true">
          {tabs.slice(0, 4).map((tab, index) => (
            <span
              className="tab-group-collapsed-favicon"
              key={tab.id}
              style={{ "--folder-index": index } as CSSProperties}
              title={tab.title || tab.url}
            >
              <SidebarItemIcon
                className="tab-favicon"
                faviconCache={faviconCache}
                faviconUrl={tab.faviconUrl}
                url={tab.url}
              />
            </span>
          ))}
        </div>
      )}
    </section>
  );
}
