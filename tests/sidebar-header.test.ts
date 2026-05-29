import { createElement } from "react";
import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, it, vi } from "vitest";

import { SidebarHeader } from "../src/renderer/surfaces/sidebar/components/chrome/SidebarHeader";

describe("sidebar header", () => {
  it("labels the New tab control without a native title tooltip", () => {
    const html = renderToStaticMarkup(createElement(SidebarHeader, {
      onNewTab: vi.fn(),
      workspaceName: "Personal"
    }));

    expect(html).toContain('aria-label="New tab"');
    expect(html).not.toContain('title="New tab"');
  });
});
