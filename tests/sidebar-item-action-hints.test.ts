import { createElement } from "react";
import { act } from "react";
import { createRoot } from "react-dom/client";
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

  it("uses the visible tab button as the drag source", () => {
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
    Object.defineProperty(event, "dataTransfer", {
      value: {
        effectAllowed: "",
        setData: (type: string, value: string) => data.set(type, value)
      }
    });
    container.querySelector(".tab-button")?.dispatchEvent(event);

    expect(container.querySelector(".tab-button")?.getAttribute("draggable")).toBe("true");
    expect(setDraggingTabId).toHaveBeenCalledWith(tab.id);
    expect(data.get("text/plain")).toBe(tab.id);

    act(() => root.unmount());
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
