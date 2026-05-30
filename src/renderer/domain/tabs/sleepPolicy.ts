import type { BrowserState, BrowserTab, Workspace } from "../browser";
import { getSplitTabIds } from "./splitView";

export type SleepPolicyState = Pick<BrowserState, "splitMode" | "splitTabId" | "splitTabIds">;

export function getMemorySaverProtectedTabIds(workspace: Workspace, state: SleepPolicyState): Set<string> {
  return new Set([workspace.activeTabId, ...getSplitTabIds(state)].filter((tabId): tabId is string => Boolean(tabId)));
}

export function isMemoryReleasableTab(tab: BrowserTab, protectedTabIds: ReadonlySet<string>): boolean {
  return !tab.isSleeping && !tab.isPinned && !protectedTabIds.has(tab.id);
}

export function getMemoryReleasableTabs(workspace: Workspace, state: SleepPolicyState): BrowserTab[] {
  const protectedTabIds = getMemorySaverProtectedTabIds(workspace, state);
  return workspace.tabs.filter((tab) => isMemoryReleasableTab(tab, protectedTabIds));
}

export function getGroupSleepableTabs(workspace: Workspace, state: SleepPolicyState, groupId: string): BrowserTab[] {
  const protectedTabIds = getMemorySaverProtectedTabIds(workspace, state);
  return workspace.tabs.filter((tab) => tab.groupId === groupId && isMemoryReleasableTab(tab, protectedTabIds));
}

export function markTabSleeping(tab: BrowserTab): void {
  tab.isSleeping = true;
  tab.isLoading = false;
  tab.canGoBack = false;
  tab.canGoForward = false;
}

export function markTabAwake(tab: BrowserTab, now = Date.now()): void {
  tab.isSleeping = false;
  tab.lastActiveAt = now;
}
