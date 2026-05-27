import { useEffect, useRef, useState } from "react";

import { getReadableUrlTitle, getWorkspacePartition } from "../../domain/browser-core";
import type { BrowserController } from "../../app/controller/types";
import type { WebviewElement } from "../../types/browser-ui";
import { GlanceHeader, type GlanceNavigationState } from "./components/GlanceHeader";
import { GlanceWebview } from "./components/GlanceWebview";
import { getGlanceNavigationState } from "./glanceNavigation";

export function GlancePanel({ controller }: { controller: BrowserController }) {
  const { actions, activeWorkspace, glance } = controller;
  const webviewRef = useRef<WebviewElement | null>(null);
  const [previewUrl, setPreviewUrl] = useState(glance?.url ?? "");
  const [previewTitle, setPreviewTitle] = useState(glance?.title ?? "");
  const [navigation, setNavigation] = useState<GlanceNavigationState>({
    canGoBack: false,
    canGoForward: false,
    isLoading: false
  });

  useEffect(() => {
    if (!glance) return;

    setPreviewUrl(glance.url);
    setPreviewTitle(glance.title || getReadableUrlTitle(glance.url));
    setNavigation({ canGoBack: false, canGoForward: false, isLoading: false });
  }, [glance]);

  useEffect(() => {
    const closeOnEscape = (event: KeyboardEvent) => {
      if (event.key === "Escape") actions.closeGlance();
    };

    window.addEventListener("keydown", closeOnEscape);
    return () => window.removeEventListener("keydown", closeOnEscape);
  }, [actions]);

  useEffect(() => {
    const webview = webviewRef.current;
    if (!webview) return;

    const updateNavigation = (patch: Partial<GlanceNavigationState> = {}) => {
      setNavigation((current) => ({
        ...current,
        ...getGlanceNavigationState(webview),
        ...patch
      }));
    };
    const onNavigate = (event: Event) => {
      const url = (event as { url?: string }).url;
      if (!url) return;
      setPreviewUrl(url);
      setPreviewTitle((current) => current || getReadableUrlTitle(url));
      updateNavigation({ isLoading: false });
    };
    const onTitle = (event: Event) => {
      const title = (event as { title?: string }).title;
      if (title) setPreviewTitle(title);
    };
    const onDomReady = () => updateNavigation();
    const onStart = () => updateNavigation({ isLoading: true });
    const onStop = () => updateNavigation({ isLoading: false });

    webview.addEventListener("dom-ready", onDomReady);
    webview.addEventListener("did-start-loading", onStart);
    webview.addEventListener("did-stop-loading", onStop);
    webview.addEventListener("did-navigate", onNavigate);
    webview.addEventListener("did-navigate-in-page", onNavigate);
    webview.addEventListener("page-title-updated", onTitle);
    return () => {
      webview.removeEventListener("dom-ready", onDomReady);
      webview.removeEventListener("did-start-loading", onStart);
      webview.removeEventListener("did-stop-loading", onStop);
      webview.removeEventListener("did-navigate", onNavigate);
      webview.removeEventListener("did-navigate-in-page", onNavigate);
      webview.removeEventListener("page-title-updated", onTitle);
    };
  }, [glance?.url]);

  if (!glance) return null;

  const title = previewTitle || getReadableUrlTitle(previewUrl);
  const closeGlance = actions.closeGlance;

  return (
    <section
      className="glance-backdrop"
      aria-label="Glance preview"
      onClick={(event) => {
        if (event.target === event.currentTarget) actions.closeGlance();
      }}
    >
      <article className="glance-panel" role="dialog" aria-modal="true" aria-label={title}>
        <GlanceHeader
          navigation={navigation}
          title={title}
          url={previewUrl}
          onClose={closeGlance}
          onGoBack={() => webviewRef.current?.goBack?.()}
          onGoForward={() => webviewRef.current?.goForward?.()}
          onOpen={() => {
            actions.navigateActiveTab(previewUrl);
            closeGlance();
          }}
          onRefresh={() => webviewRef.current?.reload?.()}
          onSplit={() => {
            actions.openUrlInSplit(previewUrl, title);
            closeGlance();
          }}
        />
        <GlanceWebview
          key={glance.url}
          ref={webviewRef}
          url={glance.url}
          partition={getWorkspacePartition(activeWorkspace)}
        />
      </article>
    </section>
  );
}
