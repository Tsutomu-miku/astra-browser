import { createElement } from "react";
import { act } from "react";
import { renderToStaticMarkup } from "react-dom/server";
import { createRoot } from "react-dom/client";
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
      draggingGroupId: null,
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
      setDraggingGroupId: vi.fn(),
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
      draggingGroupId: null,
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
      setDraggingGroupId: vi.fn(),
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
      draggingGroupId: null,
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
      setDraggingGroupId: vi.fn(),
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
      draggingGroupId: null,
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
      setDraggingGroupId: vi.fn(),
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
      draggingGroupId: null,
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
      setDraggingGroupId: vi.fn(),
      setDraggingTabId: vi.fn(),
      splitTabIds: []
    }));

    expect(html).toContain('draggable="true"');
    expect(html).toContain('data-dragging="true"');
    expect(html).toContain('data-drop-target="true"');
  });

  it("marks tab group headers as draggable for cross-Space moves", () => {
    const grouped = { ...createTab("Docs", "https://docs.example"), groupId: "group" };
    const html = renderToStaticMarkup(createElement(SidebarSections, {
      actions: createActions(),
      activeTab: grouped,
      closedTabs: [],
      draggingEssentialId: null,
      draggingFavoriteId: null,
      draggingGroupId: "group",
      draggingTabId: null,
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
      setDraggingGroupId: vi.fn(),
      setDraggingTabId: vi.fn(),
      splitTabIds: []
    }));

    expect(html).toContain('class="tab-group-header"');
    expect(html).toContain('draggable="true"');
    expect(html).toContain('data-dragging="true"');
  });

  it("temporarily reveals collapsed tab drop sections while dragging a tab", () => {
    const activeTab = createTab("Docs", "https://docs.example");
    const pinned = { ...createTab("Mail", "https://mail.example"), isPinned: true };
    const essential = createFavorite("Calendar", "https://calendar.example");
    const favorite = createFavorite("MDN", "https://developer.mozilla.org");
    const container = document.createElement("div");
    const root = createRoot(container);
    const baseProps = {
      actions: createActions(),
      activeTab,
      closedTabs: [],
      draggingEssentialId: null,
      draggingFavoriteId: null,
      draggingGroupId: null,
      filteredItems: {
        essentials: [essential],
        favorites: [favorite],
        groupedTabs: [],
        hasMatches: true,
        isFiltering: false,
        pinnedTabs: [pinned],
        regularTabs: [activeTab]
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
      setDraggingGroupId: vi.fn(),
      setDraggingTabId: vi.fn(),
      splitTabIds: []
    };

    act(() => {
      root.render(createElement(SidebarSections, {
        ...baseProps,
        draggingTabId: null
      }));
    });
    act(() => {
      container.querySelectorAll(".sidebar-section-header-button").forEach((button) => {
        button.dispatchEvent(new MouseEvent("click", { bubbles: true }));
      });
    });

    expect(container.querySelector(".essentials")).toBeNull();
    expect(container.querySelector(".pinned-tabs")).toBeNull();
    expect(container.querySelector(".favorites")).toBeNull();
    expect(container.querySelector(".tabs")).toBeNull();

    act(() => {
      root.render(createElement(SidebarSections, {
        ...baseProps,
        draggingTabId: activeTab.id
      }));
    });

    expect(container.querySelector(".essentials")).not.toBeNull();
    expect(container.querySelector(".pinned-tabs")).not.toBeNull();
    expect(container.querySelector(".favorites")).not.toBeNull();
    expect(container.querySelector(".tabs")).not.toBeNull();

    act(() => root.unmount());
  });

  it("temporarily reveals collapsed quick-entry sections while reordering them", () => {
    const activeTab = createTab("Docs", "https://docs.example");
    const firstEssential = createFavorite("Calendar", "https://calendar.example");
    const secondEssential = createFavorite("Mail", "https://mail.example");
    const firstFavorite = createFavorite("MDN", "https://developer.mozilla.org");
    const secondFavorite = createFavorite("Chromium", "https://www.chromium.org");
    const container = document.createElement("div");
    const root = createRoot(container);
    const baseProps = {
      actions: createActions(),
      activeTab,
      closedTabs: [],
      draggingGroupId: null,
      draggingTabId: null,
      filteredItems: {
        essentials: [firstEssential, secondEssential],
        favorites: [firstFavorite, secondFavorite],
        groupedTabs: [],
        hasMatches: true,
        isFiltering: false,
        pinnedTabs: [],
        regularTabs: [activeTab]
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
      setDraggingGroupId: vi.fn(),
      setDraggingTabId: vi.fn(),
      splitTabIds: []
    };

    act(() => {
      root.render(createElement(SidebarSections, {
        ...baseProps,
        draggingEssentialId: null,
        draggingFavoriteId: null
      }));
    });
    act(() => {
      container.querySelectorAll(".sidebar-section-header-button").forEach((button) => {
        button.dispatchEvent(new MouseEvent("click", { bubbles: true }));
      });
    });

    expect(container.querySelector(".essentials")).toBeNull();
    expect(container.querySelector(".favorites")).toBeNull();

    act(() => {
      root.render(createElement(SidebarSections, {
        ...baseProps,
        draggingEssentialId: firstEssential.id,
        draggingFavoriteId: null
      }));
    });

    expect(container.querySelector(".essentials")).not.toBeNull();
    expect(container.querySelector(".favorites")).toBeNull();

    act(() => {
      root.render(createElement(SidebarSections, {
        ...baseProps,
        draggingEssentialId: null,
        draggingFavoriteId: firstFavorite.id
      }));
    });

    expect(container.querySelector(".essentials")).toBeNull();
    expect(container.querySelector(".favorites")).not.toBeNull();

    act(() => root.unmount());
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
