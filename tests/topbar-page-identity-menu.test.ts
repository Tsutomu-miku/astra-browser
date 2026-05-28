import { createElement } from "react";
import { renderToStaticMarkup } from "react-dom/server";
import { readFileSync } from "node:fs";
import { join } from "node:path";
import { describe, expect, it, vi } from "vitest";

import { PageIdentityContextMenu } from "../src/renderer/surfaces/topbar/components/PageIdentityContextMenu";

const topbarCss = readFileSync(join(__dirname, "../src/renderer/styles/topbar.css"), "utf8");

describe("page identity context menu", () => {
  it("renders page handling actions from the address identity", () => {
    const html = renderToStaticMarkup(createElement(PageIdentityContextMenu, {
      item: {
        title: "Example",
        url: "https://example.com"
      },
      left: 10,
      top: 20,
      onClose: vi.fn(),
      onCopyTitle: vi.fn(),
      onCopyUrl: vi.fn(),
      onOpenGlance: vi.fn(),
      onOpenInSplit: vi.fn(),
      onOpenSiteInfo: vi.fn()
    }));

    expect(html).toContain('role="menu"');
    expect(html).toContain("Site information");
    expect(html).toContain("Copy current URL");
    expect(html).toContain("Copy page title");
    expect(html).toContain("Preview in Glance");
    expect(html).toContain("Open in split view");
  });

  it("styles the page identity context menu", () => {
    expect(topbarCss).toContain(".page-identity-context-menu");
    expect(topbarCss).toContain(".page-identity-context-menu button:hover");
    expect(topbarCss).toContain(".page-identity-context-menu-separator");
  });
});
