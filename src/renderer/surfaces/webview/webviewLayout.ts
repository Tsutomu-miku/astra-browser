import type { BrowserState, BrowserTab, Workspace } from "../../domain/browser-core";

export interface WebviewLayoutTab {
  isVisible: boolean;
  tab: BrowserTab;
}

export function getKeepAliveWebviewTabs(
  workspace: Workspace,
  activeTab: BrowserTab,
  state: Pick<BrowserState, "splitMode" | "splitTabId">
): WebviewLayoutTab[] {
  const splitTab = state.splitMode
    ? workspace.tabs.find((tab) => tab.id === state.splitTabId && tab.id !== activeTab.id)
    : undefined;
  const visibleTabs = splitTab ? [activeTab, splitTab] : [activeTab];
  const visibleTabIds = new Set(visibleTabs.map((tab) => tab.id));

  return [
    ...visibleTabs.map((tab) => ({ isVisible: true, tab })),
    ...workspace.tabs
      .filter((tab) => !visibleTabIds.has(tab.id))
      .map((tab) => ({ isVisible: false, tab }))
  ];
}
