import { createElement } from "react";
import { act } from "react";
import { createRoot } from "react-dom/client";
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
    closeTab: vi.fn(),
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
    expect(html).toContain('aria-label="Mail, active, pinned tab"');
    expect(html).not.toContain('title="Mail"');
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

  it("closes pinned tabs on middle click without selecting first", () => {
    const pinned = { ...createTab("Mail", "https://mail.example"), isPinned: true };
    const actions = createActions();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(SidebarPinnedTabs, {
        actions,
        activeTab: { ...createTab("Active", "https://active.example") },
        draggingTabId: null,
        onTabContextMenu: vi.fn(),
        onTabDrop: vi.fn(),
        onPinDrop: vi.fn(),
        pinnedTabs: [pinned],
        setDraggingTabId: vi.fn(),
        splitTabIds: []
      }));
    });

    const button = container.querySelector(".pinned-tab-button");
    button?.dispatchEvent(new MouseEvent("auxclick", { bubbles: true, button: 1 }));

    expect(actions.closeTab).toHaveBeenCalledWith(pinned.id);
    expect(actions.selectTab).not.toHaveBeenCalled();

    act(() => root.unmount());
  });

  it("closes focused pinned tabs with Backspace without selecting first", () => {
    const pinned = { ...createTab("Mail", "https://mail.example"), isPinned: true };
    const actions = createActions();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(SidebarPinnedTabs, {
        actions,
        activeTab: { ...createTab("Active", "https://active.example") },
        draggingTabId: null,
        onTabContextMenu: vi.fn(),
        onTabDrop: vi.fn(),
        onPinDrop: vi.fn(),
        pinnedTabs: [pinned],
        setDraggingTabId: vi.fn(),
        splitTabIds: []
      }));
    });

    const button = container.querySelector(".pinned-tab-button");
    button?.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "Backspace" }));

    expect(actions.closeTab).toHaveBeenCalledWith(pinned.id);
    expect(actions.selectTab).not.toHaveBeenCalled();

    act(() => root.unmount());
  });

  it("keeps pinned tab close keys from bubbling to sidebar navigation", () => {
    const pinned = { ...createTab("Mail", "https://mail.example"), isPinned: true };
    const actions = createActions();
    const onParentKeyDown = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement("div", {
        onKeyDown: onParentKeyDown
      }, createElement(SidebarPinnedTabs, {
        actions,
        activeTab: { ...createTab("Active", "https://active.example") },
        draggingTabId: null,
        onTabContextMenu: vi.fn(),
        onTabDrop: vi.fn(),
        onPinDrop: vi.fn(),
        pinnedTabs: [pinned],
        setDraggingTabId: vi.fn(),
        splitTabIds: []
      })));
    });

    container.querySelector(".pinned-tab-button")?.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "Delete" }));

    expect(actions.closeTab).toHaveBeenCalledWith(pinned.id);
    expect(onParentKeyDown).not.toHaveBeenCalled();

    act(() => root.unmount());
  });

  it("runs pinned tab keyboard preview and split activation", () => {
    const pinned = { ...createTab("Mail", "https://mail.example"), isPinned: true };
    const actions = createActions();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(SidebarPinnedTabs, {
        actions,
        activeTab: { ...createTab("Active", "https://active.example") },
        draggingTabId: null,
        onTabContextMenu: vi.fn(),
        onTabDrop: vi.fn(),
        onPinDrop: vi.fn(),
        pinnedTabs: [pinned],
        setDraggingTabId: vi.fn(),
        splitTabIds: []
      }));
    });

    const button = container.querySelector(".pinned-tab-button");
    button?.dispatchEvent(new KeyboardEvent("keydown", { altKey: true, bubbles: true, key: "Enter" }));
    button?.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "Enter", shiftKey: true }));

    expect(actions.openGlance).toHaveBeenCalledWith(pinned.url, pinned.title);
    expect(actions.openTabInSplit).toHaveBeenCalledWith(pinned.id);
    expect(actions.selectTab).not.toHaveBeenCalled();

    act(() => root.unmount());
  });

  it("opens pinned tab context menus from the keyboard", () => {
    const pinned = { ...createTab("Mail", "https://mail.example"), isPinned: true };
    const onTabContextMenu = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(SidebarPinnedTabs, {
        actions: createActions(),
        activeTab: pinned,
        draggingTabId: null,
        onTabContextMenu,
        onTabDrop: vi.fn(),
        onPinDrop: vi.fn(),
        pinnedTabs: [pinned],
        setDraggingTabId: vi.fn(),
        splitTabIds: []
      }));
    });

    container.querySelector(".pinned-tab-button")?.dispatchEvent(new KeyboardEvent("keydown", {
      bubbles: true,
      key: "ContextMenu"
    }));

    expect(onTabContextMenu).toHaveBeenCalledWith(expect.objectContaining({ type: "contextmenu" }), pinned);

    act(() => root.unmount());
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
    expect(html).toContain('aria-label="Mail, active, pinned tab, Split, Muted"');
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
    expect(dropZoneCss).toContain('.pinned-tab-button[data-drop-placement]::before');
    expect(dropZoneCss).toContain('.tab-row[data-drop-placement]::before');
    expect(dropZoneCss).toContain('.essentials .favorite-button[data-drop-placement]::before');
    expect(dropZoneCss).toContain('.essentials[data-drop-target="true"]');
    expect(dropZoneCss).toContain('.favorites[data-drop-target="true"]');
    expect(dropZoneCss).toContain('.favorite-button[data-dragging="true"]');
    expect(dropZoneCss).toContain('.favorite-button[data-drop-target="true"]');
    expect(dropZoneCss).toContain(".tab-organization-drop-target");
    expect(dropZoneCss).toContain("font: inherit");
    expect(dropZoneCss).toContain("cursor: pointer");
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
