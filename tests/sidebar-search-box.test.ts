import { createElement } from "react";
import { readFileSync } from "node:fs";
import { join } from "node:path";
import { act } from "react";
import { createRoot } from "react-dom/client";
import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, it, vi } from "vitest";

import { SidebarSearchBox } from "../src/renderer/surfaces/sidebar/components/chrome/SidebarSearchBox";

const sidebarSearchCss = readFileSync(join(__dirname, "../src/renderer/styles/sidebar-search.css"), "utf8");
const sidebarCss = readFileSync(join(__dirname, "../src/renderer/styles/sidebar.css"), "utf8");

describe("sidebar search box", () => {
  it("renders an explicitly labelled clear control while searching", () => {
    const html = renderToStaticMarkup(createElement(SidebarSearchBox, {
      query: "docs",
      onClear: vi.fn(),
      onKeyDown: vi.fn(),
      onQueryChange: vi.fn()
    }));

    expect(html).toContain('aria-label="Clear sidebar search"');
    expect(html).not.toContain('title="Clear sidebar search"');
  });

  it("clears search without dropping keyboard focus", () => {
    const onClear = vi.fn();
    const container = document.createElement("div");
    document.body.append(container);
    const root = createRoot(container);

    act(() => {
      root.render(createElement(SidebarSearchBox, {
        query: "docs",
        onClear,
        onKeyDown: vi.fn(),
        onQueryChange: vi.fn()
      }));
    });

    const input = container.querySelector<HTMLInputElement>("input")!;
    input.focus();

    act(() => {
      container.querySelector<HTMLButtonElement>(".icon-button")?.dispatchEvent(new MouseEvent("click", {
        bubbles: true
      }));
    });

    expect(onClear).toHaveBeenCalled();
    expect(document.activeElement).toBe(input);

    act(() => root.unmount());
    container.remove();
  });

  it("wires Escape to the parent keyboard handler", () => {
    const onKeyDown = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(SidebarSearchBox, {
        query: "docs",
        onClear: vi.fn(),
        onKeyDown,
        onQueryChange: vi.fn()
      }));
    });

    container.querySelector("input")?.dispatchEvent(new KeyboardEvent("keydown", {
      bubbles: true,
      key: "Escape"
    }));

    expect(onKeyDown).toHaveBeenCalledWith(expect.objectContaining({ key: "Escape" }));

    act(() => root.unmount());
  });

  it("renders search action hints as icon-only glyphs", () => {
    const html = renderToStaticMarkup(createElement(SidebarSearchBox, {
      activeSearchTarget: {
        id: "docs",
        title: "Docs",
        type: "tab",
        url: "https://docs.example"
      },
      query: "docs",
      resultCount: 3,
      onClear: vi.fn(),
      onKeyDown: vi.fn(),
      onQueryChange: vi.fn()
    }));

    expect(html).toContain('class="sidebar-search-meta"');
    expect(html).toContain('class="sidebar-search-status"');
    expect(html).toContain(">3 results<");
    expect(html).toContain('class="sidebar-search-action-hints"');
    expect(html).toContain('aria-label="Alt Preview, Shift Split"');
    expect(html).toContain('data-action-hint="preview"');
    expect(html).toContain('data-action-hint="split"');
    expect(html).not.toContain("<kbd");
  });

  it("renders a quiet no-match search status", () => {
    const html = renderToStaticMarkup(createElement(SidebarSearchBox, {
      query: "missing",
      resultCount: 0,
      onClear: vi.fn(),
      onKeyDown: vi.fn(),
      onQueryChange: vi.fn()
    }));

    expect(html).toContain('role="status"');
    expect(html).toContain('aria-live="polite"');
    expect(html).toContain(">No matches<");
    expect(html).not.toContain('class="sidebar-search-action-hints"');
  });

  it("keeps sidebar search and compact address focus states quiet", () => {
    const searchFocusBlock = getRuleBlock(sidebarSearchCss, ".sidebar-search input:focus");
    const addressFocusBlock = getRuleBlock(sidebarCss, ".sidebar-address-form:focus-within");
    const metaBlock = getRuleBlock(sidebarSearchCss, "\n.sidebar-search-meta {");
    const hintBlock = getRuleBlock(sidebarSearchCss, "\n.sidebar-search-action-hint {");

    expect(searchFocusBlock).toContain("border-color: transparent");
    expect(searchFocusBlock).toContain("box-shadow: none");
    expect(searchFocusBlock).not.toContain("var(--accent)");
    expect(addressFocusBlock).toContain("border-color: transparent");
    expect(addressFocusBlock).toContain("box-shadow: none");
    expect(addressFocusBlock).not.toContain("var(--accent)");
    expect(metaBlock).toContain("justify-content: space-between");
    expect(metaBlock).not.toContain("var(--accent)");
    expect(hintBlock).toContain("width: 16px");
    expect(sidebarSearchCss).not.toContain("kbd");
  });
});

function getRuleBlock(css: string, selector: string): string {
  const start = css.indexOf(selector);
  expect(start).toBeGreaterThanOrEqual(0);
  const bodyStart = css.indexOf("{", start);
  const bodyEnd = css.indexOf("}", bodyStart);
  return css.slice(bodyStart + 1, bodyEnd);
}
