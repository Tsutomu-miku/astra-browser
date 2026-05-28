import { readFileSync } from "node:fs";
import { join } from "node:path";
import { createElement } from "react";
import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, it, vi } from "vitest";

import type { ClosedTab } from "../src/renderer/domain/browser";
import { ClosedTabButton } from "../src/renderer/surfaces/sidebar/components/tabs/SidebarItems";

const sidebarCss = readFileSync(join(__dirname, "../src/renderer/styles/sidebar.css"), "utf8");

describe("sidebar recently closed tabs", () => {
  it("renders a compact restore action", () => {
    const html = renderToStaticMarkup(createElement(ClosedTabButton, {
      closedIndex: 2,
      onRestore: vi.fn(),
      tab: closedTab()
    }));

    expect(html).toContain('class="closed-tab-button"');
    expect(html).toContain("Docs");
    expect(html).toContain("https://docs.example/");
    expect(html).toContain("Restore");
    expect(html).toContain('title="Restore Docs"');
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
