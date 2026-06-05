import { useEffect, useRef } from "react";

import type { BrowserTab } from "../../../domain/browser";
import {
  getNavigationState,
  syncWebviewPreferences,
  type NavigationState
} from "../../../platform/webviewLifecycle";
import type { WebviewElement } from "../../../types/browser-ui";

export function BrowserWebview({
  isActive = false,
  isVisible,
  onLoadingChange,
  onFaviconChange,
  onMediaStateChange,
  onNavigate,
  onPermissionRequest,
  onTitleChange,
  onWebviewReady,
  onWebviewRemoved,
  partition,
  tab
}: {
  isActive?: boolean;
  isVisible: boolean;
  onLoadingChange: (isLoading: boolean, navigationState: NavigationState) => void;
  onFaviconChange: (faviconUrl: string | undefined) => void;
  onMediaStateChange: (mediaPlaying: boolean) => void;
  onNavigate: (url: string) => void;
  onPermissionRequest: (permission: "camera" | "media" | "microphone", active: boolean) => void;
  onTitleChange: (title: string, explicitSet: boolean) => void;
  onWebviewReady: (tabId: string, webview: WebviewElement) => void;
  onWebviewRemoved: (tabId: string, webview: WebviewElement) => void;
  partition: string;
  tab: BrowserTab;
}) {
  const ref = useRef<WebviewElement | null>(null);
  const readyRef = useRef(false);
  const latestRef = useRef({
    isActive,
    onLoadingChange,
    onFaviconChange,
    onMediaStateChange,
    onNavigate,
    onPermissionRequest,
    onTitleChange,
    tab
  });

  useEffect(() => {
    latestRef.current = { isActive, onLoadingChange, onFaviconChange, onMediaStateChange, onNavigate, onPermissionRequest, onTitleChange, tab };
  }, [isActive, onLoadingChange, onFaviconChange, onMediaStateChange, onNavigate, onPermissionRequest, onTitleChange, tab]);

  useEffect(() => {
    const webview = ref.current;
    if (!webview) return;

    webview.setAttribute("allowpopups", "true");
    const fallbackNavigationState = () => ({ canGoBack: false, canGoForward: false });
    const readNavigationState = () => readyRef.current ? getNavigationState(webview) : fallbackNavigationState();
    const onStart = () => latestRef.current.onLoadingChange(true, readNavigationState());
    const onStop = () => latestRef.current.onLoadingChange(false, readNavigationState());
    const onTitle = (event: Event) => {
      const detail = event as { title?: string; explicitSet?: boolean };
      const nextTitle = detail.title ?? latestRef.current.tab.title;
      latestRef.current.onTitleChange(nextTitle, Boolean(detail.explicitSet));
    };
    const onFavicon = (event: Event) => {
      const [faviconUrl] = (event as { favicons?: string[] }).favicons ?? [];
      latestRef.current.onFaviconChange(faviconUrl);
    };
    const onNav = (event: Event) => {
      latestRef.current.onNavigate((event as { url?: string }).url ?? latestRef.current.tab.url);
      latestRef.current.onLoadingChange(false, readNavigationState());
    };
    const onMediaStarted = () => latestRef.current.onMediaStateChange(true);
    const onMediaPaused = () => latestRef.current.onMediaStateChange(false);
    const handlePermissionRequest = (event: Event) => {
      const detail = event as { permission?: string };
      if (detail.permission === "media") {
        latestRef.current.onPermissionRequest("media", true);
      } else if (detail.permission === "videoCapture") {
        latestRef.current.onPermissionRequest("camera", true);
      } else if (detail.permission === "audioCapture") {
        latestRef.current.onPermissionRequest("microphone", true);
      }
    };
    const onDomReady = () => {
      readyRef.current = true;
      onWebviewReady(tab.id, webview);
      syncWebviewPreferences(webview, latestRef.current.tab);
      latestRef.current.onLoadingChange(false, readNavigationState());
    };

    webview.addEventListener("dom-ready", onDomReady);
    webview.addEventListener("did-start-loading", onStart);
    webview.addEventListener("did-stop-loading", onStop);
    webview.addEventListener("page-title-updated", onTitle);
    webview.addEventListener("page-favicon-updated", onFavicon);
    webview.addEventListener("did-navigate", onNav);
    webview.addEventListener("did-navigate-in-page", onNav);
    webview.addEventListener("media-started-playing", onMediaStarted);
    webview.addEventListener("media-paused", onMediaPaused);
    webview.addEventListener("permission-request", handlePermissionRequest);

    return () => {
      readyRef.current = false;
      onWebviewRemoved(tab.id, webview);
      webview.removeEventListener("dom-ready", onDomReady);
      webview.removeEventListener("did-start-loading", onStart);
      webview.removeEventListener("did-stop-loading", onStop);
      webview.removeEventListener("page-title-updated", onTitle);
      webview.removeEventListener("page-favicon-updated", onFavicon);
      webview.removeEventListener("did-navigate", onNav);
      webview.removeEventListener("did-navigate-in-page", onNav);
      webview.removeEventListener("media-started-playing", onMediaStarted);
      webview.removeEventListener("media-paused", onMediaPaused);
      webview.removeEventListener("permission-request", handlePermissionRequest);
    };
  }, [onWebviewReady, onWebviewRemoved, tab.id]);

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
