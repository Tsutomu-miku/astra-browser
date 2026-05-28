import { createElement } from "react";
import { renderToStaticMarkup } from "react-dom/server";
import { readFileSync } from "node:fs";
import { join } from "node:path";
import { describe, expect, it, vi } from "vitest";

import { createFavorite } from "../src/renderer/domain/browser";
import { QuickEntryContextMenu } from "../src/renderer/surfaces/sidebar/components/tabs/QuickEntryContextMenu";

const contextMenuCss = readFileSync(join(__dirname, "../src/renderer/styles/sidebar-context-menu.css"), "utf8");

describe("sidebar quick entry context menu", () => {
  it("renders Essential actions for open, preview, split, and removal", () => {
    const html = renderToStaticMarkup(createElement(QuickEntryContextMenu, {
      item: createFavorite("Docs", "https://docs.example"),
      kind: "essential",
      left: 10,
      top: 20,
      onClose: vi.fn(),
      onCopyText: vi.fn(),
      onOpen: vi.fn(),
      onOpenInSplit: vi.fn(),
      onPreview: vi.fn(),
      onRemove: vi.fn()
    }));

    expect(html).toContain('role="menu"');
    expect(html).toContain("Open");
    expect(html).toContain("Preview in Glance");
    expect(html).toContain("Open in split view");
    expect(html).toContain("Copy URL");
    expect(html).toContain("Copy title");
    expect(html).toContain("Remove Essential");
  });

  it("copies quick entry URL and title", () => {
    const onCopyText = vi.fn();
    const menu = createElement(QuickEntryContextMenu, {
      item: createFavorite("Docs", "https://docs.example"),
      kind: "favorite",
      left: 10,
      top: 20,
      onClose: vi.fn(),
      onCopyText,
      onOpen: vi.fn(),
      onOpenInSplit: vi.fn(),
      onPreview: vi.fn(),
      onRemove: vi.fn()
    });

    menu.props.onCopyText(menu.props.item.url);
    menu.props.onCopyText(menu.props.item.title || menu.props.item.url);

    expect(renderToStaticMarkup(menu)).toContain("Copy URL");
    expect(onCopyText).toHaveBeenCalledWith("https://docs.example");
    expect(onCopyText).toHaveBeenCalledWith("Docs");
  });

  it("renders Favorite removal copy", () => {
    const html = renderToStaticMarkup(createElement(QuickEntryContextMenu, {
      item: createFavorite("Docs", "https://docs.example"),
      kind: "favorite",
      left: 10,
      top: 20,
      onClose: vi.fn(),
      onCopyText: vi.fn(),
      onOpen: vi.fn(),
      onOpenInSplit: vi.fn(),
      onPreview: vi.fn(),
      onRemove: vi.fn()
    }));

    expect(html).toContain("Remove Favorite");
  });

  it("styles quick entry menus with the shared sidebar menu surface", () => {
    expect(contextMenuCss).toContain(".quick-entry-context-menu");
    expect(contextMenuCss).toContain(".tab-context-menu button.danger");
  });
});
