import { createElement } from "react";
import { readFileSync } from "node:fs";
import { join } from "node:path";
import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, it, vi } from "vitest";

import { SidebarSectionHeader } from "../src/renderer/surfaces/sidebar/components/common/SidebarSectionHeader";

const sidebarCss = readFileSync(join(__dirname, "../src/renderer/styles/sidebar.css"), "utf8");

describe("sidebar section header", () => {
  it("renders collapsible section state", () => {
    const html = renderToStaticMarkup(createElement(SidebarSectionHeader, {
      count: 4,
      isCollapsed: false,
      onToggle: vi.fn(),
      title: "Tabs"
    }));

    expect(html).toContain("<button");
    expect(html).toContain('aria-expanded="true"');
    expect(html).toContain('data-collapsed="false"');
    expect(html).toContain("Tabs");
    expect(html).toContain("4");
  });

  it("marks collapsed section headers so counts stay available when contents are hidden", () => {
    const html = renderToStaticMarkup(createElement(SidebarSectionHeader, {
      count: 7,
      isCollapsed: true,
      onToggle: vi.fn(),
      title: "Favorites"
    }));

    expect(html).toContain('aria-expanded="false"');
    expect(html).toContain('data-collapsed="true"');
    expect(html).toContain("7");
  });

  it("keeps static section headers button-free", () => {
    const html = renderToStaticMarkup(createElement(SidebarSectionHeader, {
      count: 2,
      title: "Favorites"
    }));

    expect(html).not.toContain("<button");
    expect(html).toContain("Favorites");
  });

  it("keeps section counts as quiet text instead of badges", () => {
    const countBlock = getRuleBlock(sidebarCss, ".sidebar-section-count");
    const dropLabelBlock = getRuleBlock(sidebarCss, "\n.sidebar-section-drop-label {");

    expect(countBlock).not.toContain("background");
    expect(countBlock).not.toContain("border-radius");
    expect(countBlock).not.toContain("var(--accent)");
    expect(dropLabelBlock).toContain("border: 0");
    expect(dropLabelBlock).toContain("background: transparent");
    expect(dropLabelBlock).not.toContain("border-radius");
    expect(dropLabelBlock).not.toContain("var(--accent)");
  });

  it("keeps expanded section counts quiet until the header is engaged", () => {
    const countBlock = getRuleBlock(sidebarCss, "\n.sidebar-section-count {\n  color:");
    const revealBlock = getRuleBlock(sidebarCss, ".sidebar-section-header:hover .sidebar-section-count,\n.sidebar-section-header:focus-within .sidebar-section-count,\n.sidebar-section-header[data-collapsed=\"true\"] .sidebar-section-count");

    expect(countBlock).toContain("opacity: 0");
    expect(countBlock).toContain("transition: opacity 120ms ease");
    expect(countBlock).not.toContain("transform");
    expect(revealBlock).toContain("opacity: 1");
    expect(revealBlock).not.toContain("transform");
  });

  it("uses natural-cased quiet section labels", () => {
    const headerBlock = getRuleBlock(sidebarCss, ".sidebar-section-header");

    expect(headerBlock).toContain("font-weight: 450");
    expect(headerBlock).toContain("text-transform: none");
    expect(headerBlock).not.toContain("uppercase");
  });
});

function getRuleBlock(css: string, selector: string): string {
  const start = css.indexOf(selector);
  expect(start).toBeGreaterThanOrEqual(0);
  const bodyStart = css.indexOf("{", start);
  const bodyEnd = css.indexOf("}", bodyStart);
  return css.slice(bodyStart + 1, bodyEnd);
}
