import { createElement } from "react";
import { act } from "react";
import { createRoot } from "react-dom/client";
import { describe, expect, it, vi } from "vitest";

import { createFavorite, createTab } from "../src/renderer/domain/browser";
import type { BrowserController } from "../src/renderer/app/controller/types";
import { SidebarSections } from "../src/renderer/surfaces/sidebar/components/tabs/SidebarSections";
import { focusCurrentOrFirstSidebarItem, handleSidebarFocusNavigation } from "../src/renderer/surfaces/sidebar/model/sidebarFocusNavigation";

describe("sidebar focus navigation", () => {
  it("collapses and expands sidebar sections with Left and Right arrows", () => {
    const activeTab = createTab("Docs", "https://docs.example");
    const container = document.createElement("div");
    document.body.append(container);
    const root = createRoot(container);

    act(() => {
      root.render(createElement("div", {
        onKeyDown: handleSidebarFocusNavigation
      }, createElement(SidebarSections, {
        actions: createActions(),
        activeTab,
        closedTabs: [],
        draggingEssentialId: null,
        draggingFavoriteId: null,
        draggingGroupId: null,
        draggingTabId: null,
        filteredItems: {
          essentials: [createFavorite("Calendar", "https://calendar.example")],
          favorites: [],
          groupedTabs: [],
          hasMatches: true,
          isFiltering: false,
          pinnedTabs: [],
          regularTabs: [activeTab]
        },
        onClosedTabContextMenu: vi.fn(),
        onEssentialDragStart: vi.fn(),
        onEssentialDrop: vi.fn(),
        onEssentialReorderDrop: vi.fn(),
        onFavoriteDragStart: vi.fn(),
        onFavoriteDrop: vi.fn(),
        onFavoriteReorderDrop: vi.fn(),
        onPinDrop: vi.fn(),
        onQuickEntryContextMenu: vi.fn(),
        onTabContextMenu: vi.fn(),
        onTabDrop: vi.fn(),
        onTabGroupContextMenu: vi.fn(),
        setDraggingClosedTabIndex: vi.fn(),
        setDraggingEssentialId: vi.fn(),
        setDraggingFavoriteId: vi.fn(),
        setDraggingGroupId: vi.fn(),
        setDraggingTabId: vi.fn(),
        splitTabIds: []
      })));
    });

    const essentialsHeader = Array.from(container.querySelectorAll<HTMLButtonElement>(".sidebar-section-header-button"))
      .find((header) => header.textContent?.includes("Essentials"))!;

    expect(essentialsHeader.getAttribute("aria-expanded")).toBe("true");
    expect(essentialsHeader.getAttribute("aria-label")).toBe("Collapse Essentials");
    expect(container.querySelector(".essentials")).not.toBeNull();

    act(() => {
      essentialsHeader.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "ArrowLeft" }));
    });

    expect(essentialsHeader.getAttribute("aria-expanded")).toBe("false");
    expect(essentialsHeader.getAttribute("aria-label")).toBe("Expand Essentials");
    expect(container.querySelector(".essentials")).toBeNull();

    act(() => {
      essentialsHeader.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "ArrowRight" }));
    });

    expect(essentialsHeader.getAttribute("aria-expanded")).toBe("true");
    expect(container.querySelector(".essentials")).not.toBeNull();

    act(() => root.unmount());
    container.remove();
  });

  it("moves focus through visible sidebar items with Arrow, Home, and End", () => {
    const activeTab = createTab("Docs", "https://docs.example");
    const pinnedTab = { ...createTab("Mail", "https://mail.example"), isPinned: true };
    const favorite = createFavorite("MDN", "https://developer.mozilla.org");
    const container = document.createElement("div");
    document.body.append(container);
    const root = createRoot(container);

    act(() => {
      root.render(createElement("div", {
        onKeyDown: handleSidebarFocusNavigation
      }, createElement(SidebarSections, {
        actions: createActions(),
        activeTab,
        closedTabs: [{ title: "Closed", url: "https://closed.example", closedAt: 1 }],
        draggingEssentialId: null,
        draggingFavoriteId: null,
        draggingGroupId: null,
        draggingTabId: null,
        filteredItems: {
          essentials: [createFavorite("Calendar", "https://calendar.example")],
          favorites: [favorite],
          groupedTabs: [],
          hasMatches: true,
          isFiltering: false,
          pinnedTabs: [pinnedTab],
          regularTabs: [activeTab]
        },
        onClosedTabContextMenu: vi.fn(),
        onEssentialDragStart: vi.fn(),
        onEssentialDrop: vi.fn(),
        onEssentialReorderDrop: vi.fn(),
        onFavoriteDragStart: vi.fn(),
        onFavoriteDrop: vi.fn(),
        onFavoriteReorderDrop: vi.fn(),
        onPinDrop: vi.fn(),
        onQuickEntryContextMenu: vi.fn(),
        onTabContextMenu: vi.fn(),
        onTabDrop: vi.fn(),
        onTabGroupContextMenu: vi.fn(),
        setDraggingClosedTabIndex: vi.fn(),
        setDraggingEssentialId: vi.fn(),
        setDraggingFavoriteId: vi.fn(),
        setDraggingGroupId: vi.fn(),
        setDraggingTabId: vi.fn(),
        splitTabIds: []
      })));
    });

    const firstHeader = container.querySelector<HTMLButtonElement>(".sidebar-section-header-button")!;
    firstHeader.focus();

    act(() => {
      firstHeader.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "ArrowDown" }));
    });
    expect(document.activeElement?.classList.contains("favorite-button")).toBe(true);

    act(() => {
      document.activeElement?.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "End" }));
    });
    expect(document.activeElement?.classList.contains("closed-tab-button")).toBe(true);
    expect(document.activeElement?.textContent).toContain("Closed");

    act(() => {
      document.activeElement?.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "Home" }));
    });
    expect(document.activeElement).toBe(firstHeader);

    act(() => root.unmount());
    container.remove();
  });

  it("orders current tab folders before recently closed recovery", () => {
    const activeTab = createTab("Docs", "https://docs.example");
    const pinnedTab = { ...createTab("Mail", "https://mail.example"), isPinned: true };
    const favorite = createFavorite("MDN", "https://developer.mozilla.org");
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(SidebarSections, {
        actions: createActions(),
        activeTab,
        closedTabs: [{ title: "Closed", url: "https://closed.example", closedAt: 1 }],
        draggingEssentialId: null,
        draggingFavoriteId: null,
        draggingGroupId: null,
        draggingTabId: null,
        filteredItems: {
          essentials: [createFavorite("Calendar", "https://calendar.example")],
          favorites: [favorite],
          groupedTabs: [],
          hasMatches: true,
          isFiltering: false,
          pinnedTabs: [pinnedTab],
          regularTabs: [activeTab]
        },
        onClosedTabContextMenu: vi.fn(),
        onEssentialDragStart: vi.fn(),
        onEssentialDrop: vi.fn(),
        onEssentialReorderDrop: vi.fn(),
        onFavoriteDragStart: vi.fn(),
        onFavoriteDrop: vi.fn(),
        onFavoriteReorderDrop: vi.fn(),
        onPinDrop: vi.fn(),
        onQuickEntryContextMenu: vi.fn(),
        onTabContextMenu: vi.fn(),
        onTabDrop: vi.fn(),
        onTabGroupContextMenu: vi.fn(),
        setDraggingClosedTabIndex: vi.fn(),
        setDraggingEssentialId: vi.fn(),
        setDraggingFavoriteId: vi.fn(),
        setDraggingGroupId: vi.fn(),
        setDraggingTabId: vi.fn(),
        splitTabIds: []
      }));
    });

    const headers = Array.from(container.querySelectorAll(".sidebar-section-header-button"))
      .map((header) => header.textContent?.replace(/\d+$/, ""));
    expect(headers).toEqual(["Essentials", "Pinned", "Favorites", "Tabs", "Recently Closed"]);

    act(() => root.unmount());
  });

  it("includes tab group toggles in sidebar item focus navigation", () => {
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
        closedTabs: [],
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
        onClosedTabContextMenu: vi.fn(),
        onEssentialDragStart: vi.fn(),
        onEssentialDrop: vi.fn(),
        onEssentialReorderDrop: vi.fn(),
        onFavoriteDragStart: vi.fn(),
        onFavoriteDrop: vi.fn(),
        onFavoriteReorderDrop: vi.fn(),
        onPinDrop: vi.fn(),
        onQuickEntryContextMenu: vi.fn(),
        onTabContextMenu: vi.fn(),
        onTabDrop: vi.fn(),
        onTabGroupContextMenu: vi.fn(),
        setDraggingClosedTabIndex: vi.fn(),
        setDraggingEssentialId: vi.fn(),
        setDraggingFavoriteId: vi.fn(),
        setDraggingGroupId: vi.fn(),
        setDraggingTabId: vi.fn(),
        splitTabIds: []
      })));
    });

    const tabsHeader = Array.from(container.querySelectorAll<HTMLButtonElement>(".sidebar-section-header-button"))
      .find((button) => button.textContent?.includes("Tabs"))!;
    tabsHeader.focus();

    act(() => {
      tabsHeader.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "ArrowDown" }));
    });
    expect(document.activeElement?.classList.contains("tab-group-toggle")).toBe(true);

    act(() => {
      document.activeElement?.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "ArrowDown" }));
    });
    expect(document.activeElement?.classList.contains("tab-button")).toBe(true);

    act(() => root.unmount());
    container.remove();
  });

  it("keeps tab group title editing keys inside the input", () => {
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
        closedTabs: [],
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
        onClosedTabContextMenu: vi.fn(),
        onEssentialDragStart: vi.fn(),
        onEssentialDrop: vi.fn(),
        onEssentialReorderDrop: vi.fn(),
        onFavoriteDragStart: vi.fn(),
        onFavoriteDrop: vi.fn(),
        onFavoriteReorderDrop: vi.fn(),
        onPinDrop: vi.fn(),
        onQuickEntryContextMenu: vi.fn(),
        onTabContextMenu: vi.fn(),
        onTabDrop: vi.fn(),
        onTabGroupContextMenu: vi.fn(),
        setDraggingClosedTabIndex: vi.fn(),
        setDraggingEssentialId: vi.fn(),
        setDraggingFavoriteId: vi.fn(),
        setDraggingGroupId: vi.fn(),
        setDraggingTabId: vi.fn(),
        splitTabIds: []
      })));
    });

    const input = container.querySelector<HTMLInputElement>(".tab-group-title-input")!;
    input.focus();

    act(() => {
      input.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "ArrowDown" }));
    });

    expect(document.activeElement).toBe(input);

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
