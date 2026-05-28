import { createElement } from "react";
import { renderToStaticMarkup } from "react-dom/server";
import { readFileSync } from "node:fs";
import { join } from "node:path";
import { describe, expect, it, vi } from "vitest";

import { createTab } from "../src/renderer/domain/browser";
import type { BrowserController } from "../src/renderer/app/controller/types";
import { SidebarPinnedTabs } from "../src/renderer/surfaces/sidebar/components/tabs/SidebarPinnedTabs";

const sidebarCss = readFileSync(join(__dirname, "../src/renderer/styles/sidebar.css"), "utf8");
const actionHintCss = readFileSync(join(__dirname, "../src/renderer/styles/sidebar-action-hints.css"), "utf8");
const dropZoneCss = readFileSync(join(__dirname, "../src/renderer/styles/sidebar-drop-zones.css"), "utf8");

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
      onPinDrop: vi.fn(),
      pinnedTabs: [pinned],
      setDraggingTabId: vi.fn(),
      splitTabIds: []
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
      onPinDrop: vi.fn(),
      pinnedTabs: [pinned],
      setDraggingTabId: vi.fn(),
      splitTabIds: []
    }));

    expect(html).toContain('data-dragging="true"');
  });

  it("renders preview and split hints for pinned tab buttons", () => {
    const pinned = { ...createTab("Mail", "https://mail.example"), isPinned: true };
    const html = renderToStaticMarkup(createElement(SidebarPinnedTabs, {
      actions: createActions(),
      activeTab: pinned,
      draggingTabId: null,
      onTabContextMenu: vi.fn(),
      onTabDrop: vi.fn(),
      onPinDrop: vi.fn(),
      pinnedTabs: [pinned],
      setDraggingTabId: vi.fn(),
      splitTabIds: []
    }));

    expect(html).toContain('class="sidebar-item-action-hints"');
    expect(html).toContain("Alt");
    expect(html).toContain("Preview");
    expect(html).toContain("Shift");
    expect(html).toContain("Split");
  });

  it("renders compact status badges for split and muted pinned tabs", () => {
    const pinned = { ...createTab("Mail", "https://mail.example"), isMuted: true, isPinned: true };
    const html = renderToStaticMarkup(createElement(SidebarPinnedTabs, {
      actions: createActions(),
      activeTab: pinned,
      draggingTabId: null,
      onTabContextMenu: vi.fn(),
      onTabDrop: vi.fn(),
      onPinDrop: vi.fn(),
      pinnedTabs: [pinned],
      setDraggingTabId: vi.fn(),
      splitTabIds: [pinned.id]
    }));

    expect(html).toContain('class="pinned-tab-status-badges"');
    expect(html).toContain('aria-label="Split, Muted"');
    expect(html).toContain('class="pinned-tab-status-badge is-split"');
    expect(html).toContain('class="pinned-tab-status-badge is-muted"');
  });

  it("collapses pinned tab contents behind the section header", () => {
    const pinned = { ...createTab("Mail", "https://mail.example"), isPinned: true };
    const html = renderToStaticMarkup(createElement(SidebarPinnedTabs, {
      actions: createActions(),
      activeTab: pinned,
      draggingTabId: null,
      isCollapsed: true,
      onTabContextMenu: vi.fn(),
      onTabDrop: vi.fn(),
      onPinDrop: vi.fn(),
      onToggle: vi.fn(),
      pinnedTabs: [pinned],
      setDraggingTabId: vi.fn(),
      splitTabIds: []
    }));

    expect(html).toContain('aria-expanded="false"');
    expect(html).toContain("Pinned");
    expect(html).not.toContain('aria-label="Pinned tabs"');
  });

  it("renders an empty pinned drop target while a tab is dragging", () => {
    const activeTab = createTab("Mail", "https://mail.example");
    const html = renderToStaticMarkup(createElement(SidebarPinnedTabs, {
      actions: createActions(),
      activeTab,
      draggingTabId: activeTab.id,
      onTabContextMenu: vi.fn(),
      onTabDrop: vi.fn(),
      onPinDrop: vi.fn(),
      pinnedTabs: [],
      setDraggingTabId: vi.fn(),
      splitTabIds: []
    }));

    expect(html).toContain('aria-label="Pinned tabs"');
    expect(html).toContain('data-drop-target="true"');
    expect(html).toContain("Drop to pin");
  });

  it("styles pinned tab drag and drop states", () => {
    expect(sidebarCss).toContain('.pinned-tab-button[data-dragging="true"]');
    expect(sidebarCss).toContain('.pinned-tab-button[data-drop-target="true"]');
    expect(dropZoneCss).toContain('.pinned-tabs[data-drop-target="true"]');
    expect(dropZoneCss).toContain('.favorites[data-drop-target="true"]');
    expect(dropZoneCss).toContain('.favorite-button[data-dragging="true"]');
    expect(dropZoneCss).toContain('.favorite-button[data-drop-target="true"]');
    expect(dropZoneCss).toContain(".sidebar-drop-empty");
    expect(sidebarCss).toContain("cursor: grabbing");
  });

  it("styles compact pinned tab status badges", () => {
    expect(sidebarCss).toContain(".pinned-tab-status-badges");
    expect(sidebarCss).toContain(".pinned-tab-status-badge.is-split");
    expect(sidebarCss).toContain(".pinned-tab-status-badge.is-muted");
  });

  it("styles collapsible section headers", () => {
    expect(sidebarCss).toContain(".sidebar-section-header-button");
    expect(sidebarCss).toContain(".sidebar-section-header-button:focus-visible");
    expect(sidebarCss).toContain(".sidebar-section-title svg");
  });

  it("styles pinned tab action hints as hover and focus floaters", () => {
    expect(actionHintCss).toContain(".pinned-tab-button .sidebar-item-action-hints");
    expect(actionHintCss).toContain(".pinned-tab-button:hover .sidebar-item-action-hints");
    expect(actionHintCss).toContain(".pinned-tab-button:focus-visible .sidebar-item-action-hints");
  });
});
