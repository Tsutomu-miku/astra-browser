import { createElement } from "react";
import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, it, vi } from "vitest";

import { SidebarSectionHeader } from "../src/renderer/surfaces/sidebar/components/tabs/SidebarItems";

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
});
