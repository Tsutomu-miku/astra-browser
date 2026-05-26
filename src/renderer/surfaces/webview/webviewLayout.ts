import type { BrowserState, BrowserTab, Workspace } from "../../domain/browser-core";
import { getSplitTabIds } from "../../domain/split-view";

export interface WebviewLayoutTab {
  isVisible: boolean;
  tab: BrowserTab;
}

export const DEFAULT_SPLIT_RATIO = 0.5;
export const MAX_SPLIT_RATIO = 0.75;
export const MIN_SPLIT_RATIO = 0.25;
export type ResizableSplitLayout = "horizontal" | "vertical";

export function normalizeSplitRatio(value: number): number {
  if (!Number.isFinite(value)) {
    return DEFAULT_SPLIT_RATIO;
  }

  return Math.min(MAX_SPLIT_RATIO, Math.max(MIN_SPLIT_RATIO, value));
}

export function canResizeSplitLayout(layout: string, visibleCount: number): layout is ResizableSplitLayout {
  return visibleCount === 2 && (layout === "horizontal" || layout === "vertical");
}

export function getSplitRatioFromPoint(
  layout: ResizableSplitLayout,
  point: { x: number; y: number },
  rect: Pick<DOMRect, "height" | "left" | "top" | "width">
): number {
  const size = layout === "vertical" ? rect.height : rect.width;
  if (size <= 0) return DEFAULT_SPLIT_RATIO;

  const offset = layout === "vertical" ? point.y - rect.top : point.x - rect.left;
  return normalizeSplitRatio(offset / size);
}

export function getKeepAliveWebviewTabs(
  workspace: Workspace,
  activeTab: BrowserTab,
  state: Pick<BrowserState, "splitMode" | "splitTabId" | "splitTabIds">
): WebviewLayoutTab[] {
  const splitTabs = getSplitTabIds(state)
    .map((tabId) => workspace.tabs.find((tab) => tab.id === tabId && tab.id !== activeTab.id))
    .filter((tab): tab is BrowserTab => Boolean(tab));
  const visibleTabs = [activeTab, ...splitTabs];
  const visibleTabIds = new Set(visibleTabs.map((tab) => tab.id));

  return [
    ...visibleTabs.map((tab) => ({ isVisible: true, tab })),
    ...workspace.tabs
      .filter((tab) => !visibleTabIds.has(tab.id) && !tab.isSleeping)
      .map((tab) => ({ isVisible: false, tab }))
  ];
}
