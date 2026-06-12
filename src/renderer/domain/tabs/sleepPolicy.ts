import type { BrowserTab, Workspace } from "../browser";
import { getAncillaryTabIds } from "./splitView";

export function getMemorySaverProtectedTabIds(workspace: Workspace): Set<string> {
  return new Set([workspace.activeTabId, ...getAncillaryTabIds(workspace)].filter((tabId): tabId is string => Boolean(tabId)));
}

export function isMemoryReleasableTab(tab: BrowserTab, protectedTabIds: ReadonlySet<string>): boolean {
  return !tab.isSleeping && !tab.isPinned && !protectedTabIds.has(tab.id);
}

export function getMemoryReleasableTabs(workspace: Workspace): BrowserTab[] {
  const protectedTabIds = getMemorySaverProtectedTabIds(workspace);
  return workspace.tabs.filter((tab) => isMemoryReleasableTab(tab, protectedTabIds));
}

export function getGroupSleepableTabs(workspace: Workspace, groupId: string): BrowserTab[] {
  const protectedTabIds = getMemorySaverProtectedTabIds(workspace);
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
