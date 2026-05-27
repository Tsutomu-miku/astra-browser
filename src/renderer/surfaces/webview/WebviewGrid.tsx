import {
  Fragment,
  useRef,
  useState,
  type CSSProperties,
  type DragEvent,
  type KeyboardEvent,
  type PointerEvent
} from "react";

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
              <div className={`browser-pane ${tab.id === activeTab.id ? "is-active" : ""}`}>
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
