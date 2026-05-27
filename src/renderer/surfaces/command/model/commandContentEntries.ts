import type { BrowserState, Workspace } from "../../../domain/browser";
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
      run: () => actions.openUrlInActiveWorkspace(essential.url, essential.title),
      runInSplit: () => actions.openUrlInSplit(essential.url, essential.title),
      runPreview: () => actions.openGlance(essential.url, essential.title)
    })),
    ...workspace.favorites.map((favorite) => ({
      title: favorite.title,
      subtitle: `Favorite · ${favorite.url}`,
      run: () => actions.openUrlInActiveWorkspace(favorite.url, favorite.title),
      runInSplit: () => actions.openUrlInSplit(favorite.url, favorite.title),
      runPreview: () => actions.openGlance(favorite.url, favorite.title)
    })),
    ...workspace.tabs.map((tab) => ({
      title: tab.title || tab.url,
      subtitle: `Open tab · ${tab.url}`,
      run: () => actions.selectTab(tab.id),
      runInSplit: () => actions.openTabInSplit(tab.id),
      runPreview: () => actions.openGlance(tab.url, tab.title)
    })),
    ...workspace.closedTabs.slice(0, 10).map((tab, index) => ({
      title: `Reopen ${tab.title || tab.url}`,
      subtitle: `Recently closed · ${tab.url}`,
      run: () => actions.restoreClosedTab(index)
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
