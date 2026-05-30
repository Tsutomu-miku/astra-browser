import { createElement } from "react";
import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, it } from "vitest";

import { BrowserItemIcon } from "../src/renderer/common/icons/BrowserItemIcon";
import { getBrowserItemIconState } from "../src/renderer/common/icons/browserItemIconState";
import { SidebarItemIcon } from "../src/renderer/surfaces/sidebar/components/common/SidebarItemIcon";

describe("sidebar item icon", () => {
  it("uses web host initials for normal pages", () => {
    expect(getBrowserItemIconState("https://developer.mozilla.org/docs")).toEqual({
      kind: "web",
      label: "developer.mozilla.org",
      text: "D"
    });
  });

  it("uses symbolic icons for internal and file pages", () => {
    expect(getBrowserItemIconState("astra://newtab")).toMatchObject({
      kind: "internal",
      text: null
    });
    expect(getBrowserItemIconState("file:///Users/test/report.pdf")).toMatchObject({
      kind: "file",
      text: null
    });
  });

  it("renders loading and sleeping status as icon glyphs", () => {
    const loadingHtml = renderToStaticMarkup(createElement(SidebarItemIcon, {
      className: "tab-favicon",
      status: "loading",
      url: "https://docs.example"
    }));
    const sleepingHtml = renderToStaticMarkup(createElement(SidebarItemIcon, {
      className: "favorite-icon",
      status: "sleeping",
      url: "https://docs.example"
    }));

    expect(loadingHtml).toContain('data-icon-kind="web"');
    expect(loadingHtml).toContain('data-icon-status="loading"');
    expect(loadingHtml).toContain("sidebar-item-icon-status is-loading");
    expect(sleepingHtml).toContain('data-icon-status="sleeping"');
    expect(sleepingHtml).toContain("sidebar-item-icon-status is-sleeping");
  });

  it("renders a site favicon before falling back to generated icons", () => {
    const html = renderToStaticMarkup(createElement(SidebarItemIcon, {
      className: "tab-favicon",
      faviconUrl: "https://docs.example/favicon.ico",
      url: "https://docs.example"
    }));

    expect(html).toContain("sidebar-item-icon-image");
    expect(html).toContain('src="https://docs.example/favicon.ico"');
    expect(html).toContain('data-icon-kind="web"');
  });

  it("uses cached site favicons when the item has no own favicon", () => {
    const html = renderToStaticMarkup(createElement(BrowserItemIcon, {
      className: "favorite-icon",
      faviconCache: {
        "https://docs.example": "https://docs.example/cached.ico"
      },
      url: "https://docs.example/deep/page"
    }));

    expect(html).toContain("sidebar-item-icon-image");
    expect(html).toContain('src="https://docs.example/cached.ico"');
  });
});
