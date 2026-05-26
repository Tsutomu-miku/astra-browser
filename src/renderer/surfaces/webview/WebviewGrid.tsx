import {
  Fragment,
  useEffect,
  useRef,
  useState,
  type CSSProperties,
  type DragEvent,
  type KeyboardEvent,
  type PointerEvent
} from "react";
import { FiColumns, FiGrid, FiMinus, FiMoreHorizontal, FiSidebar } from "react-icons/fi";

import { getWorkspacePartition, isInternalNewTabUrl, type BrowserTab } from "../../domain/browser-core";
import type { BrowserController } from "../../hooks/types";
import {
  getNavigationState,
  registerReadyWebview,
  syncWebviewPreferences,
  unregisterWebview,
  type NavigationState
} from "../../platform/webviewLifecycle";
import type { WebviewElement } from "../../types/browser-ui";
import { StartPage } from "../start/StartPage";
import {
  canResizeSplitLayout,
  DEFAULT_SPLIT_RATIO,
  getSplitRatioFromPoint,
  getKeepAliveWebviewTabs,
  normalizeSplitRatio
} from "./webviewLayout";

export function WebviewGrid({ controller }: { controller: BrowserController }) {
  const { activeTab, activeWorkspace, actions, splitLayout, state, webviews } = controller;
  const [isSplitDropTarget, setSplitDropTarget] = useState(false);
  const [splitRatio, setSplitRatio] = useState(DEFAULT_SPLIT_RATIO);
  const splitGridRef = useRef<HTMLElement | null>(null);
  const layoutTabs = getKeepAliveWebviewTabs(activeWorkspace, activeTab, state);
  const visibleCount = layoutTabs.filter((entry) => entry.isVisible).length;
  const partition = getWorkspacePartition(activeWorkspace);
  const canResizeSplit = canResizeSplitLayout(splitLayout, visibleCount);
  const splitStyle = visibleCount === 2
    ? { "--split-primary-size": `${normalizeSplitRatio(splitRatio) * 100}%` } as CSSProperties
    : undefined;

  function getDraggedTabId(event: DragEvent<HTMLElement>) {
    return event.dataTransfer.getData("text/plain");
  }

  function hasTabDragPayload(event: DragEvent<HTMLElement>) {
    return Array.from(event.dataTransfer.types).includes("text/plain");
  }

  function canSplitDrop(event: DragEvent<HTMLElement>) {
    const tabId = getDraggedTabId(event);
    return Boolean(tabId && tabId !== activeTab.id && activeWorkspace.tabs.some((tab) => tab.id === tabId));
  }

  function onSplitDragOver(event: DragEvent<HTMLElement>) {
    if (!hasTabDragPayload(event)) return;

    event.preventDefault();
    event.dataTransfer.dropEffect = "move";
    setSplitDropTarget(true);
  }

  function onSplitDrop(event: DragEvent<HTMLElement>) {
    if (!canSplitDrop(event)) {
      setSplitDropTarget(false);
      return;
    }

    event.preventDefault();
    actions.openTabInSplit(getDraggedTabId(event));
    setSplitDropTarget(false);
  }

  function resizeSplitFromPointer(event: PointerEvent<HTMLElement>) {
    const rect = splitGridRef.current?.getBoundingClientRect();
    if (!rect || !canResizeSplit) return;

    setSplitRatio(getSplitRatioFromPoint(splitLayout, { x: event.clientX, y: event.clientY }, rect));
  }

  function onResizeKeyDown(event: KeyboardEvent<HTMLButtonElement>) {
    const isDecreaseKey = (
      (splitLayout === "horizontal" && event.key === "ArrowLeft") ||
      (splitLayout === "vertical" && event.key === "ArrowUp")
    );
    const isIncreaseKey = (
      (splitLayout === "horizontal" && event.key === "ArrowRight") ||
      (splitLayout === "vertical" && event.key === "ArrowDown")
    );

    if (isDecreaseKey) {
      event.preventDefault();
      setSplitRatio((ratio) => normalizeSplitRatio(ratio - 0.05));
    } else if (isIncreaseKey) {
      event.preventDefault();
      setSplitRatio((ratio) => normalizeSplitRatio(ratio + 0.05));
    } else if (event.key === "Home" || event.key === "End") {
      event.preventDefault();
      setSplitRatio(DEFAULT_SPLIT_RATIO);
    }
  }

  return (
    <section
      ref={splitGridRef}
      className={`view-grid ${visibleCount > 1 ? "is-split" : ""} split-count-${visibleCount} split-layout-${splitLayout} ${isSplitDropTarget ? "is-split-drop-target" : ""}`}
      style={splitStyle}
      aria-label="Browser content"
      onDragOver={onSplitDragOver}
      onDragLeave={() => setSplitDropTarget(false)}
      onDrop={onSplitDrop}
    >
      {layoutTabs.map(({ isVisible, tab }, index) => {
        const surface = isInternalNewTabUrl(tab.url)
          ? <StartPage controller={controller} isVisible={isVisible} />
          : (
            <BrowserWebview
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
          );

        return (
          <Fragment key={tab.id}>
            {isVisible ? (
              <div className="browser-pane">
                {surface}
                {visibleCount > 1 && (
                  <SplitPaneOverlay
                    isPrimary={tab.id === activeTab.id}
                    tabId={tab.id}
                    controller={controller}
                  />
                )}
              </div>
            ) : surface}
            {canResizeSplit && index === 0 && (
              <button
                className={`split-resizer is-${splitLayout}`}
                type="button"
                aria-label="Resize split view"
                onKeyDown={onResizeKeyDown}
                onPointerDown={(event) => {
                  event.currentTarget.setPointerCapture(event.pointerId);
                  resizeSplitFromPointer(event);
                }}
                onPointerMove={(event) => {
                  if (event.currentTarget.hasPointerCapture(event.pointerId)) {
                    resizeSplitFromPointer(event);
                  }
                }}
                onDoubleClick={() => setSplitRatio(DEFAULT_SPLIT_RATIO)}
              />
            )}
          </Fragment>
        );
      })}
    </section>
  );
}

function SplitPaneOverlay({
  controller,
  isPrimary,
  tabId
}: {
  controller: BrowserController;
  isPrimary: boolean;
  tabId: string;
}) {
  const { actions } = controller;

  return (
    <div className="split-pane-overlay" aria-label="Split pane controls">
      <span className="split-pane-handle" title="Split pane"><FiMoreHorizontal /></span>
      <button type="button" title="Horizontal split layout" aria-label="Horizontal split layout" onClick={() => actions.setSplitLayout("horizontal")}><FiColumns /></button>
      <button type="button" title="Vertical split layout" aria-label="Vertical split layout" onClick={() => actions.setSplitLayout("vertical")}><FiSidebar /></button>
      <button type="button" title="Grid split layout" aria-label="Grid split layout" onClick={() => actions.setSplitLayout("grid")}><FiGrid /></button>
      <button
        type="button"
        title={isPrimary ? "Close split view" : "Unsplit tab"}
        aria-label={isPrimary ? "Close split view" : "Unsplit tab"}
        onClick={() => {
          isPrimary ? actions.toggleSplitMode() : actions.removeTabFromSplit(tabId);
        }}
      >
        <FiMinus />
      </button>
    </div>
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
