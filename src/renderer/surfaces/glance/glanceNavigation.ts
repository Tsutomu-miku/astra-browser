import type { WebviewElement } from "../../types/browser-ui";

export function getGlanceNavigationState(webview: WebviewElement | null): {
  canGoBack: boolean;
  canGoForward: boolean;
} {
  return {
    canGoBack: Boolean(webview?.canGoBack?.()),
    canGoForward: Boolean(webview?.canGoForward?.())
  };
}
