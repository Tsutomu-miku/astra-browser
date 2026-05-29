import type { BrowserState, Workspace } from "../../domain/browser";
import { getMemorySaverProtectedTabIds, isMemoryReleasableTab } from "../../domain/tabs/sleepPolicy";

export interface MemorySaverState {
  mountedWebviews: number;
  protectedTabs: number;
  reclaimableTabs: number;
  sleepAfterMinutes: number;
  sleepEnabled: boolean;
  sleepingTabs: number;
  summary: string;
}

export function getMemorySaverState(
  workspace: Workspace,
  state: Pick<BrowserState, "splitMode" | "splitTabId" | "splitTabIds"> & Partial<Pick<BrowserState, "settings">>
): MemorySaverState {
  const protectedTabIds = getMemorySaverProtectedTabIds(workspace, state);
  let protectedTabs = 0;
  let reclaimableTabs = 0;
  let sleepingTabs = 0;

  for (const tab of workspace.tabs) {
    if (tab.isSleeping) {
      sleepingTabs += 1;
      continue;
    }

    if (isMemoryReleasableTab(tab, protectedTabIds)) {
      reclaimableTabs += 1;
    } else {
      protectedTabs += 1;
    }
  }

  const mountedWebviews = workspace.tabs.length - sleepingTabs;

  return {
    mountedWebviews,
    protectedTabs,
    reclaimableTabs,
    sleepAfterMinutes: state.settings?.memorySaverIdleMinutes ?? 30,
    sleepEnabled: state.settings?.memorySaverEnabled ?? true,
    sleepingTabs,
    summary: `${reclaimableTabs} releasable · ${sleepingTabs} sleeping · ${protectedTabs} protected`
  };
}
