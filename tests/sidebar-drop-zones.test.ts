import { createElement } from "react";
import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, it, vi } from "vitest";

import { createTab } from "../src/renderer/domain/browser";
import type { BrowserController } from "../src/renderer/app/controller/types";
import { SidebarSections } from "../src/renderer/surfaces/sidebar/components/tabs/SidebarSections";

describe("sidebar section drop zones", () => {
  it("shows Pinned and Favorites drop targets while dragging a regular tab", () => {
    const tab = createTab("Docs", "https://docs.example");
    const html = renderToStaticMarkup(createElement(SidebarSections, {
      actions: createActions(),
      activeTab: tab,
      closedTabs: [],
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
      onFavoriteDrop: vi.fn(),
      onPinDrop: vi.fn(),
      onQuickEntryContextMenu: vi.fn(),
      onTabContextMenu: vi.fn(),
      onTabDrop: vi.fn(),
      setDraggingTabId: vi.fn(),
      splitTabIds: []
    }));

    expect(html).toContain("Drop to pin");
    expect(html).toContain("Drop to favorite");
    expect(html).toContain('aria-label="Pinned tabs"');
    expect(html).toContain('aria-label="Favorites"');
  });
});

function createActions() {
  return {
    assignTabToGroup: vi.fn(),
    closeTab: vi.fn(),
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
