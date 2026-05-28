import { createElement } from "react";
import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, it, vi } from "vitest";

import { createFavorite, createTab } from "../src/renderer/domain/browser";
import { FavoriteButton, TabRow } from "../src/renderer/surfaces/sidebar/components/tabs/SidebarItems";

describe("sidebar item action hints", () => {
  it("renders preview and split hints for tab rows", () => {
    const tab = createTab("Docs", "https://docs.example");
    const html = renderToStaticMarkup(createElement(TabRow, {
      activeTabId: tab.id,
      draggingTabId: null,
      onClose: vi.fn(),
      onContextMenu: vi.fn(),
      onDrop: vi.fn(),
      onPreview: vi.fn(),
      onSelect: vi.fn(),
      onSplit: vi.fn(),
      setDraggingTabId: vi.fn(),
      splitTabIds: [],
      tab
    }));

    expect(html).toContain('class="sidebar-item-action-hints"');
    expect(html).toContain("Alt");
    expect(html).toContain("Preview");
    expect(html).toContain("Shift");
    expect(html).toContain("Split");
  });

  it("renders preview and split hints for favorite rows", () => {
    const html = renderToStaticMarkup(createElement(FavoriteButton, {
      favorite: createFavorite("Docs", "https://docs.example"),
      onOpen: vi.fn(),
      onOpenInSplit: vi.fn(),
      onPreview: vi.fn()
    }));

    expect(html).toContain('class="sidebar-item-action-hints"');
    expect(html).toContain("Alt");
    expect(html).toContain("Preview");
    expect(html).toContain("Shift");
    expect(html).toContain("Split");
  });
});
