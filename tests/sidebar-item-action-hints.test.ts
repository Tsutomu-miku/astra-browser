import { readFileSync } from "node:fs";
import { join } from "node:path";
import { createElement } from "react";
import { act } from "react";
import { createRoot } from "react-dom/client";
import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, it, vi } from "vitest";

import { createFavorite, createTab } from "../src/renderer/domain/browser";
import { SIDEBAR_TAB_DRAG_TYPE } from "../src/renderer/common/drag-drop/sidebarDragPayload";
import { FavoriteButton, TabRow } from "../src/renderer/surfaces/sidebar/components/tabs/SidebarItems";

const sidebarCss = readFileSync(join(__dirname, "../src/renderer/styles/sidebar.css"), "utf8");
const sidebarActionHintsCss = readFileSync(join(__dirname, "../src/renderer/styles/sidebar-action-hints.css"), "utf8");

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
    expect(html).toContain('aria-hidden="true"');
    expect(html).toContain('data-action-hint="preview"');
    expect(html).toContain('data-action-hint="split"');
    expect(html).toContain('aria-label="Docs, active, tab"');
    expect(html).toContain('aria-label="Close Docs"');
    expect(html).not.toContain('title="Close tab"');
    expect(html).not.toContain("<kbd");
    expect(html).not.toContain(">Preview<");
    expect(html).not.toContain(">Split<");
  });

  it("closes tab rows on middle click without selecting first", () => {
    const tab = createTab("Docs", "https://docs.example");
    const onClose = vi.fn();
    const onSelect = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(TabRow, {
        activeTabId: "other-tab",
        draggingTabId: null,
        onClose,
        onContextMenu: vi.fn(),
        onDrop: vi.fn(),
        onPreview: vi.fn(),
        onSelect,
        onSplit: vi.fn(),
        setDraggingTabId: vi.fn(),
        splitTabIds: [],
        tab
      }));
    });

    const button = container.querySelector(".tab-button");
    button?.dispatchEvent(new MouseEvent("auxclick", { bubbles: true, button: 1 }));

    expect(onClose).toHaveBeenCalledWith(tab.id);
    expect(onSelect).not.toHaveBeenCalled();

    act(() => root.unmount());
  });

  it("closes focused tab rows with Delete without selecting first", () => {
    const tab = createTab("Docs", "https://docs.example");
    const onClose = vi.fn();
    const onSelect = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(TabRow, {
        activeTabId: "other-tab",
        draggingTabId: null,
        onClose,
        onContextMenu: vi.fn(),
        onDrop: vi.fn(),
        onPreview: vi.fn(),
        onSelect,
        onSplit: vi.fn(),
        setDraggingTabId: vi.fn(),
        splitTabIds: [],
        tab
      }));
    });

    const button = container.querySelector(".tab-button");
    button?.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "Delete" }));

    expect(onClose).toHaveBeenCalledWith(tab.id);
    expect(onSelect).not.toHaveBeenCalled();

    act(() => root.unmount());
  });

  it("keeps tab row close keys from bubbling to sidebar navigation", () => {
    const tab = createTab("Docs", "https://docs.example");
    const onClose = vi.fn();
    const onParentKeyDown = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement("div", {
        onKeyDown: onParentKeyDown
      }, createElement(TabRow, {
        activeTabId: "other-tab",
        draggingTabId: null,
        onClose,
        onContextMenu: vi.fn(),
        onDrop: vi.fn(),
        onPreview: vi.fn(),
        onSelect: vi.fn(),
        onSplit: vi.fn(),
        setDraggingTabId: vi.fn(),
        splitTabIds: [],
        tab
      })));
    });

    container.querySelector(".tab-button")?.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "Backspace" }));

    expect(onClose).toHaveBeenCalledWith(tab.id);
    expect(onParentKeyDown).not.toHaveBeenCalled();

    act(() => root.unmount());
  });

  it("runs tab row keyboard preview and split activation", () => {
    const tab = createTab("Docs", "https://docs.example");
    const onPreview = vi.fn();
    const onSelect = vi.fn();
    const onSplit = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(TabRow, {
        activeTabId: "other-tab",
        draggingTabId: null,
        onClose: vi.fn(),
        onContextMenu: vi.fn(),
        onDrop: vi.fn(),
        onPreview,
        onSelect,
        onSplit,
        setDraggingTabId: vi.fn(),
        splitTabIds: [],
        tab
      }));
    });

    const button = container.querySelector(".tab-button");
    button?.dispatchEvent(new KeyboardEvent("keydown", { altKey: true, bubbles: true, key: "Enter" }));
    button?.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "Enter", shiftKey: true }));

    expect(onPreview).toHaveBeenCalledWith(tab.url, tab.title);
    expect(onSplit).toHaveBeenCalledWith(tab.id);
    expect(onSelect).not.toHaveBeenCalled();

    act(() => root.unmount());
  });

  it("opens tab row context menus from the keyboard", () => {
    const tab = createTab("Docs", "https://docs.example");
    const onContextMenu = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(TabRow, {
        activeTabId: "other-tab",
        draggingTabId: null,
        onClose: vi.fn(),
        onContextMenu,
        onDrop: vi.fn(),
        onPreview: vi.fn(),
        onSelect: vi.fn(),
        onSplit: vi.fn(),
        setDraggingTabId: vi.fn(),
        splitTabIds: [],
        tab
      }));
    });

    container.querySelector(".tab-button")?.dispatchEvent(new KeyboardEvent("keydown", {
      bubbles: true,
      key: "F10",
      shiftKey: true
    }));

    expect(onContextMenu).toHaveBeenCalledWith(expect.objectContaining({ type: "contextmenu" }), tab);

    act(() => root.unmount());
  });

  it("opens favorite context menus from the keyboard", () => {
    const favorite = createFavorite("Docs", "https://docs.example");
    const onContextMenu = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(FavoriteButton, {
        favorite,
        onContextMenu,
        onOpen: vi.fn(),
        onOpenInSplit: vi.fn(),
        onPreview: vi.fn()
      }));
    });

    container.querySelector(".favorite-button")?.dispatchEvent(new KeyboardEvent("keydown", {
      bubbles: true,
      key: "ContextMenu"
    }));

    expect(onContextMenu).toHaveBeenCalledWith(expect.objectContaining({ type: "contextmenu" }), favorite);

    act(() => root.unmount());
  });

  it("uses the whole visible tab row as the native drag source", () => {
    const tab = createTab("Docs", "https://docs.example");
    const setDraggingTabId = vi.fn();
    const data = new Map<string, string>();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(TabRow, {
        activeTabId: tab.id,
        draggingTabId: null,
        onClose: vi.fn(),
        onContextMenu: vi.fn(),
        onDrop: vi.fn(),
        onPreview: vi.fn(),
        onSelect: vi.fn(),
        onSplit: vi.fn(),
        setDraggingTabId,
        splitTabIds: [],
        tab
      }));
    });

    const event = new Event("dragstart", { bubbles: true });
    const dataTransfer = {
      effectAllowed: "",
      setData: (type: string, value: string) => data.set(type, value)
    };
    Object.defineProperty(event, "dataTransfer", {
      value: dataTransfer
    });
    container.querySelector(".tab-row")?.dispatchEvent(event);

    expect(container.querySelector(".tab-row")?.getAttribute("draggable")).toBe("true");
    expect(container.querySelector(".tab-button")?.getAttribute("draggable")).toBe("false");
    expect(setDraggingTabId).toHaveBeenCalledWith(tab.id);
    expect(dataTransfer.effectAllowed).toBe("move");
    expect(data.get(SIDEBAR_TAB_DRAG_TYPE)).toBe(tab.id);
    expect(data.get("text/plain")).toBe(tab.id);

    act(() => root.unmount());
  });

  it("uses in-flow action hints so hover labels do not cover tab titles", () => {
    expect(sidebarCss).toContain("grid-template-columns: 24px minmax(0, 1fr) 38px");
    expect(sidebarCss).toContain("grid-template-columns: 22px minmax(0, 1fr) 38px");
    expect(sidebarActionHintsCss).toContain("width: 38px");
    expect(sidebarActionHintsCss).toContain("min-width: 38px");
    expect(sidebarActionHintsCss).not.toContain("max-width");
    expect(sidebarActionHintsCss).toContain("width: 16px");
    expect(sidebarActionHintsCss).not.toContain("kbd");
    expect(sidebarActionHintsCss).not.toContain("right: 6px");
  });

  it("renders preview and split hints for favorite rows", () => {
    const favorite = createFavorite("Docs", "https://docs.example");
    const html = renderToStaticMarkup(createElement(FavoriteButton, {
      draggingQuickEntryId: "other-favorite",
      draggable: true,
      favorite,
      isActive: true,
      isSearchSelected: true,
      kind: "favorite",
      onOpen: vi.fn(),
      onOpenInSplit: vi.fn(),
      onPreview: vi.fn()
    }));

    expect(html).toContain('class="sidebar-item-action-hints"');
    expect(html).toContain('aria-hidden="true"');
    expect(html).toContain('data-action-hint="preview"');
    expect(html).toContain('data-action-hint="split"');
    expect(html).toContain('aria-label="Docs, Favorite, current page, selected search result, drop target"');
    expect(html).not.toContain('title="https://docs.example"');
    expect(html).not.toContain("<kbd");
    expect(html).not.toContain(">Preview<");
    expect(html).not.toContain(">Split<");
  });

  it("runs favorite keyboard open, preview, and split activation", () => {
    const favorite = createFavorite("Docs", "https://docs.example");
    const onOpen = vi.fn();
    const onOpenInSplit = vi.fn();
    const onPreview = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(FavoriteButton, {
        favorite,
        onOpen,
        onOpenInSplit,
        onPreview
      }));
    });

    const button = container.querySelector(".favorite-button");
    button?.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "Enter" }));
    button?.dispatchEvent(new KeyboardEvent("keydown", { altKey: true, bubbles: true, key: "Enter" }));
    button?.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "Enter", shiftKey: true }));

    expect(onOpen).toHaveBeenCalledWith(favorite.url, favorite.title);
    expect(onPreview).toHaveBeenCalledWith(favorite.url, favorite.title);
    expect(onOpenInSplit).toHaveBeenCalledWith(favorite.url, favorite.title);

    act(() => root.unmount());
  });

  it("does not run quick entry activation when opening its keyboard context menu", () => {
    const favorite = createFavorite("Docs", "https://docs.example");
    const onContextMenu = vi.fn();
    const onOpen = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(FavoriteButton, {
        favorite,
        onContextMenu,
        onOpen,
        onOpenInSplit: vi.fn(),
        onPreview: vi.fn()
      }));
    });

    container.querySelector(".favorite-button")?.dispatchEvent(new KeyboardEvent("keydown", {
      bubbles: true,
      key: "ContextMenu"
    }));

    expect(onContextMenu).toHaveBeenCalledWith(expect.objectContaining({ type: "contextmenu" }), favorite);
    expect(onOpen).not.toHaveBeenCalled();

    act(() => root.unmount());
  });
});
