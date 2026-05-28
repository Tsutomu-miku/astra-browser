import { createElement } from "react";
import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, it, vi } from "vitest";

import { GlanceHeader } from "../src/renderer/surfaces/glance/components/GlanceHeader";

describe("GlanceHeader", () => {
  it("turns the reload control into stop loading while the preview is loading", () => {
    const html = renderToStaticMarkup(createElement(GlanceHeader, {
      navigation: {
        canGoBack: false,
        canGoForward: false,
        isLoading: true
      },
      onClose: vi.fn(),
      onCopyUrl: vi.fn(),
      onGoBack: vi.fn(),
      onGoForward: vi.fn(),
      onOpen: vi.fn(),
      onRefresh: vi.fn(),
      onSplit: vi.fn(),
      title: "Example",
      url: "https://example.com"
    }));

    expect(html).toContain('aria-label="Stop loading"');
    expect(html).toContain('title="Stop loading"');
  });

  it("offers a direct copy action for the preview URL", () => {
    const html = renderToStaticMarkup(createElement(GlanceHeader, {
      navigation: {
        canGoBack: true,
        canGoForward: true,
        isLoading: false
      },
      onClose: vi.fn(),
      onCopyUrl: vi.fn(),
      onGoBack: vi.fn(),
      onGoForward: vi.fn(),
      onOpen: vi.fn(),
      onRefresh: vi.fn(),
      onSplit: vi.fn(),
      title: "Example",
      url: "https://example.com"
    }));

    expect(html).toContain('title="Copy preview URL"');
    expect(html).toContain("Open in split view");
  });
});
