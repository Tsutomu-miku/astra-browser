import {
  Fragment,
  useRef,
  useState,
  type CSSProperties,
  type DragEvent,
  type KeyboardEvent,
  type PointerEvent
} from "react";
import { FiX, FiMaximize2 } from "react-icons/fi";

import { getPointerDropZone, type DropAxis } from "../../common/drag-drop/dropPlacement";
import { getWorkspacePartition, isInternalNewTabUrl } from "../../domain/browser";
import type { BrowserController } from "../../app/controller/types";
import { StartPage } from "../start/StartPage";
import { BrowserWebview } from "./components/BrowserWebview";
import {
  canResizeSplitLayout,
  DEFAULT_SPLIT_RATIO,
  getKeepAliveWebviewTabs,
  getSplitRatioFromPoint,
  normalizeSplitRatio
} from "./webviewLayout";

type SplitDropZone = "before" | "after" | "onto" | null;

const EDGE_DROP_ZONE_WIDTH = 120; // px - width of the edge drop zone for split creation

export function WebviewGrid({ controller }: { controller: BrowserController }) {
  const { activeTab, activeWorkspace, actions, registerWebview, removeWebview, splitLayout, state } = controller;
  const [isSplitDropTarget, setSplitDropTarget] = useState(false);
  const [splitDropZone, setSplitDropZone] = useState<SplitDropZone>(null);
  const [splitRatio, setSplitRatio] = useState(DEFAULT_SPLIT_RATIO);
  const splitGridRef = useRef<HTMLElement | null>(null);

  const layoutTabs = getKeepAliveWebviewTabs(activeWorkspace);
  const hasSplit = activeWorkspace.splitMode && activeWorkspace.ancillaryTabIds.length > 0;
  const partition = getWorkspacePartition(activeWorkspace);
  const canResizeSplit = canResizeSplitLayout(splitLayout, hasSplit);

  const visibleTabs = layoutTabs.filter((entry) => entry.isVisible);
  const primaryTab = visibleTabs.find((t) => t.pane === "primary");
  const ancillaryTab = visibleTabs.find((t) => t.pane === "ancillary");

  const splitStyle = hasSplit && canResizeSplit
    ? { "--split-primary-size": `${normalizeSplitRatio(splitRatio) * 100}%` } as CSSProperties
    : undefined;

  const splitDropAxis: DropAxis = splitLayout === "vertical" ? "vertical" : "horizontal";
  const splitSide = activeWorkspace.splitSide ?? "right";

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
    const tabId = getDraggedTabId(event);
    // Set split side based on drop position (before = left, after = right)
    if (splitDropZone === "before") {
      actions.setSplitPaneSide?.("left");
    } else if (splitDropZone === "after") {
      actions.setSplitPaneSide?.("right");
    }
    actions.openTabInSplit(tabId);
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

  function renderWebview(entry: { isVisible: boolean; tab: typeof activeTab }) {
    const { tab, isVisible } = entry;
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

    return surface;
  }

  return (
    <section
      ref={splitGridRef}
      className={`view-grid ${hasSplit ? "is-split" : ""} split-count-${visibleTabs.length} split-layout-${splitLayout} split-side-${splitSide} ${isSplitDropTarget ? "is-split-drop-target" : ""}`}
      data-split-drop-zone={isSplitDropTarget && splitDropZone ? splitDropZone : undefined}
      style={splitStyle}
      aria-label="Browser content"
      onDragOver={onSplitDragOver}
      onDragLeave={onSplitDragLeave}
      onDrop={onSplitDrop}
    >
      {/* Primary pane */}
      {primaryTab && (
        <div className="browser-pane is-primary">
          {renderWebview(primaryTab)}
        </div>
      )}

      {/* Split resizer - between panes */}
      {hasSplit && canResizeSplit && (
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

      {/* Ancillary / secondary pane */}
      {hasSplit && ancillaryTab && (
        <div className="browser-pane is-ancillary">
          {/* Ancillary pane header — Arc-style */}
          <div className="ancillary-pane-header">
            <span className="ancillary-pane-title" title={ancillaryTab.tab.title}>
              {ancillaryTab.tab.title || ancillaryTab.tab.url}
            </span>
            <div className="ancillary-pane-actions">
              <button
                type="button"
                className="ancillary-pane-btn"
                title="Make this the main pane"
                aria-label="Make main pane"
                onClick={() => actions.swapSplitPanes?.()}
              >
                <FiMaximize2 />
              </button>
              <button
                type="button"
                className="ancillary-pane-btn"
                title="Close split pane"
                aria-label="Close split pane"
                onClick={() => actions.toggleSplitMode()}
              >
                <FiX />
              </button>
            </div>
          </div>
          {renderWebview(ancillaryTab)}
        </div>
      )}

      {/* Edge drop zone hints — visible while dragging a tab */}
      {isSplitDropTarget && !hasSplit && (
        <>
          <div className="split-edge-drop-zone is-left" data-drop-active={splitDropZone === "before"} />
          <div className="split-edge-drop-zone is-right" data-drop-active={splitDropZone === "after"} />
        </>
      )}
    </section>
  );
}
