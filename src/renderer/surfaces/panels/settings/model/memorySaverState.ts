import type { BrowserState, Workspace } from "../../../../domain/browser";
import { getSplitTabIds } from "../../../../domain/tabs/splitView";

export interface MemorySaverState {
  mountedWebviews: number;
  protectedTabs: number;
  reclaimableTabs: number;
  sleepingTabs: number;
  summary: string;
}

export function getMemorySaverState(
  workspace: Workspace,
  state: Pick<BrowserState, "splitMode" | "splitTabId" | "splitTabIds">
): MemorySaverState {
  const visibleTabIds = new Set([workspace.activeTabId, ...getSplitTabIds(state)].filter(Boolean));
  let protectedTabs = 0;
  let reclaimableTabs = 0;
  let sleepingTabs = 0;

  for (const tab of workspace.tabs) {
    if (tab.isSleeping) {
      sleepingTabs += 1;
      continue;
    }

    if (visibleTabIds.has(tab.id) || tab.isPinned) {
      protectedTabs += 1;
    } else {
      reclaimableTabs += 1;
    }
  }

  const mountedWebviews = workspace.tabs.length - sleepingTabs;

  return {
    mountedWebviews,
    protectedTabs,
    reclaimableTabs,
    sleepingTabs,
    summary: `${reclaimableTabs} releasable · ${sleepingTabs} sleeping · ${protectedTabs} protected`
  };
}
