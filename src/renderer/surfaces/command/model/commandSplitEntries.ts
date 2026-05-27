import type { BrowserState, BrowserTab, Workspace } from "../../../domain/browser-core";
import type { Command, CommandActions } from "./commandTypes";

export function buildSplitCommands(
  state: BrowserState,
  workspace: Workspace,
  activeTab: BrowserTab,
  actions: CommandActions
): Command[] {
  const splitTabCommands = workspace.tabs
    .filter((tab) => tab.id !== activeTab.id)
    .map((tab) => ({
      title: `Open ${tab.title || tab.url} in split view`,
      subtitle: tab.url,
      run: () => actions.openTabInSplit(tab.id)
    }));
  const unsplitCommands: Command[] = state.splitMode
    ? [{
      title: "Unsplit all tabs",
      subtitle: "Close the current split view",
      run: actions.toggleSplitMode
    }]
    : [];

  return [
    {
      title: state.splitMode ? "Close split view" : "Open split view",
      subtitle: state.splitMode ? `${state.splitTabIds.length + 1} panes open` : "Show Chromium webviews side by side",
      run: actions.toggleSplitMode
    },
    ...unsplitCommands,
    {
      title: "Fill split grid",
      subtitle: "Open up to four tabs in Zen-style split view",
      run: actions.fillSplitView
    },
    {
      title: "Split layout horizontal",
      subtitle: "Arrange split tabs side by side",
      run: () => actions.setSplitLayout("horizontal")
    },
    {
      title: "Split layout vertical",
      subtitle: "Stack split tabs top to bottom",
      run: () => actions.setSplitLayout("vertical")
    },
    {
      title: "Split layout grid",
      subtitle: "Arrange split tabs in a grid",
      run: () => actions.setSplitLayout("grid")
    },
    ...splitTabCommands
  ];
}
