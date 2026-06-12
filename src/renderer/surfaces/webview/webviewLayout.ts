import type { BrowserTab, SplitLayout, Workspace } from "../../domain/browser";
import { getActiveAncillaryTabId, getAncillaryTabIds, isSplitModeActiveFromWorkspace } from "../../domain/tabs/splitView";

export interface WebviewLayoutTab {
  isVisible: boolean;
  tab: BrowserTab;
  pane: "primary" | "ancillary";
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

export function canResizeSplitLayout(layout: string, hasSplit: boolean): layout is ResizableSplitLayout {
  return hasSplit && (layout === "horizontal" || layout === "vertical");
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

/**
 * Arc-style split view: primary pane + ancillary pane.
 * In grid layout, all ancillary tabs are visible side-by-side.
 * In horizontal/vertical layout, only the active ancillary tab is visible.
 *
 * All ancillary tabs stay alive (webview mounted) for instant switching.
 * Non-ancillary sleeping tabs are not mounted.
 *
 * Returns keep-alive tabs ordered: active primary → active ancillary → other ancillary → other alive tabs
 */
export function getKeepAliveWebviewTabs(
  workspace: Workspace
): WebviewLayoutTab[] {
  const splitActive = isSplitModeActiveFromWorkspace(workspace);
  const ancillaryIds = getAncillaryTabIds(workspace);
  const activeAncillaryId = getActiveAncillaryTabId(workspace);
  const ancillarySet = new Set(ancillaryIds);
  const isGridLayout = workspace.splitLayout === "grid";

  const primary: WebviewLayoutTab[] = [];
  const ancillaryActive: WebviewLayoutTab[] = [];
  const ancillaryOther: WebviewLayoutTab[] = [];
  const otherAlive: WebviewLayoutTab[] = [];

  for (const tab of workspace.tabs) {
    const isActive = tab.id === workspace.activeTabId;
    const isAncillary = ancillarySet.has(tab.id);
    const isActiveAncillary = tab.id === activeAncillaryId;

    // Visibility: primary always visible; ancillary visible if grid mode or active ancillary
    const isVisible = isActive || (isAncillary && (isGridLayout || isActiveAncillary));

    const entry: WebviewLayoutTab = {
      isVisible,
      tab,
      pane: isAncillary ? "ancillary" : "primary"
    };

    if (isActive) {
      primary.push(entry);
    } else if (isActiveAncillary) {
      ancillaryActive.push(entry);
    } else if (isAncillary) {
      ancillaryOther.push(entry);
    } else if (!tab.isSleeping) {
      otherAlive.push(entry);
    }
    // Sleeping non-ancillary tabs: not returned (no webview)
  }

  return [...primary, ...ancillaryActive, ...ancillaryOther, ...otherAlive];
}

export function getVisiblePaneCount(workspace: Workspace): number {
  let count = 1; // primary always visible
  if (isSplitModeActiveFromWorkspace(workspace)) count += 1;
  return count;
}
