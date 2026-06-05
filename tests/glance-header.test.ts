import { createElement } from "react";
import { act } from "react";
import { createRoot } from "react-dom/client";
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

  it("offers copy, open, split, close, back, forward, and reload actions", () => {
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
    expect(html).toContain('title="Open in tab"');
    expect(html).toContain('title="Open in split view"');
    expect(html).toContain('title="Close Glance"');
    expect(html).toContain('title="Back"');
    expect(html).toContain('title="Forward"');
    expect(html).toContain('aria-label="Reload"');
    expect(html).toContain("Example");
    expect(html).toContain("https://example.com");
  });

  it("disables back and forward when there is no history", () => {
    const container = document.createElement("div");
    document.body.append(container);
    const root = createRoot(container);

    act(() => {
      root.render(createElement(GlanceHeader, {
        navigation: { canGoBack: false, canGoForward: false, isLoading: false },
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
    });

    const navButtons = container.querySelectorAll<HTMLButtonElement>(".glance-nav .icon-button");
    expect(navButtons[0].disabled).toBe(true); // Back
    expect(navButtons[1].disabled).toBe(true); // Forward
  });

  it("fires copy, open, split, close, back, forward, and reload callbacks", () => {
    const container = document.createElement("div");
    document.body.append(container);
    const root = createRoot(container);
    const onClose = vi.fn();
    const onCopyUrl = vi.fn();
    const onGoBack = vi.fn();
    const onGoForward = vi.fn();
    const onOpen = vi.fn();
    const onRefresh = vi.fn();
    const onSplit = vi.fn();

    act(() => {
      root.render(createElement(GlanceHeader, {
        navigation: { canGoBack: true, canGoForward: true, isLoading: false },
        onClose,
        onCopyUrl,
        onGoBack,
        onGoForward,
        onOpen,
        onRefresh,
        onSplit,
        title: "Example",
        url: "https://example.com"
      }));
    });

    const [backBtn, forwardBtn, reloadBtn] = container.querySelectorAll<HTMLButtonElement>(".glance-nav .icon-button");
    const [copyBtn, openBtn, splitBtn, closeBtn] = container.querySelectorAll<HTMLButtonElement>(".glance-actions .icon-button");

    act(() => backBtn.click());
    act(() => forwardBtn.click());
    act(() => reloadBtn.click());
    act(() => copyBtn.click());
    act(() => openBtn.click());
    act(() => splitBtn.click());
    act(() => closeBtn.click());

    expect(onGoBack).toHaveBeenCalled();
    expect(onGoForward).toHaveBeenCalled();
    expect(onRefresh).toHaveBeenCalled();
    expect(onCopyUrl).toHaveBeenCalled();
    expect(onOpen).toHaveBeenCalled();
    expect(onSplit).toHaveBeenCalled();
    expect(onClose).toHaveBeenCalled();
  });
});
