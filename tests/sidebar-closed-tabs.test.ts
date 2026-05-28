import { readFileSync } from "node:fs";
import { join } from "node:path";
import { createElement } from "react";
import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, it, vi } from "vitest";

import type { ClosedTab } from "../src/renderer/domain/browser";
import { ClosedTabButton } from "../src/renderer/surfaces/sidebar/components/tabs/ClosedTabButton";
import { ClosedTabContextMenu } from "../src/renderer/surfaces/sidebar/components/tabs/ClosedTabContextMenu";

const sidebarCss = readFileSync(join(__dirname, "../src/renderer/styles/sidebar.css"), "utf8");

describe("sidebar recently closed tabs", () => {
  it("renders a compact restore action", () => {
    const html = renderToStaticMarkup(createElement(ClosedTabButton, {
      closedIndex: 2,
      onContextMenu: vi.fn(),
      onOpenInSplit: vi.fn(),
      onPreview: vi.fn(),
      onRestore: vi.fn(),
      tab: closedTab()
    }));

    expect(html).toContain('class="closed-tab-button"');
    expect(html).toContain("Docs");
    expect(html).toContain("https://docs.example/");
    expect(html).toContain("Restore");
    expect(html).toContain("Alt");
    expect(html).toContain("Preview");
    expect(html).toContain("Shift");
    expect(html).toContain("Split");
    expect(html).toContain('title="Restore Docs"');
  });

  it("renders a recently closed context menu with restore, split, preview, and copy actions", () => {
    const html = renderToStaticMarkup(createElement(ClosedTabContextMenu, {
      closedIndex: 1,
      left: 10,
      onClose: vi.fn(),
      onCopyText: vi.fn(),
      onOpenInSplit: vi.fn(),
      onPreview: vi.fn(),
      onRestore: vi.fn(),
      tab: closedTab(),
      top: 20
    }));

    expect(html).toContain('role="menu"');
    expect(html).toContain("Restore");
    expect(html).toContain("Preview in Glance");
    expect(html).toContain("Open in split view");
    expect(html).toContain("Copy URL");
    expect(html).toContain("Copy title");
  });

  it("copies recently closed URL and title", () => {
    const onCopyText = vi.fn();
    const menu = createElement(ClosedTabContextMenu, {
      closedIndex: 1,
      left: 10,
      onClose: vi.fn(),
      onCopyText,
      onOpenInSplit: vi.fn(),
      onPreview: vi.fn(),
      onRestore: vi.fn(),
      tab: closedTab(),
      top: 20
    });

    menu.props.onCopyText(menu.props.tab.url);
    menu.props.onCopyText(menu.props.tab.title || menu.props.tab.url);

    expect(renderToStaticMarkup(menu)).toContain("Copy URL");
    expect(onCopyText).toHaveBeenCalledWith("https://docs.example/");
    expect(onCopyText).toHaveBeenCalledWith("Docs");
  });

  it("styles the recently closed sidebar section", () => {
    expect(sidebarCss).toContain(".recently-closed-tabs");
    expect(sidebarCss).toContain(".closed-tab-button");
    expect(sidebarCss).toContain(".closed-tab-action");
  });
});

function closedTab(): ClosedTab {
  return {
    closedAt: 1,
    title: "Docs",
    url: "https://docs.example/"
  };
}
