import { createElement } from "react";
import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, it, vi } from "vitest";

import { createFavorite, createTab } from "../src/renderer/domain/browser";
import type { BrowserController } from "../src/renderer/app/controller/types";
import { SidebarSections } from "../src/renderer/surfaces/sidebar/components/tabs/SidebarSections";

describe("sidebar section drop zones", () => {
  it("shows Pinned, Essentials, and Favorites drop targets while dragging a regular tab", () => {
    const tab = createTab("Docs", "https://docs.example");
    const html = renderToStaticMarkup(createElement(SidebarSections, {
      actions: createActions(),
      activeTab: tab,
      closedTabs: [],
      draggingEssentialId: null,
      draggingFavoriteId: null,
      draggingTabId: tab.id,
      filteredItems: {
        essentials: [],
        favorites: [],
        groupedTabs: [],
        hasMatches: true,
        isFiltering: false,
        pinnedTabs: [],
        regularTabs: [tab]
      },
      onEssentialDragStart: vi.fn(),
      onEssentialDrop: vi.fn(),
      onEssentialReorderDrop: vi.fn(),
      onFavoriteDragStart: vi.fn(),
      onFavoriteDrop: vi.fn(),
      onFavoriteReorderDrop: vi.fn(),
      onClosedTabContextMenu: vi.fn(),
      onTabGroupContextMenu: vi.fn(),
      onPinDrop: vi.fn(),
      onQuickEntryContextMenu: vi.fn(),
      onTabContextMenu: vi.fn(),
      onTabDrop: vi.fn(),
      setDraggingEssentialId: vi.fn(),
      setDraggingFavoriteId: vi.fn(),
      setDraggingTabId: vi.fn(),
      splitTabIds: []
    }));

    expect(html).toContain("Drop to pin");
    expect(html).toContain("Drop to essential");
    expect(html).toContain("Drop to favorite");
    expect(html).toContain("New group");
    expect(html).not.toContain("Ungroup tab");
    expect(html).toContain('aria-label="Pinned tabs"');
    expect(html).toContain('aria-label="Essentials"');
    expect(html).toContain('aria-label="Favorites"');
    expect(html).toContain('aria-label="Create new group from dragged tab"');
  });

  it("does not offer a new group target for dragged pinned tabs", () => {
    const pinned = { ...createTab("Mail", "https://mail.example"), isPinned: true };
    const html = renderToStaticMarkup(createElement(SidebarSections, {
      actions: createActions(),
      activeTab: pinned,
      closedTabs: [],
      draggingEssentialId: null,
      draggingFavoriteId: null,
      draggingTabId: pinned.id,
      filteredItems: {
        essentials: [],
        favorites: [],
        groupedTabs: [],
        hasMatches: true,
        isFiltering: false,
        pinnedTabs: [pinned],
        regularTabs: []
      },
      onEssentialDragStart: vi.fn(),
      onEssentialDrop: vi.fn(),
      onEssentialReorderDrop: vi.fn(),
      onFavoriteDragStart: vi.fn(),
      onFavoriteDrop: vi.fn(),
      onFavoriteReorderDrop: vi.fn(),
      onClosedTabContextMenu: vi.fn(),
      onTabGroupContextMenu: vi.fn(),
      onPinDrop: vi.fn(),
      onQuickEntryContextMenu: vi.fn(),
      onTabContextMenu: vi.fn(),
      onTabDrop: vi.fn(),
      setDraggingEssentialId: vi.fn(),
      setDraggingFavoriteId: vi.fn(),
      setDraggingTabId: vi.fn(),
      splitTabIds: []
    }));

    expect(html).not.toContain("New group");
  });

  it("offers an Ungroup target for dragged grouped tabs", () => {
    const grouped = { ...createTab("Docs", "https://docs.example"), groupId: "group" };
    const html = renderToStaticMarkup(createElement(SidebarSections, {
      actions: createActions(),
      activeTab: grouped,
      closedTabs: [],
      draggingEssentialId: null,
      draggingFavoriteId: null,
      draggingTabId: grouped.id,
      filteredItems: {
        essentials: [],
        favorites: [],
        groupedTabs: [{
          group: { id: "group", name: "Research", color: "#7dd3fc", isCollapsed: false },
          tabs: [grouped]
        }],
        hasMatches: true,
        isFiltering: false,
        pinnedTabs: [],
        regularTabs: []
      },
      onEssentialDragStart: vi.fn(),
      onEssentialDrop: vi.fn(),
      onEssentialReorderDrop: vi.fn(),
      onFavoriteDragStart: vi.fn(),
      onFavoriteDrop: vi.fn(),
      onFavoriteReorderDrop: vi.fn(),
      onClosedTabContextMenu: vi.fn(),
      onTabGroupContextMenu: vi.fn(),
      onPinDrop: vi.fn(),
      onQuickEntryContextMenu: vi.fn(),
      onTabContextMenu: vi.fn(),
      onTabDrop: vi.fn(),
      setDraggingEssentialId: vi.fn(),
      setDraggingFavoriteId: vi.fn(),
      setDraggingTabId: vi.fn(),
      splitTabIds: []
    }));

    expect(html).toContain("Ungroup tab");
    expect(html).toContain('aria-label="Remove dragged tab from group"');
    expect(html).not.toContain("New group");
  });

  it("marks Essentials as reorderable drop targets while dragging an Essential", () => {
    const tab = createTab("Docs", "https://docs.example");
    const first = createFavorite("Mail", "https://mail.example");
    const second = createFavorite("Calendar", "https://calendar.example");
    const html = renderToStaticMarkup(createElement(SidebarSections, {
      actions: createActions(),
      activeTab: tab,
      closedTabs: [],
      draggingEssentialId: first.id,
      draggingFavoriteId: null,
      draggingTabId: null,
      filteredItems: {
        essentials: [first, second],
        favorites: [],
        groupedTabs: [],
        hasMatches: true,
        isFiltering: false,
        pinnedTabs: [],
        regularTabs: [tab]
      },
      onEssentialDragStart: vi.fn(),
      onEssentialDrop: vi.fn(),
      onEssentialReorderDrop: vi.fn(),
      onFavoriteDragStart: vi.fn(),
      onFavoriteDrop: vi.fn(),
      onFavoriteReorderDrop: vi.fn(),
      onClosedTabContextMenu: vi.fn(),
      onTabGroupContextMenu: vi.fn(),
      onPinDrop: vi.fn(),
      onQuickEntryContextMenu: vi.fn(),
      onTabContextMenu: vi.fn(),
      onTabDrop: vi.fn(),
      setDraggingEssentialId: vi.fn(),
      setDraggingFavoriteId: vi.fn(),
      setDraggingTabId: vi.fn(),
      splitTabIds: []
    }));

    expect(html).toContain('draggable="true"');
    expect(html).toContain('data-dragging="true"');
    expect(html).toContain('data-drop-target="true"');
  });

  it("marks Space favorites as reorderable drop targets while dragging a favorite", () => {
    const tab = createTab("Docs", "https://docs.example");
    const first = createFavorite("First", "https://first.example");
    const second = createFavorite("Second", "https://second.example");
    const html = renderToStaticMarkup(createElement(SidebarSections, {
      actions: createActions(),
      activeTab: tab,
      closedTabs: [],
      draggingEssentialId: null,
      draggingFavoriteId: first.id,
      draggingTabId: null,
      filteredItems: {
        essentials: [],
        favorites: [first, second],
        groupedTabs: [],
        hasMatches: true,
        isFiltering: false,
        pinnedTabs: [],
        regularTabs: [tab]
      },
      onEssentialDragStart: vi.fn(),
      onEssentialDrop: vi.fn(),
      onEssentialReorderDrop: vi.fn(),
      onFavoriteDragStart: vi.fn(),
      onFavoriteDrop: vi.fn(),
      onFavoriteReorderDrop: vi.fn(),
      onClosedTabContextMenu: vi.fn(),
      onTabGroupContextMenu: vi.fn(),
      onPinDrop: vi.fn(),
      onQuickEntryContextMenu: vi.fn(),
      onTabContextMenu: vi.fn(),
      onTabDrop: vi.fn(),
      setDraggingEssentialId: vi.fn(),
      setDraggingFavoriteId: vi.fn(),
      setDraggingTabId: vi.fn(),
      splitTabIds: []
    }));

    expect(html).toContain('draggable="true"');
    expect(html).toContain('data-dragging="true"');
    expect(html).toContain('data-drop-target="true"');
  });
});

function createActions() {
  return {
    assignTabToGroup: vi.fn(),
    closeTab: vi.fn(),
    groupTab: vi.fn(),
    openGlance: vi.fn(),
    openTabInSplit: vi.fn(),
    openUrlInActiveWorkspace: vi.fn(),
    openUrlInSplit: vi.fn(),
    restoreClosedTab: vi.fn(),
    selectTab: vi.fn(),
    toggleTabGroupCollapsed: vi.fn(),
    updateTabGroup: vi.fn()
  } as unknown as BrowserController["actions"];
}
