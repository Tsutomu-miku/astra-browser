import { createElement } from "react";
import { act } from "react";
import { createRoot } from "react-dom/client";
import { describe, expect, it, vi } from "vitest";

import { createTab } from "../src/renderer/domain/browser";
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

describe("sidebar favorites same-url tab resolution", () => {
  it("selects the exact favorite tab when multiple tabs share the same URL", () => {
    const tabA = { ...createFavoriteTab("Tab A", "https://same.example"), id: "tab-a" };
    const tabB = { ...createTab("Tab B", "https://same.example"), id: "tab-b" };
    const tabC = { ...createTab("Tab C", "https://other.example"), id: "tab-c" };

    // tabA is the favorite. tabB has the same URL but is a completely separate
    // tab — it must NOT be selected when clicking the favorite.

    const actions = createActions();
    const container = document.createElement("div");
    const root = createRoot(container);

    // tabC is the initially active tab.
    const activeTab = tabC;

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
          favorites: [{ kind: "tab", tab: tabA }],
          groupedTabs: [],
          hasMatches: true,
          isFiltering: false,
          pinnedTabs: [],
          regularTabs: [tabB]
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
        workspaceTabs: [tabA, tabB, tabC]
      }));
    });

    const favoriteTabButton = container.querySelector(".favorites .tab-button");
    expect(favoriteTabButton).not.toBeNull();
    expect(favoriteTabButton?.getAttribute("aria-label")).toBe("Tab A, favorite tab");

    favoriteTabButton?.dispatchEvent(new MouseEvent("click", { bubbles: true }));

    // The click must resolve to tabA (the exact favorite tab),
    // never fall back to tabB even though their URLs are identical.
    expect(actions.selectTab).toHaveBeenCalledWith("tab-a");
    expect(actions.selectTab).not.toHaveBeenCalledWith("tab-b");
    expect(actions.selectTab).toHaveBeenCalledTimes(1);

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
