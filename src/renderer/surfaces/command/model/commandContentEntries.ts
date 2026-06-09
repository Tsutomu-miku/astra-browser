import { type BrowserState, type Workspace } from "../../../domain/browser";
import type { Command, CommandActions } from "./commandTypes";

export function buildContentCommands(
  state: BrowserState,
  workspace: Workspace,
  actions: CommandActions
): Command[] {
  return [
    ...state.essentials.map((essential) => ({
      title: essential.title,
      subtitle: `Essential · ${essential.url}`,
      run: () => actions.navigateActiveTab(essential.url),
      runInSplit: () => actions.openUrlInSplit(essential.url, essential.title),
      runPreview: () => actions.openGlance(essential.url, essential.title)
    })),
    ...workspace.favoriteOrder
      .map((tabId) => workspace.tabs.find((t) => t.id === tabId && t.isFavorite))
      .filter((t): t is Workspace["tabs"][number] => Boolean(t))
      .map((tab) => ({
        title: tab.title || tab.url,
        subtitle: `Favorite tab · ${tab.url}`,
        run: () => actions.selectTab(tab.id),
        runInSplit: () => actions.openTabInSplit(tab.id),
        runPreview: () => actions.openGlance(tab.url, tab.title)
      })),
    ...workspace.tabs.map((tab) => ({
      title: tab.title || tab.url,
      subtitle: `${getOpenTabCommandLabel(tab.id, workspace.activeTabId, tab.isSleeping)} · ${tab.url}`,
      run: () => actions.selectTab(tab.id),
      runInSplit: () => actions.openTabInSplit(tab.id),
      runPreview: () => actions.openGlance(tab.url, tab.title)
    })),
    ...workspace.closedTabs.slice(0, 10).map((tab, index) => ({
      title: `Reopen ${tab.title || tab.url}`,
      subtitle: `Recently closed · ${tab.url}`,
      run: () => actions.restoreClosedTab(index),
      runInSplit: () => actions.openUrlInSplit(tab.url, tab.title),
      runPreview: () => actions.openGlance(tab.url, tab.title)
    })),
    ...state.history.slice(0, 10).map((entry) => ({
      title: entry.title,
      subtitle: `History · ${entry.url}`,
      run: () => actions.openUrlInActiveWorkspace(entry.url, entry.title),
      runInSplit: () => actions.openUrlInSplit(entry.url, entry.title),
      runPreview: () => actions.openGlance(entry.url, entry.title)
    }))
  ];
}

function getOpenTabCommandLabel(tabId: string, activeTabId: string | null, isSleeping: boolean): string {
  if (tabId === activeTabId) return "Active tab";
  return isSleeping ? "Sleeping tab" : "Open tab";
}
