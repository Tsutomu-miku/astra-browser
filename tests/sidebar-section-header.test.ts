import { createElement } from "react";
import { readFileSync } from "node:fs";
import { join } from "node:path";
import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, it, vi } from "vitest";

import { SidebarSectionHeader } from "../src/renderer/surfaces/sidebar/components/tabs/SidebarItems";

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
    expect(html).toContain("Tabs");
    expect(html).toContain("4");
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

    expect(countBlock).not.toContain("background");
    expect(countBlock).not.toContain("border-radius");
    expect(countBlock).not.toContain("var(--accent)");
  });

  it("uses natural-cased quiet section labels", () => {
    const headerBlock = getRuleBlock(sidebarCss, ".sidebar-section-header");

    expect(headerBlock).toContain("font-weight: 500");
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
