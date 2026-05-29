import { createElement } from "react";
import { act } from "react";
import { createRoot } from "react-dom/client";
import { describe, expect, it, vi } from "vitest";

import { createTab } from "../src/renderer/domain/browser";
import { BrowserWebview } from "../src/renderer/surfaces/webview/components/BrowserWebview";

describe("browser webview", () => {
  it("reports Electron page favicons to tab state", () => {
    const container = document.createElement("div");
    document.body.append(container);
    const root = createRoot(container);
    const onFaviconChange = vi.fn();
    const tab = createTab("Docs", "https://docs.example");

    act(() => {
      root.render(createElement(BrowserWebview, {
        isVisible: true,
        onFaviconChange,
        onLoadingChange: vi.fn(),
        onNavigate: vi.fn(),
        onTitleChange: vi.fn(),
        onWebviewReady: vi.fn(),
        onWebviewRemoved: vi.fn(),
        partition: "persist:personal",
        tab
      }));
    });

    const webview = container.querySelector("webview")!;
    act(() => {
      webview.dispatchEvent(Object.assign(new Event("page-favicon-updated"), {
        favicons: ["https://docs.example/favicon.ico"]
      }));
    });

    expect(onFaviconChange).toHaveBeenCalledWith("https://docs.example/favicon.ico");

    act(() => root.unmount());
    container.remove();
  });
});
