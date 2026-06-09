import type { BrowserTab, ClosedTab, Workspace } from "../browser";
import { getReadableUrlTitle } from "../browser";

export type TabDropPlacement = "before" | "after";

function snapshotTabForClose(tab: BrowserTab): ClosedTab {
  return {
    title: tab.title || getReadableUrlTitle(tab.url),
    customTitle: tab.customTitle,
    url: tab.url,
    faviconUrl: tab.faviconUrl,
    groupId: tab.groupId,
    canGoBack: tab.canGoBack,
    canGoForward: tab.canGoForward,
    isMuted: tab.isMuted,
    isPinned: tab.isPinned,
    zoomFactor: tab.zoomFactor,
    closedAt: Date.now()
  };
}

export function prependClosedTabs(workspace: Workspace, tabs: BrowserTab[]) {
  workspace.closedTabs = [
    ...tabs.map(snapshotTabForClose).reverse(),
    ...workspace.closedTabs
  ].slice(0, 25);
}
