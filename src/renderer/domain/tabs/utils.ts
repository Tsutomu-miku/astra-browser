import type { BrowserTab, Workspace } from "../browser";
import { getReadableUrlTitle } from "../browser";

export type TabDropPlacement = "before" | "after";

export function prependClosedTabs(workspace: Workspace, tabs: BrowserTab[]) {
  workspace.closedTabs = [
    ...tabs.map((tab) => ({
      title: tab.title || getReadableUrlTitle(tab.url),
      url: tab.url,
      closedAt: Date.now()
    })).reverse(),
    ...workspace.closedTabs
  ].slice(0, 25);
}
