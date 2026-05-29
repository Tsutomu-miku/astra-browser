import { createElement } from "react";
import { act } from "react";
import { createRoot } from "react-dom/client";
import { describe, expect, it, vi } from "vitest";

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

    container.querySelector(".favorite-button")?.dispatchEvent(new MouseEvent("click", { bubbles: true }));

    expect(container.querySelector(".favorite-button")?.getAttribute("aria-label")).toBe("Docs, Favorite");
    expect(actions.selectTab).toHaveBeenCalledWith(docsTab.id);
    expect(actions.navigateActiveTab).not.toHaveBeenCalled();
    expect(actions.openUrlInActiveWorkspace).not.toHaveBeenCalled();

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
