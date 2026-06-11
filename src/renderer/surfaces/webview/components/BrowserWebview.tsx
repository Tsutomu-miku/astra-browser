import { useEffect, useRef } from "react";

import type { BrowserTab } from "../../../domain/browser";
import {
  getNavigationState,
  syncWebviewPreferences,
  type NavigationState
} from "../../../platform/webviewLifecycle";
import { useBrowserStore } from "../../../stores/browserStore";
import type { WebviewElement } from "../../../types/browser-ui";

export function BrowserWebview({
  isActive = false,
  isVisible,
  onLoadingChange,
  onFaviconChange,
  onMediaStateChange,
  onMuteChange,
  onNavigate,
  onPageContent,
  onPermissionRequest,
  onTitleChange,
  onWebviewReady,
  onWebviewRemoved,
  onZoomChange,
  partition,
  tab
}: {
  isActive?: boolean;
  isVisible: boolean;
  onLoadingChange: (isLoading: boolean, navigationState: NavigationState) => void;
  onFaviconChange: (faviconUrl: string | undefined) => void;
  onMediaStateChange: (mediaPlaying: boolean) => void;
  onMuteChange: (isMuted: boolean) => void;
  onNavigate: (url: string) => void;
  onPageContent?: (tabId: string, html: string) => void;
  onPermissionRequest: (permission: "camera" | "media" | "microphone", active: boolean) => void;
  onTitleChange: (title: string, explicitSet: boolean) => void;
  onWebviewReady: (tabId: string, webview: WebviewElement) => void;
  onWebviewRemoved: (tabId: string, webview: WebviewElement) => void;
  onZoomChange: (zoomFactor: number) => void;
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
    onMuteChange,
    onNavigate,
    onPageContent,
    onPermissionRequest,
    onTitleChange,
    onZoomChange,
    tab
  });

  useEffect(() => {
    latestRef.current = { isActive, onLoadingChange, onFaviconChange, onMediaStateChange, onMuteChange, onNavigate, onPageContent, onPermissionRequest, onTitleChange, onZoomChange, tab };
  }, [isActive, onLoadingChange, onFaviconChange, onMediaStateChange, onMuteChange, onNavigate, onPageContent, onPermissionRequest, onTitleChange, onZoomChange, tab]);

  useEffect(() => {
    const webview = ref.current;
    if (!webview) return;

    webview.setAttribute("allowpopups", "true");
    const fallbackNavigationState = () => ({ canGoBack: false, canGoForward: false });
    const readNavigationState = () => readyRef.current ? getNavigationState(webview) : fallbackNavigationState();
    const onStart = () => latestRef.current.onLoadingChange(true, readNavigationState());
    const onStop = async () => {
      latestRef.current.onLoadingChange(false, readNavigationState());
      if (
        readyRef.current &&
        latestRef.current.onPageContent &&
        typeof webview.executeJavaScript === "function" &&
        latestRef.current.tab.url &&
        !latestRef.current.tab.url.startsWith("data:")
      ) {
        try {
          const html = await webview.executeJavaScript("document.documentElement.outerHTML");
          if (typeof html === "string") {
            latestRef.current.onPageContent(latestRef.current.tab.id, html);
          }
        } catch {
          /* ignore OOP-framed content we cannot introspect */
        }
      }
    };
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
    const onZoomChanged = async () => {
      // The zoom-changed event carries { newZoomLevel }, but state stores a
      // zoomFactor. Read it back from the webview so we always round-trip
      // through the same conversion Electron uses internally.
      if (typeof webview.getZoomFactor !== "function") return;
      try {
        const factor = await webview.getZoomFactor();
        if (Number.isFinite(factor) && readyRef.current) {
          latestRef.current.onZoomChange(factor);
        }
      } catch {
        // ignore — webview may have been detached
      }
    };
    const onAudioStateChanged = (event: Event) => {
      const detail = event as { audioMuted?: boolean };
      if (typeof detail.audioMuted === "boolean") {
        latestRef.current.onMuteChange(detail.audioMuted);
      }
    };
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
    // Renderer-side fallback for popups opened from inside a <webview>.
    // Electron's main-process setWindowOpenHandler is authoritative for most
    // popup kinds but the legacy DOM-level `new-window` event still fires for
    // some <webview>-internal cases (notably <a target="_blank"> plain clicks
    // on some Electron versions). Both paths converge on the same CustomEvent
    // that useBrowserEffects already listens for.
    const onNewWindow = (event: Event) => {
      const targetUrl = (event as { url?: string }).url;
      if (!targetUrl || targetUrl.startsWith("astra://")) return;
      event.preventDefault();
      window.dispatchEvent(new CustomEvent("astra:open-url-in-new-tab", { detail: targetUrl }));
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
    webview.addEventListener("zoom-changed", onZoomChanged);
    webview.addEventListener("audio-state-changed", onAudioStateChanged);
    webview.addEventListener("new-window", onNewWindow);
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
      webview.removeEventListener("zoom-changed", onZoomChanged);
      webview.removeEventListener("audio-state-changed", onAudioStateChanged);
      webview.removeEventListener("new-window", onNewWindow);
      webview.removeEventListener("permission-request", handlePermissionRequest);
    };
  }, [onWebviewReady, onWebviewRemoved, tab.id]);

  useEffect(() => {
    const webview = ref.current;
    if (!webview || !readyRef.current) return;

    syncWebviewPreferences(webview, tab);
  }, [tab.isMuted, tab.zoomFactor]);

  const autofillBridgePath = useBrowserStore((s) => s.autofillBridgePath);

  return (
    <webview
      ref={ref}
      allowpopups
      className={`browser-view ${isVisible ? "is-visible" : "is-hidden"}`}
      src={tab.url}
      partition={partition}
      aria-hidden={!isVisible}
      preload={autofillBridgePath || undefined}
    />
  );
}
