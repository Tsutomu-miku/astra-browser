import { createElement } from "react";
import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, it, vi } from "vitest";

import type { BrowserController } from "../src/renderer/app/controller/types";
import { SplitPaneOverlay } from "../src/renderer/surfaces/webview/components/SplitPaneOverlay";

describe("split pane overlay", () => {
  it("marks the active split layout button as pressed", () => {
    const markup = renderToStaticMarkup(
      createElement(SplitPaneOverlay, {
        controller: createController("grid"),
        isPrimary: true,
        tabId: "tab-1"
      })
    );

    expect(layoutButton(markup, "Horizontal split layout")).toContain('aria-pressed="false"');
    expect(layoutButton(markup, "Vertical split layout")).toContain('aria-pressed="false"');
    expect(layoutButton(markup, "Grid split layout")).toContain('aria-pressed="true"');
  });
});

function layoutButton(markup: string, label: string) {
  const match = markup.match(new RegExp(`<button[^>]*aria-label="${label}"[^>]*>`));
  if (!match) throw new Error(`Missing layout button: ${label}`);
  return match[0];
}

function createController(splitLayout: "grid" | "horizontal" | "vertical"): BrowserController {
  return {
    actions: {
      focusSplitPane: vi.fn(),
      removeTabFromSplit: vi.fn(),
      setSplitLayout: vi.fn(),
      toggleSplitMode: vi.fn()
    },
    splitLayout
  } as unknown as BrowserController;
}
