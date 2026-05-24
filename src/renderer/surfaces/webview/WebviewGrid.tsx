import { useEffect, useRef } from "react";

import { getWorkspacePartition, type BrowserTab } from "../../domain/browser-core";
import type { BrowserController } from "../../hooks/types";
import type { WebviewElement } from "../../types/browser-ui";
import { getKeepAliveWebviewTabs } from "./webviewLayout";

export function WebviewGrid({ controller }: { controller: BrowserController }) {
  const { activeTab, activeWorkspace, actions, state, webviews } = controller;
  const layoutTabs = getKeepAliveWebviewTabs(activeWorkspace, activeTab, state);
  const visibleCount = layoutTabs.filter((entry) => entry.isVisible).length;
  const partition = getWorkspacePartition(activeWorkspace);

  return (
    <section className={`view-grid ${visibleCount === 2 ? "is-split" : ""}`} aria-label="Browser content">
      {layoutTabs.map(({ isVisible, tab }) => (
        <BrowserWebview
          key={tab.id}
          isVisible={isVisible}
          partition={partition}
          tab={tab}
          refMap={webviews.current}
          onLoadingChange={(isLoading, navigationState) => actions.updateTab(tab.id, { isLoading, ...navigationState })}
          onTitleChange={(title) => actions.updateTab(tab.id, { title })}
          onNavigate={(url) => {
            actions.updateTab(tab.id, { url });
            actions.recordHistory(tab.id, url);
          }}
        />
      ))}
    </section>
  );
}

function BrowserWebview({
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

  useEffect(() => {
    const webview = ref.current;
    if (!webview) return;

    refMap.set(tab.id, webview);
    webview.setZoomFactor?.(tab.zoomFactor);
    webview.setAudioMuted?.(tab.isMuted);
    const getNavigationState = () => ({
      canGoBack: Boolean(webview.canGoBack?.()),
      canGoForward: Boolean(webview.canGoForward?.())
    });
    const onStart = () => onLoadingChange(true, getNavigationState());
    const onStop = () => onLoadingChange(false, getNavigationState());
    const onTitle = (event: Event) => onTitleChange((event as { title?: string }).title ?? tab.title);
    const onNav = (event: Event) => {
      onNavigate((event as { url?: string }).url ?? tab.url);
      onLoadingChange(false, getNavigationState());
    };

    webview.addEventListener("did-start-loading", onStart);
    webview.addEventListener("did-stop-loading", onStop);
    webview.addEventListener("page-title-updated", onTitle);
    webview.addEventListener("did-navigate", onNav);
    webview.addEventListener("did-navigate-in-page", onNav);

    return () => {
      webview.removeEventListener("did-start-loading", onStart);
      webview.removeEventListener("did-stop-loading", onStop);
      webview.removeEventListener("page-title-updated", onTitle);
      webview.removeEventListener("did-navigate", onNav);
      webview.removeEventListener("did-navigate-in-page", onNav);
      refMap.delete(tab.id);
    };
  }, [onLoadingChange, onNavigate, onTitleChange, refMap, tab.id, tab.isMuted, tab.title, tab.url, tab.zoomFactor]);

  return (
    <webview
      ref={ref}
      className={`browser-view ${isVisible ? "is-visible" : "is-hidden"}`}
      src={tab.url}
      partition={partition}
      allowpopups
      aria-hidden={!isVisible}
    />
  );
}

interface NavigationState {
  canGoBack: boolean;
  canGoForward: boolean;
}
