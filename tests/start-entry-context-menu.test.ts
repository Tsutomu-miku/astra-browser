import { createElement } from "react";
import { act } from "react";
import { createRoot } from "react-dom/client";
import { renderToStaticMarkup } from "react-dom/server";
import { readFileSync } from "node:fs";
import { join } from "node:path";
import { describe, expect, it, vi } from "vitest";

import { createFavorite, type HistoryEntry } from "../src/renderer/domain/browser";
import { StartEntryContextMenu } from "../src/renderer/surfaces/start/components/StartEntryContextMenu";
import { StartTileGrid } from "../src/renderer/surfaces/start/components/StartTileGrid";

const startContextMenuCss = readFileSync(join(__dirname, "../src/renderer/styles/start-context-menu.css"), "utf8");
const recentEntry: HistoryEntry = {
  id: "history_docs",
  title: "Docs",
  url: "https://docs.example",
  visitedAt: 1,
  workspaceId: "personal"
};

describe("start entry context menu", () => {
  it("renders Essential actions for open, preview, split, and removal", () => {
    const html = renderToStaticMarkup(createElement(StartEntryContextMenu, {
      item: createFavorite("Docs", "https://docs.example"),
      kind: "essential",
      left: 10,
      top: 20,
      onClose: vi.fn(),
      onOpen: vi.fn(),
      onOpenInSplit: vi.fn(),
      onPreview: vi.fn(),
      onRemove: vi.fn()
    }));

    expect(html).toContain('role="menu"');
    expect(html).toContain("Open");
    expect(html).toContain("Preview in Glance");
    expect(html).toContain("Open in split view");
    expect(html).toContain("Remove Essential");
  });

  it("renders Favorite removal copy", () => {
    const html = renderToStaticMarkup(createElement(StartEntryContextMenu, {
      item: createFavorite("Docs", "https://docs.example"),
      kind: "favorite",
      left: 10,
      top: 20,
      onClose: vi.fn(),
      onOpen: vi.fn(),
      onOpenInSplit: vi.fn(),
      onPreview: vi.fn(),
      onRemove: vi.fn()
    }));

    expect(html).toContain("Remove Favorite");
  });

  it("renders Recent history removal copy", () => {
    const html = renderToStaticMarkup(createElement(StartEntryContextMenu, {
      item: recentEntry,
      kind: "history",
      left: 10,
      top: 20,
      onClose: vi.fn(),
      onOpen: vi.fn(),
      onOpenInSplit: vi.fn(),
      onPreview: vi.fn(),
      onRemove: vi.fn()
    }));

    expect(html).toContain("Remove History");
  });

  it("opens with the full item identity", () => {
    const favorite = createFavorite("Docs", "https://docs.example", "tab-docs");
    const onOpen = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(StartEntryContextMenu, {
        item: favorite,
        kind: "favorite",
        left: 10,
        top: 20,
        onClose: vi.fn(),
        onOpen,
        onOpenInSplit: vi.fn(),
        onPreview: vi.fn(),
        onRemove: vi.fn()
      }));
    });

    container.querySelector("button")?.dispatchEvent(new MouseEvent("click", { bubbles: true }));

    expect(onOpen).toHaveBeenCalledWith(favorite, "favorite");

    act(() => root.unmount());
  });

  it("keeps tile action hints while supporting contextual management", () => {
    const html = renderToStaticMarkup(createElement(StartTileGrid, {
      emptyText: "Empty",
      items: [createFavorite("Docs", "https://docs.example")],
      kind: "favorite",
      onContextMenu: vi.fn(),
      onOpen: vi.fn()
    }));

    expect(html).toContain('class="start-tile"');
    expect(html).toContain('class="start-entry-action-hints"');
  });

  it("styles the start context menu surface", () => {
    expect(startContextMenuCss).toContain(".start-context-menu");
    expect(startContextMenuCss).toContain(".start-context-menu button.danger");
    expect(startContextMenuCss).toContain(".start-context-menu-separator");
  });
});
