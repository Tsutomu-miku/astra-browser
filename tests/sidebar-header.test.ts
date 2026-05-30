import { createElement } from "react";
import { readFileSync } from "node:fs";
import { join } from "node:path";
import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, it, vi } from "vitest";

import { SidebarHeader } from "../src/renderer/surfaces/sidebar/components/chrome/SidebarHeader";

const sidebarCss = readFileSync(join(__dirname, "../src/renderer/styles/sidebar.css"), "utf8");

describe("sidebar header", () => {
  it("labels the New tab control without a native title tooltip", () => {
    const html = renderToStaticMarkup(createElement(SidebarHeader, {
      onNewTab: vi.fn(),
      workspaceName: "Personal"
    }));

    expect(html).toContain('class="sidebar-heading"');
    expect(html).toContain('class="sidebar-eyebrow"');
    expect(html).toContain(">Space<");
    expect(html).toContain('class="sidebar-title"');
    expect(html).toContain('aria-label="New tab"');
    expect(html).not.toContain('title="New tab"');
  });

  it("keeps sidebar header chrome quiet and scoped", () => {
    const titleBlock = getRuleBlock(sidebarCss, ".sidebar-title");
    const eyebrowBlock = getRuleBlock(sidebarCss, ".sidebar-eyebrow");
    const newTabBlock = getRuleBlock(sidebarCss, ".sidebar-header .icon-button");
    const newTabPressedBlock = getRuleBlock(sidebarCss, ".sidebar-header .icon-button:active");

    expect(sidebarCss).not.toContain("\nh1 {");
    expect(titleBlock).toContain("font-size: 15px");
    expect(titleBlock).toContain("font-weight: 450");
    expect(titleBlock).toContain("max-width: 100%");
    expect(eyebrowBlock).toContain("text-transform: none");
    expect(newTabBlock).toContain("width: 28px");
    expect(newTabBlock).toContain("background: transparent");
    expect(newTabBlock).toContain("box-shadow: none");
    expect(newTabPressedBlock).toContain("transform: none");
  });
});

function getRuleBlock(css: string, selector: string): string {
  const start = css.indexOf(selector);
  expect(start).toBeGreaterThanOrEqual(0);
  const bodyStart = css.indexOf("{", start);
  const bodyEnd = css.indexOf("}", bodyStart);
  return css.slice(bodyStart + 1, bodyEnd);
}
