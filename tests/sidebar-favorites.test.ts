import { createElement, type DragEvent as ReactDragEvent } from "react";
import { act } from "react";
import { createRoot } from "react-dom/client";
import { describe, expect, it, vi } from "vitest";

import { SIDEBAR_TAB_DRAG_TYPE } from "../src/renderer/common/drag-drop/sidebarDragPayload";
import { createFavorite, createTab } from "../src/renderer/domain/browser";
import type { BrowserController } from "../src/renderer/app/controller/types";
import { SidebarSections } from "../src/renderer/surfaces/sidebar/components/tabs/SidebarSections";

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
        closedTabs: [],
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
    });

    container.querySelector(".essentials .favorite-button")?.dispatchEvent(new MouseEvent("click", { bubbles: true }));

    expect(container.querySelector(".essentials .favorite-button")?.getAttribute("aria-label")).toBe("Docs, Essential");
    expect(actions.navigateActiveTab).toHaveBeenCalledWith(essential.url, essential.title);
    expect(actions.openUrlInActiveWorkspace).not.toHaveBeenCalled();

    act(() => root.unmount());
  });

  it("selects the matching tab when a Space favorite is clicked", () => {
    const activeTab = createTab("Active", "https://active.example");
    const docsTab = createTab("Docs", "https://docs.example");
    const favorite = createFavorite("Docs", docsTab.url, docsTab.id);
    const actions = createActions();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(SidebarSections, {
        actions,
        activeTab,
        closedTabs: [],
        draggingEssentialId: null,
        draggingFavoriteId: null,
        draggingGroupId: null,
        draggingTabId: null,
        filteredItems: {
          essentials: [],
          favorites: [favorite],
          groupedTabs: [],
          hasMatches: true,
          isFiltering: false,
          pinnedTabs: [],
          regularTabs: [activeTab, docsTab]
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
        splitTabIds: [],
        workspaceTabs: [activeTab, docsTab]
      }));
    });

    container.querySelector(".favorites .tab-button")?.dispatchEvent(new MouseEvent("click", { bubbles: true }));

    expect(container.querySelector(".favorites .tab-button")?.getAttribute("aria-label")).toBe("Docs, favorite tab");
    expect(actions.selectTab).toHaveBeenCalledWith(docsTab.id);
    expect(actions.navigateActiveTab).not.toHaveBeenCalled();
    expect(actions.openUrlInActiveWorkspace).not.toHaveBeenCalled();
    expect(container.querySelector(".favorites .favorite-button")).toBeNull();

    act(() => root.unmount());
  });

  it("marks tab-backed Favorites active only when their tab id matches", () => {
    const activeTab = createTab("Docs clone", "https://docs.example");
    const docsTab = createTab("Docs", "https://docs.example");
    const favorite = createFavorite("Docs", docsTab.url, docsTab.id);
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(SidebarSections, {
        actions: createActions(),
        activeTab,
        closedTabs: [],
        draggingEssentialId: null,
        draggingFavoriteId: null,
        draggingGroupId: null,
        draggingTabId: null,
        filteredItems: {
          essentials: [],
          favorites: [favorite],
          groupedTabs: [],
          hasMatches: true,
          isFiltering: false,
          pinnedTabs: [],
          regularTabs: [activeTab, docsTab]
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
        splitTabIds: [],
        workspaceTabs: [activeTab, docsTab]
      }));
    });

    expect(container.querySelector(".favorites .tab-row")?.getAttribute("aria-current")).toBe("false");
    expect(container.querySelector(".favorites .tab-button")?.getAttribute("aria-label")).toBe("Docs, favorite tab");

    act(() => root.unmount());
  });

  it("uses tab actions for tab-backed Favorites", () => {
    const activeTab = createTab("Active", "https://active.example");
    const docsTab = createTab("Docs", "https://docs.example");
    const favorite = createFavorite("Docs", docsTab.url, docsTab.id);
    const actions = createActions();
    const onQuickEntryContextMenu = vi.fn();
    const onTabContextMenu = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(SidebarSections, {
        actions,
        activeTab,
        closedTabs: [],
        draggingEssentialId: null,
        draggingFavoriteId: null,
        draggingGroupId: null,
        draggingTabId: null,
        filteredItems: {
          essentials: [],
          favorites: [favorite],
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
        onQuickEntryContextMenu,
        onTabContextMenu,
        onTabDrop: vi.fn(),
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

  it("opens tab-backed Favorites in split by tab id", () => {
    const activeTab = createTab("Active", "https://active.example");
    const docsTab = createTab("Docs", "https://docs.example");
    const favorite = createFavorite("Docs", docsTab.url, docsTab.id);
    const actions = createActions();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(SidebarSections, {
        actions,
        activeTab,
        closedTabs: [],
        draggingEssentialId: null,
        draggingFavoriteId: null,
        draggingGroupId: null,
        draggingTabId: null,
        filteredItems: {
          essentials: [],
          favorites: [favorite],
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

  it("renders tab status on tab-backed Favorites", () => {
    const activeTab = createTab("Active", "https://active.example");
    const docsTab = { ...createTab("Docs", "https://docs.example"), isMuted: true };
    const favorite = createFavorite("Docs", docsTab.url, docsTab.id);
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(SidebarSections, {
        actions: createActions(),
        activeTab,
        closedTabs: [],
        draggingEssentialId: null,
        draggingFavoriteId: null,
        draggingGroupId: null,
        draggingTabId: null,
        filteredItems: {
          essentials: [],
          favorites: [favorite],
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

    act(() => root.unmount());
  });

  it("drags tab-backed Favorites as tab rows while preserving the Favorite payload", () => {
    const activeTab = createTab("Active", "https://active.example");
    const docsTab = createTab("Docs", "https://docs.example");
    const favorite = createFavorite("Docs", docsTab.url, docsTab.id);
    const setDraggingTabId = vi.fn();
    const onFavoriteDragStart = vi.fn((event: ReactDragEvent<HTMLElement>, favoriteId: string) => {
      event.dataTransfer.setData("text/favorite-id", favoriteId);
    });
    const data = new Map<string, string>();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(SidebarSections, {
        actions: createActions(),
        activeTab,
        closedTabs: [],
        draggingEssentialId: null,
        draggingFavoriteId: null,
        draggingGroupId: null,
        draggingTabId: null,
        filteredItems: {
          essentials: [],
          favorites: [favorite],
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
        onClosedTabContextMenu: vi.fn(),
        onTabGroupContextMenu: vi.fn(),
        onPinDrop: vi.fn(),
        onQuickEntryContextMenu: vi.fn(),
        onTabContextMenu: vi.fn(),
        onTabDrop: vi.fn(),
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

    expect(setDraggingTabId).toHaveBeenCalledWith(docsTab.id);
    expect(onFavoriteDragStart).toHaveBeenCalledWith(expect.objectContaining({ type: "dragstart" }), favorite.id);
    expect(data.get(SIDEBAR_TAB_DRAG_TYPE)).toBe(docsTab.id);
    expect(data.get("text/favorite-id")).toBe(favorite.id);

    act(() => root.unmount());
  });

  it("reorders legacy Favorites against tab-backed Favorite rows", () => {
    const activeTab = createTab("Active", "https://active.example");
    const docsTab = createTab("Docs", "https://docs.example");
    const legacyFavorite = createFavorite("Legacy", "https://legacy.example");
    const tabBackedFavorite = createFavorite("Docs", docsTab.url, docsTab.id);
    const onFavoriteReorderDrop = vi.fn();
    const onTabDrop = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(SidebarSections, {
        actions: createActions(),
        activeTab,
        closedTabs: [],
        draggingEssentialId: null,
        draggingFavoriteId: null,
        draggingGroupId: null,
        draggingTabId: null,
        filteredItems: {
          essentials: [],
          favorites: [legacyFavorite, tabBackedFavorite],
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
        onFavoriteReorderDrop,
        onClosedTabContextMenu: vi.fn(),
        onTabGroupContextMenu: vi.fn(),
        onPinDrop: vi.fn(),
        onQuickEntryContextMenu: vi.fn(),
        onTabContextMenu: vi.fn(),
        onTabDrop,
        setDraggingEssentialId: vi.fn(),
        setDraggingFavoriteId: vi.fn(),
        setDraggingGroupId: vi.fn(),
        setDraggingTabId: vi.fn(),
        splitTabIds: [],
        workspaceTabs: [activeTab, docsTab]
      }));
    });

    const tabBackedRow = container.querySelector<HTMLElement>(".favorites .tab-row")!;
    tabBackedRow.dispatchEvent(createDragEvent("drop", {
      "text/favorite-id": legacyFavorite.id
    }));

    expect(onFavoriteReorderDrop).toHaveBeenCalledWith(expect.objectContaining({ type: "drop" }), tabBackedFavorite.id, "vertical");
    expect(onTabDrop).not.toHaveBeenCalled();

    act(() => root.unmount());
  });

  it("keeps legacy URL Favorites on the quick entry path", () => {
    const activeTab = createTab("Active", "https://active.example");
    const favorite = createFavorite("Docs", "https://docs.example");
    const actions = createActions();
    const onQuickEntryContextMenu = vi.fn();
    const onTabContextMenu = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(SidebarSections, {
        actions,
        activeTab,
        closedTabs: [],
        draggingEssentialId: null,
        draggingFavoriteId: null,
        draggingGroupId: null,
        draggingTabId: null,
        filteredItems: {
          essentials: [],
          favorites: [favorite],
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
        onQuickEntryContextMenu,
        onTabContextMenu,
        onTabDrop: vi.fn(),
        setDraggingEssentialId: vi.fn(),
        setDraggingFavoriteId: vi.fn(),
        setDraggingGroupId: vi.fn(),
        setDraggingTabId: vi.fn(),
        splitTabIds: [],
        workspaceTabs: [activeTab]
      }));
    });

    const favoriteButton = container.querySelector(".favorites .favorite-button")!;
    favoriteButton.dispatchEvent(new MouseEvent("contextmenu", { bubbles: true }));
    favoriteButton.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "Delete" }));
    favoriteButton.dispatchEvent(new MouseEvent("auxclick", { bubbles: true, button: 1 }));
    favoriteButton.dispatchEvent(new MouseEvent("click", { bubbles: true, shiftKey: true }));

    expect(onQuickEntryContextMenu).toHaveBeenCalledWith(expect.objectContaining({ type: "contextmenu" }), favorite, "favorite");
    expect(onTabContextMenu).not.toHaveBeenCalled();
    expect(actions.closeTab).not.toHaveBeenCalled();
    expect(actions.openUrlInSplit).toHaveBeenCalledWith(favorite.url, favorite.title);
    expect(actions.openTabInSplit).not.toHaveBeenCalled();

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

function createDragEvent(type: string, data: Record<string, string>) {
  const event = new Event(type, { bubbles: true, cancelable: true }) as Event & {
    dataTransfer: DataTransfer;
  };
  Object.defineProperty(event, "dataTransfer", {
    value: {
      dropEffect: "none",
      getData: (key: string) => data[key] ?? "",
      setData: vi.fn()
    }
  });
  return event;
}
