import { createElement } from "react";
import { act } from "react";
import { createRoot } from "react-dom/client";
import { describe, expect, it, vi } from "vitest";

import { SIDEBAR_TAB_DRAG_TYPE } from "../src/renderer/common/drag-drop/sidebarDragPayload";
import { createFavorite, createTab } from "../src/renderer/domain/browser";
import type { BrowserTab } from "../src/renderer/domain/browser";
import type { BrowserController } from "../src/renderer/app/controller/types";
import { SidebarSections } from "../src/renderer/surfaces/sidebar/components/tabs/SidebarSections";

/**
 * Helper to create a favorite tab (new model): a BrowserTab with isFavorite=true.
 */
function createFavoriteTab(title: string, url: string): BrowserTab {
  const tab = createTab(title, url);
  tab.isFavorite = true;
  return tab;
}

describe("sidebar favorites", () => {
  it("navigates the active tab when an Essential is clicked", () => {
    const activeTab = createTab("Active", "https://active.example");
    const essential = createFavorite("Docs", "https://docs.example");
    const actions = createActions();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(SidebarSections, {
        actions,
        activeTab,
        draggingEssentialId: null,
        draggingFavoriteId: null,
        draggingGroupId: null,
        draggingTabId: null,
        filteredItems: {
          essentials: [essential],
          favorites: [],
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
        onFavoriteTabDrop: vi.fn(),
        onTabGroupContextMenu: vi.fn(),
        onQuickEntryContextMenu: vi.fn(),
        onTabContextMenu: vi.fn(),
        onTabDrop: vi.fn(),
        onTabsDrop: vi.fn(),
        onRenameTab: vi.fn(),
        onToggleSection: vi.fn(),
        setDraggingEssentialId: vi.fn(),
        setDraggingFavoriteId: vi.fn(),
        setDraggingGroupId: vi.fn(),
        setDraggingTabId: vi.fn(),
        splitTabIds: []
      }));
    });

    container.querySelector(".essentials .favorite-button")?.dispatchEvent(new MouseEvent("click", { bubbles: true }));

    expect(container.querySelector(".essentials .favorite-button")?.getAttribute("aria-label")).toBe("Docs, Essential");
    expect(actions.navigateActiveTab).toHaveBeenCalledWith(essential.url, essential.title);
    expect(actions.openUrlInActiveWorkspace).not.toHaveBeenCalled();

    act(() => root.unmount());
  });

  it("selects the exact favorite tab when a Space favorite is clicked", () => {
    const activeTab = createTab("Active", "https://active.example");
    const docsTab = createFavoriteTab("Docs", "https://docs.example");
    const actions = createActions();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(SidebarSections, {
        actions,
        activeTab,
        draggingEssentialId: null,
        draggingFavoriteId: null,
        draggingGroupId: null,
        draggingTabId: null,
        filteredItems: {
          essentials: [],
          favorites: [{ kind: "tab", tab: docsTab }],
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
        onFavoriteTabDrop: vi.fn(),
        onTabGroupContextMenu: vi.fn(),
        onQuickEntryContextMenu: vi.fn(),
        onTabContextMenu: vi.fn(),
        onTabDrop: vi.fn(),
        onTabsDrop: vi.fn(),
        onRenameTab: vi.fn(),
        onToggleSection: vi.fn(),
        setDraggingEssentialId: vi.fn(),
        setDraggingFavoriteId: vi.fn(),
        setDraggingGroupId: vi.fn(),
        setDraggingTabId: vi.fn(),
        splitTabIds: [],
        workspaceTabs: [activeTab, docsTab]
      }));
    });

    container.querySelector(".favorites .tab-button")?.dispatchEvent(new MouseEvent("click", { bubbles: true }));

    expect(container.querySelector(".favorites .tab-button")?.getAttribute("aria-label")).toBe("Docs, favorite tab");
    expect(actions.selectTab).toHaveBeenCalledWith(docsTab.id);
    expect(actions.navigateActiveTab).not.toHaveBeenCalled();
    expect(actions.openUrlInActiveWorkspace).not.toHaveBeenCalled();
    // Favorites render as TabRow, never as FavoriteButton.
    expect(container.querySelector(".favorites .favorite-button")).toBeNull();

    act(() => root.unmount());
  });

  it("marks favorite tabs active only when their tab id matches the active tab", () => {
    const activeTab = { ...createTab("Docs clone", "https://docs.example"), id: "active-clone" };
    const docsTab = { ...createFavoriteTab("Docs", "https://docs.example"), id: "docs-fav" };
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(SidebarSections, {
        actions: createActions(),
        activeTab,
        draggingEssentialId: null,
        draggingFavoriteId: null,
        draggingGroupId: null,
        draggingTabId: null,
        filteredItems: {
          essentials: [],
          favorites: [{ kind: "tab", tab: docsTab }],
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
        onFavoriteTabDrop: vi.fn(),
        onTabGroupContextMenu: vi.fn(),
        onQuickEntryContextMenu: vi.fn(),
        onTabContextMenu: vi.fn(),
        onTabDrop: vi.fn(),
        onTabsDrop: vi.fn(),
        onRenameTab: vi.fn(),
        onToggleSection: vi.fn(),
        setDraggingEssentialId: vi.fn(),
        setDraggingFavoriteId: vi.fn(),
        setDraggingGroupId: vi.fn(),
        setDraggingTabId: vi.fn(),
        splitTabIds: [],
        workspaceTabs: [activeTab, docsTab]
      }));
    });

    // docsTab is NOT the active tab (activeTab has a different id, same URL)
    // → aria-current must be false. URL match is NOT enough.
    expect(container.querySelector(".favorites .tab-row")?.getAttribute("aria-current")).toBe("false");
    expect(container.querySelector(".favorites .tab-button")?.getAttribute("aria-label")).toBe("Docs, favorite tab");

    act(() => root.unmount());
  });

  it("uses tab actions for favorite tabs", () => {
    const activeTab = createTab("Active", "https://active.example");
    const docsTab = createFavoriteTab("Docs", "https://docs.example");
    const actions = createActions();
    const onQuickEntryContextMenu = vi.fn();
    const onTabContextMenu = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(SidebarSections, {
        actions,
        activeTab,
        draggingEssentialId: null,
        draggingFavoriteId: null,
        draggingGroupId: null,
        draggingTabId: null,
        filteredItems: {
          essentials: [],
          favorites: [{ kind: "tab", tab: docsTab }],
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
        onFavoriteTabDrop: vi.fn(),
        onTabGroupContextMenu: vi.fn(),
        onQuickEntryContextMenu,
        onTabContextMenu,
        onTabDrop: vi.fn(),
        onTabsDrop: vi.fn(),
        onRenameTab: vi.fn(),
        onToggleSection: vi.fn(),
        setDraggingEssentialId: vi.fn(),
        setDraggingFavoriteId: vi.fn(),
        setDraggingGroupId: vi.fn(),
        setDraggingTabId: vi.fn(),
        splitTabIds: [],
        workspaceTabs: [activeTab, docsTab]
      }));
    });

    const favoriteRow = container.querySelector(".favorites .tab-row")!;
    const favoriteButton = container.querySelector(".favorites .tab-button")!;
    favoriteRow.dispatchEvent(new MouseEvent("contextmenu", { bubbles: true }));
    favoriteButton.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "Delete" }));
    favoriteButton.dispatchEvent(new MouseEvent("auxclick", { bubbles: true, button: 1 }));

    expect(onTabContextMenu).toHaveBeenCalledWith(expect.objectContaining({ type: "contextmenu" }), docsTab);
    expect(onQuickEntryContextMenu).not.toHaveBeenCalled();
    expect(actions.closeTab).toHaveBeenCalledWith(docsTab.id);
    expect(actions.closeTab).toHaveBeenCalledTimes(2);

    act(() => root.unmount());
  });

  it("opens favorite tabs in split by tab id", () => {
    const activeTab = createTab("Active", "https://active.example");
    const docsTab = createFavoriteTab("Docs", "https://docs.example");
    const actions = createActions();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(SidebarSections, {
        actions,
        activeTab,
        draggingEssentialId: null,
        draggingFavoriteId: null,
        draggingGroupId: null,
        draggingTabId: null,
        filteredItems: {
          essentials: [],
          favorites: [{ kind: "tab", tab: docsTab }],
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
        onFavoriteTabDrop: vi.fn(),
        onTabGroupContextMenu: vi.fn(),
        onQuickEntryContextMenu: vi.fn(),
        onTabContextMenu: vi.fn(),
        onTabDrop: vi.fn(),
        onTabsDrop: vi.fn(),
        onRenameTab: vi.fn(),
        onToggleSection: vi.fn(),
        setDraggingEssentialId: vi.fn(),
        setDraggingFavoriteId: vi.fn(),
        setDraggingGroupId: vi.fn(),
        setDraggingTabId: vi.fn(),
        splitTabIds: [],
        workspaceTabs: [activeTab, docsTab]
      }));
    });

    const favoriteButton = container.querySelector(".favorites .tab-button")!;
    favoriteButton.dispatchEvent(new MouseEvent("click", { bubbles: true, shiftKey: true }));
    favoriteButton.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "Enter", shiftKey: true }));

    expect(actions.openTabInSplit).toHaveBeenCalledWith(docsTab.id);
    expect(actions.openTabInSplit).toHaveBeenCalledTimes(2);
    expect(actions.openUrlInSplit).not.toHaveBeenCalled();

    act(() => root.unmount());
  });

  it("renders tab status on favorite tabs", () => {
    const activeTab = createTab("Active", "https://active.example");
    const docsTab = { ...createFavoriteTab("Docs", "https://docs.example"), isMuted: true };
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(SidebarSections, {
        actions: createActions(),
        activeTab,
        draggingEssentialId: null,
        draggingFavoriteId: null,
        draggingGroupId: null,
        draggingTabId: null,
        filteredItems: {
          essentials: [],
          favorites: [{ kind: "tab", tab: docsTab }],
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
        onFavoriteTabDrop: vi.fn(),
        onTabGroupContextMenu: vi.fn(),
        onQuickEntryContextMenu: vi.fn(),
        onTabContextMenu: vi.fn(),
        onTabDrop: vi.fn(),
        onTabsDrop: vi.fn(),
        onRenameTab: vi.fn(),
        onToggleSection: vi.fn(),
        setDraggingEssentialId: vi.fn(),
        setDraggingFavoriteId: vi.fn(),
        setDraggingGroupId: vi.fn(),
        setDraggingTabId: vi.fn(),
        splitTabIds: [docsTab.id],
        workspaceTabs: [activeTab, docsTab]
      }));
    });

    const favoriteButton = container.querySelector(".favorites .tab-button");
    expect(favoriteButton?.getAttribute("aria-label")).toBe("Docs, favorite tab, Split, Muted");
    expect(favoriteButton?.querySelector(".tab-title-stack")).not.toBeNull();
    expect(favoriteButton?.querySelector(".tab-status-badge.is-split")).not.toBeNull();
    expect(favoriteButton?.querySelector(".tab-status-badge.is-muted")).not.toBeNull();
    expect(favoriteButton?.querySelector(".tab-status-badge.is-split")?.hasAttribute("title")).toBe(false);
    expect(favoriteButton?.querySelector(".tab-status-badge.is-muted")?.hasAttribute("title")).toBe(false);
    expect(favoriteButton?.querySelector(".tab-status-badge.is-split")?.textContent).toBe("");
    expect(favoriteButton?.querySelector(".tab-status-badge.is-muted")?.textContent).toBe("");

    act(() => root.unmount());
  });

  it("drags favorite tabs as plain tab rows (tab drag, not favorite drag)", () => {
    const activeTab = createTab("Active", "https://active.example");
    const docsTab = createFavoriteTab("Docs", "https://docs.example");
    const setDraggingTabId = vi.fn();
    const onFavoriteDragStart = vi.fn();
    const data = new Map<string, string>();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(SidebarSections, {
        actions: createActions(),
        activeTab,
        draggingEssentialId: null,
        draggingFavoriteId: null,
        draggingGroupId: null,
        draggingTabId: null,
        filteredItems: {
          essentials: [],
          favorites: [{ kind: "tab", tab: docsTab }],
          groupedTabs: [],
          hasMatches: true,
          isFiltering: false,
          pinnedTabs: [],
          regularTabs: [activeTab]
        },
        onEssentialDragStart: vi.fn(),
        onEssentialDrop: vi.fn(),
        onEssentialReorderDrop: vi.fn(),
        onFavoriteDragStart,
        onFavoriteDrop: vi.fn(),
        onFavoriteReorderDrop: vi.fn(),
        onFavoriteTabDrop: vi.fn(),
        onTabGroupContextMenu: vi.fn(),
        onQuickEntryContextMenu: vi.fn(),
        onTabContextMenu: vi.fn(),
        onTabDrop: vi.fn(),
        onTabsDrop: vi.fn(),
        onRenameTab: vi.fn(),
        onToggleSection: vi.fn(),
        setDraggingEssentialId: vi.fn(),
        setDraggingFavoriteId: vi.fn(),
        setDraggingGroupId: vi.fn(),
        setDraggingTabId,
        splitTabIds: [],
        workspaceTabs: [activeTab, docsTab]
      }));
    });

    const event = new Event("dragstart", { bubbles: true });
    Object.defineProperty(event, "dataTransfer", {
      value: {
        effectAllowed: "",
        getData: (type: string) => data.get(type) ?? "",
        setData: (type: string, value: string) => data.set(type, value)
      }
    });
    container.querySelector(".favorites .tab-row")?.dispatchEvent(event);

    // Favorite tabs use tab drag identity, not legacy favorite drag.
    expect(setDraggingTabId).toHaveBeenCalledWith(docsTab.id);
    expect(onFavoriteDragStart).not.toHaveBeenCalled();
    expect(data.get(SIDEBAR_TAB_DRAG_TYPE)).toBe(docsTab.id);
    expect(data.get("text/favorite-id")).toBeUndefined();

    act(() => root.unmount());
  });

});

function createActions() {
  return {
    assignTabToGroup: vi.fn(),
    closeTab: vi.fn(),
    groupTab: vi.fn(),
    navigateActiveTab: vi.fn(),
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
