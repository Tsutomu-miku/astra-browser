import { useEffect, useRef, useState } from "react";
import { FiColumns, FiExternalLink, FiX } from "react-icons/fi";

import { getReadableUrlTitle, getWorkspacePartition } from "../../domain/browser-core";
import type { BrowserController } from "../../hooks/types";
import type { WebviewElement } from "../../types/browser-ui";

export function GlancePanel({ controller }: { controller: BrowserController }) {
  const { actions, activeWorkspace, glance } = controller;
  const webviewRef = useRef<WebviewElement | null>(null);
  const [previewUrl, setPreviewUrl] = useState(glance?.url ?? "");
  const [previewTitle, setPreviewTitle] = useState(glance?.title ?? "");

  useEffect(() => {
    if (!glance) return;

    setPreviewUrl(glance.url);
    setPreviewTitle(glance.title || getReadableUrlTitle(glance.url));
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

    const onNavigate = (event: Event) => {
      const url = (event as { url?: string }).url;
      if (!url) return;
      setPreviewUrl(url);
      setPreviewTitle((current) => current || getReadableUrlTitle(url));
    };
    const onTitle = (event: Event) => {
      const title = (event as { title?: string }).title;
      if (title) setPreviewTitle(title);
    };

    webview.addEventListener("did-navigate", onNavigate);
    webview.addEventListener("did-navigate-in-page", onNavigate);
    webview.addEventListener("page-title-updated", onTitle);
    return () => {
      webview.removeEventListener("did-navigate", onNavigate);
      webview.removeEventListener("did-navigate-in-page", onNavigate);
      webview.removeEventListener("page-title-updated", onTitle);
    };
  }, [glance?.url]);

  if (!glance) return null;

  const title = previewTitle || getReadableUrlTitle(previewUrl);

  return (
    <section
      className="glance-backdrop"
      aria-label="Glance preview"
      onClick={(event) => {
        if (event.target === event.currentTarget) actions.closeGlance();
      }}
    >
      <article className="glance-panel" role="dialog" aria-modal="true" aria-label={title}>
        <header className="glance-header">
          <div className="glance-title-block">
            <p className="glance-kicker">Glance</p>
            <h2>{title}</h2>
            <span>{previewUrl}</span>
          </div>
          <div className="glance-actions">
            <button className="icon-button" title="Open in tab" type="button" onClick={() => {
              actions.navigateActiveTab(previewUrl);
              actions.closeGlance();
            }}>
              <FiExternalLink />
            </button>
            <button className="icon-button" title="Open in split view" type="button" onClick={() => {
              actions.openUrlInSplit(previewUrl, title);
              actions.closeGlance();
            }}>
              <FiColumns />
            </button>
            <button className="icon-button" title="Close Glance" type="button" onClick={actions.closeGlance}>
              <FiX />
            </button>
          </div>
        </header>
        <webview
          key={glance.url}
          ref={webviewRef}
          className="glance-webview"
          src={glance.url}
          partition={getWorkspacePartition(activeWorkspace)}
          allowpopups
        />
      </article>
    </section>
  );
}
