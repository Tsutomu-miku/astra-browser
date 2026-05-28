import { createElement } from "react";
import { renderToStaticMarkup } from "react-dom/server";
import { readFileSync } from "node:fs";
import { join } from "node:path";
import { describe, expect, it, vi } from "vitest";

import { createTab } from "../src/renderer/domain/browser";
import type { BrowserController } from "../src/renderer/app/controller/types";
import { SidebarPinnedTabs } from "../src/renderer/surfaces/sidebar/components/tabs/SidebarPinnedTabs";

const sidebarCss = readFileSync(join(__dirname, "../src/renderer/styles/sidebar.css"), "utf8");

function createActions() {
  return {
    openGlance: vi.fn(),
    openTabInSplit: vi.fn(),
    selectTab: vi.fn()
  } as unknown as BrowserController["actions"];
}

describe("sidebar pinned tabs", () => {
  it("renders pinned tab buttons as draggable reorder targets", () => {
    const pinned = { ...createTab("Mail", "https://mail.example"), isPinned: true };
    const html = renderToStaticMarkup(createElement(SidebarPinnedTabs, {
      actions: createActions(),
      activeTab: pinned,
      draggingTabId: "other-tab",
      onTabContextMenu: vi.fn(),
      onTabDrop: vi.fn(),
      pinnedTabs: [pinned],
      setDraggingTabId: vi.fn()
    }));

    expect(html).toContain('aria-label="Pinned tabs"');
    expect(html).toContain(`id="sidebar-search-tab-${pinned.id}"`);
    expect(html).toContain('draggable="true"');
    expect(html).toContain('data-drop-target="true"');
  });

  it("marks the dragged pinned tab", () => {
    const pinned = { ...createTab("Mail", "https://mail.example"), isPinned: true };
    const html = renderToStaticMarkup(createElement(SidebarPinnedTabs, {
      actions: createActions(),
      activeTab: pinned,
      draggingTabId: pinned.id,
      onTabContextMenu: vi.fn(),
      onTabDrop: vi.fn(),
      pinnedTabs: [pinned],
      setDraggingTabId: vi.fn()
    }));

    expect(html).toContain('data-dragging="true"');
  });

  it("styles pinned tab drag and drop states", () => {
    expect(sidebarCss).toContain('.pinned-tab-button[data-dragging="true"]');
    expect(sidebarCss).toContain('.pinned-tab-button[data-drop-target="true"]');
    expect(sidebarCss).toContain("cursor: grabbing");
  });
});
