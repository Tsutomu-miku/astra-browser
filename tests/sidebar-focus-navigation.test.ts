import { createElement } from "react";
import { act } from "react";
import { createRoot } from "react-dom/client";
import { describe, expect, it, vi } from "vitest";

import { createFavorite, createTab } from "../src/renderer/domain/browser";
import type { BrowserTab } from "../src/renderer/domain/browser";
import type { BrowserController } from "../src/renderer/app/controller/types";
import { SidebarSections } from "../src/renderer/surfaces/sidebar/components/tabs/SidebarSections";
import {
  focusCurrentOrFirstSidebarItem,
  handleSidebarFocusNavigation,
  scrollCurrentSidebarItemIntoView
} from "../src/renderer/surfaces/sidebar/model/sidebarFocusNavigation";

/**
 * Helper to create a favorite tab (new model): a BrowserTab with isFavorite=true.
 */
function createFavoriteTab(title: string, url: string): BrowserTab {
  const tab = createTab(title, url);
  tab.isFavorite = true;
  return tab;
}

describe("sidebar focus navigation", () => {
  it("collapses and expands sidebar sections with Left and Right arrows", () => {
    const activeTab = createTab("Docs", "https://docs.example");
    const favoriteTab = createFavoriteTab("MDN", "https://developer.mozilla.org");
    const container = document.createElement("div");
    document.body.append(container);
    const root = createRoot(container);

    act(() => {
      root.render(createElement("div", {
        onKeyDown: handleSidebarFocusNavigation
      }, createElement(SidebarSections, {
        actions: createActions(),
        activeTab,
        draggingEssentialId: null,
        draggingFavoriteId: null,
        draggingGroupId: null,
        draggingTabId: null,
        filteredItems: {
          essentials: [],
          favorites: [{ kind: "tab", tab: favoriteTab }],
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
        onQuickEntryContextMenu: vi.fn(),
        onTabContextMenu: vi.fn(),
        onTabDrop: vi.fn(),
        onTabsDrop: vi.fn(),
        onRenameTab: vi.fn(),
        onTabGroupContextMenu: vi.fn(),
        setDraggingEssentialId: vi.fn(),
        setDraggingFavoriteId: vi.fn(),
        setDraggingGroupId: vi.fn(),
        setDraggingTabId: vi.fn(),
        splitTabs: [],
        activeSplitId: null,
        workspaceTabs: [activeTab, favoriteTab]
      })));
    });

    const favoritesHeader = Array.from(container.querySelectorAll<HTMLButtonElement>(".sidebar-section-header-button"))
      .find((header) => header.textContent?.includes("Favorites"))!;

    expect(favoritesHeader.getAttribute("aria-expanded")).toBe("true");
    expect(favoritesHeader.getAttribute("aria-label")).toBe("Collapse Favorites");
    expect(container.querySelector(".favorites")).not.toBeNull();

    act(() => {
      favoritesHeader.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "ArrowLeft" }));
    });

    expect(favoritesHeader.getAttribute("aria-expanded")).toBe("false");
    expect(favoritesHeader.getAttribute("aria-label")).toBe("Expand Favorites");
    expect(container.querySelector(".favorites")).toBeNull();

    act(() => {
      favoritesHeader.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "ArrowRight" }));
    });

    expect(favoritesHeader.getAttribute("aria-expanded")).toBe("true");
    expect(container.querySelector(".favorites")).not.toBeNull();

    act(() => root.unmount());
    container.remove();
  });

  it("moves focus through visible sidebar items with Arrow, Home, and End", () => {
    const activeTab = createTab("Docs", "https://docs.example");
    const pinnedTab = { ...createTab("Mail", "https://mail.example"), isPinned: true };
    const favoriteTab = createFavoriteTab("MDN", "https://developer.mozilla.org");
    const container = document.createElement("div");
    document.body.append(container);
    const root = createRoot(container);

    act(() => {
      root.render(createElement("div", {
        onKeyDown: handleSidebarFocusNavigation
      }, createElement(SidebarSections, {
        actions: createActions(),
        activeTab,
        draggingEssentialId: null,
        draggingFavoriteId: null,
        draggingGroupId: null,
        draggingTabId: null,
        filteredItems: {
          essentials: [],
          favorites: [{ kind: "tab", tab: favoriteTab }],
          groupedTabs: [],
          hasMatches: true,
          isFiltering: false,
          pinnedTabs: [pinnedTab],
          regularTabs: [activeTab]
        },
        onEssentialDragStart: vi.fn(),
        onEssentialDrop: vi.fn(),
        onEssentialReorderDrop: vi.fn(),
        onFavoriteDragStart: vi.fn(),
        onFavoriteDrop: vi.fn(),
        onFavoriteReorderDrop: vi.fn(),
        onFavoriteTabDrop: vi.fn(),
        onQuickEntryContextMenu: vi.fn(),
        onTabContextMenu: vi.fn(),
        onTabDrop: vi.fn(),
        onTabsDrop: vi.fn(),
        onRenameTab: vi.fn(),
        onTabGroupContextMenu: vi.fn(),
        onToggleSection: vi.fn(),
        setDraggingEssentialId: vi.fn(),
        setDraggingFavoriteId: vi.fn(),
        setDraggingGroupId: vi.fn(),
        setDraggingTabId: vi.fn(),
        splitTabs: [],
        activeSplitId: null,
        workspaceTabs: [activeTab, pinnedTab, favoriteTab]
      })));
    });

    const firstHeader = container.querySelector<HTMLButtonElement>(".sidebar-section-header-button")!;
    firstHeader.focus();

    act(() => {
      firstHeader.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "ArrowDown" }));
    });
    // Favorite tabs now render as tab-button (inside TabRow), not favorite-button.
    expect(document.activeElement?.classList.contains("tab-button")).toBe(true);
    expect(document.activeElement?.getAttribute("aria-label")).toContain("MDN");

    act(() => {
      document.activeElement?.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "End" }));
    });
    expect(document.activeElement?.classList.contains("tab-button")).toBe(true);
    expect(document.activeElement?.textContent).toContain("Docs");

    act(() => {
      document.activeElement?.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "Home" }));
    });
    expect(document.activeElement).toBe(firstHeader);

    act(() => root.unmount());
    container.remove();
  });

  it("renders only the Favorites section header among quick-entry folders", () => {
    const activeTab = createTab("Docs", "https://docs.example");
    const favoriteTab = createFavoriteTab("MDN", "https://developer.mozilla.org");
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
          essentials: [createFavorite("Calendar", "https://calendar.example")],
          favorites: [{ kind: "tab", tab: favoriteTab }],
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
        onQuickEntryContextMenu: vi.fn(),
        onTabContextMenu: vi.fn(),
        onTabDrop: vi.fn(),
        onTabsDrop: vi.fn(),
        onRenameTab: vi.fn(),
        onTabGroupContextMenu: vi.fn(),
        onToggleSection: vi.fn(),
        setDraggingEssentialId: vi.fn(),
        setDraggingFavoriteId: vi.fn(),
        setDraggingGroupId: vi.fn(),
        setDraggingTabId: vi.fn(),
        splitTabs: [],
        activeSplitId: null,
        workspaceTabs: [activeTab, favoriteTab]
      }));
    });

    const headers = Array.from(container.querySelectorAll(".sidebar-section-header-button"))
      .map((header) => header.textContent?.replace(/\d+$/, ""));
    expect(headers).toEqual(["Favorites"]);

    act(() => root.unmount());
  });

  it("includes tab group toggles in sidebar item focus navigation", () => {
    const groupedTab = { ...createTab("Docs", "https://docs.example"), groupId: "group" };
    const favoriteTab = createFavoriteTab("MDN", "https://developer.mozilla.org");
    const container = document.createElement("div");
    document.body.append(container);
    const root = createRoot(container);

    act(() => {
      root.render(createElement("div", {
        onKeyDown: handleSidebarFocusNavigation
      }, createElement(SidebarSections, {
        actions: createActions(),
        activeTab: groupedTab,
        draggingEssentialId: null,
        draggingFavoriteId: null,
        draggingGroupId: null,
        draggingTabId: null,
        filteredItems: {
          essentials: [],
          favorites: [{ kind: "tab", tab: favoriteTab }],
          groupedTabs: [{
            group: { color: "#7dd3fc", id: "group", isCollapsed: false, name: "Research" },
            tabs: [groupedTab]
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
        onFavoriteTabDrop: vi.fn(),
        onQuickEntryContextMenu: vi.fn(),
        onTabContextMenu: vi.fn(),
        onTabDrop: vi.fn(),
        onTabsDrop: vi.fn(),
        onRenameTab: vi.fn(),
        onTabGroupContextMenu: vi.fn(),
        onToggleSection: vi.fn(),
        setDraggingEssentialId: vi.fn(),
        setDraggingFavoriteId: vi.fn(),
        setDraggingGroupId: vi.fn(),
        setDraggingTabId: vi.fn(),
        splitTabs: [],
        activeSplitId: null,
        workspaceTabs: [groupedTab]
      })));
    });

    const toggle = container.querySelector<HTMLButtonElement>(".tab-group-toggle")!;
    toggle.focus();

    act(() => {
      toggle.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "ArrowDown" }));
    });
    expect(document.activeElement?.classList.contains("tab-button")).toBe(true);

    act(() => root.unmount());
    container.remove();
  });

  it("keeps tab group titles out of the focus order while the toggle owns folder navigation", () => {
    const groupedTab = { ...createTab("Docs", "https://docs.example"), groupId: "group" };
    const container = document.createElement("div");
    document.body.append(container);
    const root = createRoot(container);

    act(() => {
      root.render(createElement("div", {
        onKeyDown: handleSidebarFocusNavigation
      }, createElement(SidebarSections, {
        actions: createActions(),
        activeTab: groupedTab,
        draggingEssentialId: null,
        draggingFavoriteId: null,
        draggingGroupId: null,
        draggingTabId: null,
        filteredItems: {
          essentials: [],
          favorites: [],
          groupedTabs: [{
            group: { color: "#7dd3fc", id: "group", isCollapsed: false, name: "Research" },
            tabs: [groupedTab]
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
        onFavoriteTabDrop: vi.fn(),
        onQuickEntryContextMenu: vi.fn(),
        onTabContextMenu: vi.fn(),
        onTabDrop: vi.fn(),
        onTabsDrop: vi.fn(),
        onRenameTab: vi.fn(),
        onTabGroupContextMenu: vi.fn(),
        onToggleSection: vi.fn(),
        setDraggingEssentialId: vi.fn(),
        setDraggingFavoriteId: vi.fn(),
        setDraggingGroupId: vi.fn(),
        setDraggingTabId: vi.fn(),
        splitTabs: [],
        activeSplitId: null,
        workspaceTabs: [groupedTab]
      })));
    });

    const toggle = container.querySelector<HTMLButtonElement>(".tab-group-toggle")!;
    toggle.focus();

    act(() => {
      toggle.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "ArrowDown" }));
    });

    expect(container.querySelector(".tab-group-title")?.textContent).toBe("Research");
    expect(container.querySelector(".tab-group-title-input")).toBeNull();
    expect(document.activeElement?.classList.contains("tab-button")).toBe(true);

    act(() => root.unmount());
    container.remove();
  });

  it("restores focus to the current sidebar item after leaving empty search", () => {
    const container = document.createElement("section");
    container.innerHTML = `
      <button class="sidebar-section-header-button" type="button">Tabs</button>
      <div class="tab-row" aria-current="false">
        <button class="tab-button" type="button">Background</button>
      </div>
      <div class="tab-row" aria-current="true">
        <button class="tab-button" type="button">Current</button>
      </div>
    `;
    document.body.append(container);

    expect(focusCurrentOrFirstSidebarItem(container)).toBe(true);
    expect(document.activeElement?.textContent).toBe("Current");

    container.remove();
  });

  it("falls back to the first visible sidebar item when no current item is rendered", () => {
    const container = document.createElement("section");
    container.innerHTML = `
      <button class="sidebar-section-header-button" type="button">Essentials</button>
      <button class="favorite-button" type="button">Docs</button>
    `;
    document.body.append(container);

    expect(focusCurrentOrFirstSidebarItem(container)).toBe(true);
    expect(document.activeElement?.textContent).toBe("Essentials");

    container.remove();
  });

  it("scrolls the current sidebar item into view without moving focus", () => {
    const scrollIntoView = vi.fn();
    const container = document.createElement("section");
    container.innerHTML = `
      <button class="sidebar-section-header-button" type="button">Tabs</button>
      <div class="tab-row" aria-current="true">
        <button class="tab-button" type="button">Current</button>
      </div>
    `;
    const currentButton = container.querySelector<HTMLButtonElement>(".tab-button")!;
    currentButton.scrollIntoView = scrollIntoView;
    document.body.append(container);

    expect(scrollCurrentSidebarItemIntoView(container)).toBe(true);
    expect(scrollIntoView).toHaveBeenCalledWith({ block: "nearest", inline: "nearest" });
    expect(document.activeElement).not.toBe(currentButton);

    container.remove();
  });

  it("does not scroll when no current sidebar item is rendered", () => {
    const container = document.createElement("section");
    container.innerHTML = '<button class="sidebar-section-header-button" type="button">Tabs</button>';

    expect(scrollCurrentSidebarItemIntoView(container)).toBe(false);
  });
});

function createActions() {
  return {
    assignTabToGroup: vi.fn(),
    closeTab: vi.fn(),
    groupTab: vi.fn(),
    openGlance: vi.fn(),
    openTabInSplit: vi.fn(),
    openUrlInSplit: vi.fn(),
    restoreClosedTab: vi.fn(),
    selectTab: vi.fn(),
    toggleTabGroupCollapsed: vi.fn(),
    ungroupTab: vi.fn(),
    updateTabGroup: vi.fn()
  } as unknown as BrowserController["actions"];
}
