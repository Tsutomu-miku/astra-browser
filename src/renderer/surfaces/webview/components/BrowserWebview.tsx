import { useEffect, useRef } from "react";

import type { BrowserTab } from "../../../domain/browser";
import {
  getNavigationState,
  registerReadyWebview,
  syncWebviewPreferences,
  unregisterWebview,
  type NavigationState
} from "../../../platform/webviewLifecycle";
import type { WebviewElement } from "../../../types/browser-ui";

export function BrowserWebview({
  isVisible,
  onLoadingChange,
  onNavigate,
  onTitleChange,
  refMap,
  partition,
  tab
}: {
  isVisible: boolean;
  onLoadingChange: (isLoading: boolean, navigationState: NavigationState) => void;
  onNavigate: (url: string) => void;
  onTitleChange: (title: string) => void;
  partition: string;
  refMap: Map<string, WebviewElement>;
  tab: BrowserTab;
}) {
  const ref = useRef<WebviewElement | null>(null);
  const readyRef = useRef(false);
  const latestRef = useRef({
    onLoadingChange,
    onNavigate,
    onTitleChange,
    tab
  });

  useEffect(() => {
    latestRef.current = { onLoadingChange, onNavigate, onTitleChange, tab };
  }, [onLoadingChange, onNavigate, onTitleChange, tab]);

  useEffect(() => {
    const webview = ref.current;
    if (!webview) return;

    webview.setAttribute("allowpopups", "true");
    const fallbackNavigationState = () => ({ canGoBack: false, canGoForward: false });
    const readNavigationState = () => readyRef.current ? getNavigationState(webview) : fallbackNavigationState();
    const onStart = () => latestRef.current.onLoadingChange(true, readNavigationState());
    const onStop = () => latestRef.current.onLoadingChange(false, readNavigationState());
    const onTitle = (event: Event) => {
      latestRef.current.onTitleChange((event as { title?: string }).title ?? latestRef.current.tab.title);
    };
    const onNav = (event: Event) => {
      latestRef.current.onNavigate((event as { url?: string }).url ?? latestRef.current.tab.url);
      latestRef.current.onLoadingChange(false, readNavigationState());
    };
    const onDomReady = () => {
      readyRef.current = true;
      registerReadyWebview(refMap, tab.id, webview);
      syncWebviewPreferences(webview, latestRef.current.tab);
      latestRef.current.onLoadingChange(false, readNavigationState());
    };

    webview.addEventListener("dom-ready", onDomReady);
    webview.addEventListener("did-start-loading", onStart);
    webview.addEventListener("did-stop-loading", onStop);
    webview.addEventListener("page-title-updated", onTitle);
    webview.addEventListener("did-navigate", onNav);
    webview.addEventListener("did-navigate-in-page", onNav);

    return () => {
      readyRef.current = false;
      unregisterWebview(refMap, tab.id, webview);
      webview.removeEventListener("dom-ready", onDomReady);
      webview.removeEventListener("did-start-loading", onStart);
      webview.removeEventListener("did-stop-loading", onStop);
      webview.removeEventListener("page-title-updated", onTitle);
      webview.removeEventListener("did-navigate", onNav);
      webview.removeEventListener("did-navigate-in-page", onNav);
    };
  }, [refMap, tab.id]);

  useEffect(() => {
    const webview = ref.current;
    if (!webview || !readyRef.current) return;

    syncWebviewPreferences(webview, tab);
  }, [tab.isMuted, tab.zoomFactor]);

  return (
    <webview
      ref={ref}
      className={`browser-view ${isVisible ? "is-visible" : "is-hidden"}`}
      src={tab.url}
      partition={partition}
      aria-hidden={!isVisible}
    />
  );
}
