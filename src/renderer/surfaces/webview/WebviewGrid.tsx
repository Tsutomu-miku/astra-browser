import {
  Fragment,
  useRef,
  useState,
  type CSSProperties,
  type DragEvent,
  type KeyboardEvent,
  type PointerEvent
} from "react";

import { getPointerDropZone, type DropAxis } from "../../common/drag-drop/dropPlacement";
import { getWorkspacePartition, isInternalNewTabUrl } from "../../domain/browser";
import type { BrowserController } from "../../app/controller/types";
import { StartPage } from "../start/StartPage";
import { BrowserWebview } from "./components/BrowserWebview";
import { SplitPaneOverlay } from "./components/SplitPaneOverlay";
import {
  canResizeSplitLayout,
  DEFAULT_SPLIT_RATIO,
  getSplitRatioFromPoint,
  getKeepAliveWebviewTabs,
  normalizeSplitRatio
} from "./webviewLayout";

type SplitDropZone = "before" | "after" | "onto" | null;

export function WebviewGrid({ controller }: { controller: BrowserController }) {
  const { activeTab, activeWorkspace, actions, registerWebview, removeWebview, splitLayout, state } = controller;
  const [isSplitDropTarget, setSplitDropTarget] = useState(false);
  const [splitDropZone, setSplitDropZone] = useState<SplitDropZone>(null);
  const [splitRatio, setSplitRatio] = useState(DEFAULT_SPLIT_RATIO);
  const splitGridRef = useRef<HTMLElement | null>(null);
  const layoutTabs = getKeepAliveWebviewTabs(activeWorkspace, activeTab, state);
  const visibleCount = layoutTabs.filter((entry) => entry.isVisible).length;
  const partition = getWorkspacePartition(activeWorkspace);
  const canResizeSplit = canResizeSplitLayout(splitLayout, visibleCount);
  const splitStyle = visibleCount === 2
    ? { "--split-primary-size": `${normalizeSplitRatio(splitRatio) * 100}%` } as CSSProperties
    : undefined;
  const splitDropAxis: DropAxis = splitLayout === "vertical" ? "vertical" : "horizontal";

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
    if (!canSplitDrop(event)) return;

    event.preventDefault();
    event.dataTransfer.dropEffect = "move";
    setSplitDropTarget(true);

    if (splitGridRef.current) {
      const zone = getPointerDropZone(splitGridRef.current, event, splitDropAxis, 0.5);
      setSplitDropZone(zone);
    }
  }

  function onSplitDragLeave() {
    setSplitDropTarget(false);
    setSplitDropZone(null);
  }

  function onSplitDrop(event: DragEvent<HTMLElement>) {
    if (!canSplitDrop(event)) {
      setSplitDropTarget(false);
      setSplitDropZone(null);
      return;
    }

    event.preventDefault();
    actions.openTabInSplit(getDraggedTabId(event));
    setSplitDropTarget(false);
    setSplitDropZone(null);
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
      data-split-drop-zone={isSplitDropTarget && splitDropZone ? splitDropZone : undefined}
      style={splitStyle}
      aria-label="Browser content"
      onDragOver={onSplitDragOver}
      onDragLeave={onSplitDragLeave}
      onDrop={onSplitDrop}
    >
      {layoutTabs.map(({ isVisible, tab }, index) => {
        const surface = isInternalNewTabUrl(tab.url)
          ? <StartPage controller={controller} isVisible={isVisible} />
          : (
            <BrowserWebview
              isActive={tab.id === activeTab.id}
              isVisible={isVisible}
              partition={partition}
              tab={tab}
              onWebviewReady={registerWebview}
              onWebviewRemoved={removeWebview}
              onLoadingChange={(isLoading, navigationState) => actions.updateTab(tab.id, { isLoading, ...navigationState })}
              onFaviconChange={(faviconUrl) => actions.updateTab(tab.id, { faviconUrl })}
              onMediaStateChange={(isMediaPlaying) => actions.updateTab(tab.id, { isMediaPlaying })}
              onMuteChange={(isMuted) => {
                if (tab.isMuted !== isMuted) actions.updateTab(tab.id, { isMuted });
              }}
              onTitleChange={(title, explicitSet) => {
                const isActive = tab.id === activeTab.id;
                actions.updateTab(tab.id, {
                  title,
                  hasUnread: isActive ? false : Boolean(explicitSet) || tab.hasUnread
                });
              }}
              onPermissionRequest={(permission, active) => {
                if (permission === "camera" || permission === "media") {
                  actions.updateTab(tab.id, { isCameraOn: active });
                }
                if (permission === "microphone" || permission === "media") {
                  actions.updateTab(tab.id, { isMicrophoneOn: active });
                }
              }}
              onNavigate={(url) => {
                actions.updateTab(tab.id, {
                  url,
                  faviconUrl: undefined,
                  isCameraOn: false,
                  isMicrophoneOn: false,
                  hasUnread: tab.id === activeTab.id ? false : tab.hasUnread
                });
                actions.recordHistory(tab.id, url);
              }}
              onZoomChange={(zoomFactor) => {
                if (Math.abs(tab.zoomFactor - zoomFactor) > 0.01) {
                  actions.updateTab(tab.id, { zoomFactor });
                }
              }}
              onPageContent={(tabId, html) => actions.cachePageHtml(tabId, html)}
            />
          );

        return (
          <Fragment key={tab.id}>
            {isVisible ? (
              <div className={`browser-pane ${tab.id === activeTab.id ? "is-active" : ""}`}>
                {surface}
                {visibleCount > 1 && (
                  <SplitPaneOverlay
                    isPrimary={tab.id === activeTab.id}
                    tabId={tab.id}
                    title={tab.title || tab.url}
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
