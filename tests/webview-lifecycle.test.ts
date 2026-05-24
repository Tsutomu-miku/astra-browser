import { describe, expect, it, vi } from "vitest";

import {
  getNavigationState,
  registerReadyWebview,
  syncWebviewPreferences,
  unregisterWebview
} from "../src/renderer/platform/webviewLifecycle";
import type { WebviewElement } from "../src/renderer/types/browser-ui";

describe("webview lifecycle", () => {
  it("registers only ready webviews and unregisters the same instance", () => {
    const refMap = new Map<string, WebviewElement>();
    const webview = document.createElement("webview") as WebviewElement;
    const replacement = document.createElement("webview") as WebviewElement;

    registerReadyWebview(refMap, "tab-1", webview);
    expect(refMap.get("tab-1")).toBe(webview);

    registerReadyWebview(refMap, "tab-1", replacement);
    unregisterWebview(refMap, "tab-1", webview);
    expect(refMap.get("tab-1")).toBe(replacement);

    unregisterWebview(refMap, "tab-1", replacement);
    expect(refMap.has("tab-1")).toBe(false);
  });

  it("syncs preferences directly on ready webviews", () => {
    const webview = document.createElement("webview") as WebviewElement;
    webview.setAudioMuted = vi.fn();
    webview.setZoomFactor = vi.fn();

    syncWebviewPreferences(webview, { isMuted: true, zoomFactor: 1.25 });

    expect(webview.setZoomFactor).toHaveBeenCalledWith(1.25);
    expect(webview.setAudioMuted).toHaveBeenCalledWith(true);
  });

  it("does not hide Electron webview API errors", () => {
    const webview = document.createElement("webview") as WebviewElement;
    webview.setZoomFactor = () => {
      throw new Error("WebView must be attached to the DOM and the dom-ready event emitted");
    };

    expect(() => syncWebviewPreferences(webview, { isMuted: false, zoomFactor: 1 })).toThrow(/dom-ready/);
  });

  it("reads navigation state from the ready webview", () => {
    const webview = document.createElement("webview") as WebviewElement;
    webview.canGoBack = () => true;
    webview.canGoForward = () => false;

    expect(getNavigationState(webview)).toEqual({ canGoBack: true, canGoForward: false });
  });
});
