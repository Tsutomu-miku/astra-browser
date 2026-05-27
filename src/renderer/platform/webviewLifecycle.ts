import type { BrowserTab } from "../domain/browser";
import type { WebviewElement } from "../types/browser-ui";

export interface NavigationState {
  canGoBack: boolean;
  canGoForward: boolean;
}

export function registerReadyWebview(refMap: Map<string, WebviewElement>, tabId: string, webview: WebviewElement) {
  refMap.set(tabId, webview);
}

export function unregisterWebview(refMap: Map<string, WebviewElement>, tabId: string, webview: WebviewElement) {
  if (refMap.get(tabId) === webview) {
    refMap.delete(tabId);
  }
}

export function getNavigationState(webview: WebviewElement): NavigationState {
  return {
    canGoBack: Boolean(webview.canGoBack?.()),
    canGoForward: Boolean(webview.canGoForward?.())
  };
}

export function syncWebviewPreferences(webview: WebviewElement, tab: Pick<BrowserTab, "isMuted" | "zoomFactor">) {
  webview.setZoomFactor?.(tab.zoomFactor);
  webview.setAudioMuted?.(tab.isMuted);
}
