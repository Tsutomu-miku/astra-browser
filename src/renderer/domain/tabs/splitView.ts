import type { BrowserState, Workspace } from "../browser/types";

export const MAX_SPLIT_VIEW_TABS = 4;
const MAX_SPLIT_TARGETS = MAX_SPLIT_VIEW_TABS - 1;

export function getSplitTabIds(state: Pick<BrowserState, "splitMode" | "splitTabId" | "splitTabIds">): string[] {
  if (!state.splitMode) {
    return [];
  }

  const persistedIds = Array.isArray(state.splitTabIds) ? state.splitTabIds : [];
  const ids = persistedIds.length > 0 ? persistedIds : state.splitTabId ? [state.splitTabId] : [];
  return uniqueTabIds(ids).slice(0, MAX_SPLIT_TARGETS);
}

export function setSplitTabIds(state: BrowserState, ids: string[]): void {
  const nextIds = uniqueTabIds(ids).slice(0, MAX_SPLIT_TARGETS);
  state.splitMode = nextIds.length > 0;
  state.splitTabIds = nextIds;
  state.splitTabId = nextIds[0] ?? null;
}

export function clearSplitView(state: BrowserState): void {
  setSplitTabIds(state, []);
}

export function pruneSplitTabIds(state: BrowserState, workspace: Workspace): void {
  const validTabIds = new Set(workspace.tabs.map((tab) => tab.id));
  const activeTabId = workspace.activeTabId;
  setSplitTabIds(
    state,
    getSplitTabIds(state).filter((tabId) => tabId !== activeTabId && validTabIds.has(tabId))
  );
}

function uniqueTabIds(ids: string[]): string[] {
  return Array.from(new Set(ids.filter(Boolean)));
}
