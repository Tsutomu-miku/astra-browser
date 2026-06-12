import { readFileSync } from "node:fs";
import { join } from "node:path";
import { createElement } from "react";
import { createRoot } from "react-dom/client";
import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, it, vi } from "vitest";

import { createTab } from "../src/renderer/domain/browser";
import { SplitTabRow } from "../src/renderer/surfaces/sidebar/components/tabs/SplitTabRow";

const sidebarCss = readFileSync(join(__dirname, "../src/renderer/styles/sidebar.css"), "utf8");

describe("sidebar split tab row", () => {
  it("renders a single merged row for split view (Arc-style)", () => {
    const primary = createTab("Docs", "https://docs.example");
    const secondary = createTab("Notes", "https://notes.example");
    const html = renderToStaticMarkup(createElement(SplitTabRow, {
      activeTabId: primary.id,
      draggingTabId: null,
      onClose: vi.fn(),
      onContextMenu: vi.fn(),
      onDrop: vi.fn(),
      onPreview: vi.fn(),
      onSelect: vi.fn(),
      onSwapPanes: vi.fn(),
      primaryTab: primary,
      secondaryTab: secondary,
      setDraggingTabId: vi.fn()
    }));

    expect(html).toContain('class="tab-row split-tab-row"');
    expect(html).toContain("split-tab-favicons");
    expect(html).toContain("split-tab-favicon-primary");
    expect(html).toContain("split-tab-favicon-secondary");
    expect(html).toContain("split-tab-secondary-title");
    expect(html).toContain("Docs");
    expect(html).toContain("Notes");
    expect(html).toContain("split view");
  });

  it("shows split tab row as active when isActive is true (entity model)", () => {
    const primary = createTab("Docs", "https://docs.example");
    const secondary = createTab("Notes", "https://notes.example");
    const html = renderToStaticMarkup(createElement(SplitTabRow, {
      activeTabId: primary.id,
      draggingTabId: null,
      isActive: true,
      onClose: vi.fn(),
      onContextMenu: vi.fn(),
      onDrop: vi.fn(),
      onPreview: vi.fn(),
      onSelect: vi.fn(),
      onSwapPanes: vi.fn(),
      primaryTab: primary,
      secondaryTab: secondary,
      setDraggingTabId: vi.fn()
    }));

    expect(html).toContain('aria-current="true"');
  });

  it("closes secondary pane when close button is clicked", () => {
    const primary = createTab("Docs", "https://docs.example");
    const secondary = createTab("Notes", "https://notes.example");
    const onClose = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    // use act when React 19 style - just render with createRoot
    const { act } = require("react");
    act(() => {
      root.render(createElement(SplitTabRow, {
        activeTabId: primary.id,
        draggingTabId: null,
        onClose,
        onContextMenu: vi.fn(),
        onDrop: vi.fn(),
        onPreview: vi.fn(),
        onSelect: vi.fn(),
        onSwapPanes: vi.fn(),
        primaryTab: primary,
        secondaryTab: secondary,
        setDraggingTabId: vi.fn()
      }));
    });

    container.querySelector(".tab-close")?.dispatchEvent(new MouseEvent("click", { bubbles: true }));

    expect(onClose).toHaveBeenCalledWith(secondary.id);

    act(() => root.unmount());
  });

  it("swaps panes on Shift+Enter (split modifier activation)", () => {
    const primary = createTab("Docs", "https://docs.example");
    const secondary = createTab("Notes", "https://notes.example");
    const onSwapPanes = vi.fn();
    const onSelect = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);
    const { act } = require("react");

    act(() => {
      root.render(createElement(SplitTabRow, {
        activeTabId: "other-tab",
        draggingTabId: null,
        onClose: vi.fn(),
        onContextMenu: vi.fn(),
        onDrop: vi.fn(),
        onPreview: vi.fn(),
        onSelect,
        onSwapPanes,
        primaryTab: primary,
        secondaryTab: secondary,
        setDraggingTabId: vi.fn()
      }));
    });

    const button = container.querySelector(".split-tab-button");
    button?.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "Enter", shiftKey: true }));

    expect(onSwapPanes).toHaveBeenCalled();
    expect(onSelect).not.toHaveBeenCalled();

    act(() => root.unmount());
  });

  it("selects primary tab on Enter (primary activation)", () => {
    const primary = createTab("Docs", "https://docs.example");
    const secondary = createTab("Notes", "https://notes.example");
    const onSelect = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);
    const { act } = require("react");

    act(() => {
      root.render(createElement(SplitTabRow, {
        activeTabId: "other-tab",
        draggingTabId: null,
        onClose: vi.fn(),
        onContextMenu: vi.fn(),
        onDrop: vi.fn(),
        onPreview: vi.fn(),
        onSelect,
        onSwapPanes: vi.fn(),
        primaryTab: primary,
        secondaryTab: secondary,
        setDraggingTabId: vi.fn()
      }));
    });

    const button = container.querySelector(".split-tab-button");
    button?.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "Enter" }));

    expect(onSelect).toHaveBeenCalledWith(primary.id);

    act(() => root.unmount());
  });

  it("uses the whole split tab row as the native drag source (drains primary tab)", () => {
    const primary = createTab("Docs", "https://docs.example");
    const secondary = createTab("Notes", "https://notes.example");
    const setDraggingTabId = vi.fn();
    const data = new Map<string, string>();
    const container = document.createElement("div");
    const root = createRoot(container);
    const { act } = require("react");

    act(() => {
      root.render(createElement(SplitTabRow, {
        activeTabId: primary.id,
        draggingTabId: null,
        onClose: vi.fn(),
        onContextMenu: vi.fn(),
        onDrop: vi.fn(),
        onPreview: vi.fn(),
        onSelect: vi.fn(),
        onSwapPanes: vi.fn(),
        primaryTab: primary,
        secondaryTab: secondary,
        setDraggingTabId
      }));
    });

    const event = new Event("dragstart", { bubbles: true });
    const dataTransfer = {
      effectAllowed: "",
      setData: (type: string, value: string) => data.set(type, value)
    };
    Object.defineProperty(event, "dataTransfer", { value: dataTransfer });
    container.querySelector(".split-tab-row")?.dispatchEvent(event);

    expect(container.querySelector(".split-tab-row")?.getAttribute("draggable")).toBe("true");
    expect(setDraggingTabId).toHaveBeenCalledWith(primary.id);
    expect(dataTransfer.effectAllowed).toBe("move");

    act(() => root.unmount());
  });

  it("has split-tab-row CSS with accent tint and left inset bar", () => {
    expect(sidebarCss).toContain(".tab-row.split-tab-row");
    expect(sidebarCss).toContain("box-shadow: inset 2px 0 0 var(--accent-2)");
    expect(sidebarCss).toContain("split-tab-favicons");
    expect(sidebarCss).toContain("split-tab-favicon-primary");
    expect(sidebarCss).toContain("split-tab-favicon-secondary");
    expect(sidebarCss).toContain("split-tab-secondary-title");
  });

  it("has stacked favicon layout for split tab row", () => {
    expect(sidebarCss).toContain(".split-tab-favicon-primary");
    expect(sidebarCss).toContain(".split-tab-favicon-secondary");
    expect(sidebarCss).toContain("z-index: 2");
    expect(sidebarCss).toContain("z-index: 1");
  });
});
