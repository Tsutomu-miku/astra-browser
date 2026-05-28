import type { BrowserState, BrowserTab, Workspace } from "../../../domain/browser";
import { shortcutLabels } from "../../../common/shortcuts/shortcutLabels";
import { getSplitTabIds } from "../../../domain/tabs/splitView";
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
      shortcut: shortcutLabels.unsplitAll,
      run: actions.toggleSplitMode
    }]
    : [];
  const focusSplitPaneCommands: Command[] = getSplitTabIds(state)
    .map((tabId) => workspace.tabs.find((tab) => tab.id === tabId))
    .filter((tab): tab is BrowserTab => Boolean(tab))
    .map((tab) => ({
      title: `Focus ${tab.title || tab.url} split pane`,
      subtitle: "Make this split pane active",
      run: () => actions.focusSplitPane(tab.id)
    }));

  return [
    {
      title: state.splitMode ? "Close split view" : "Open split view",
      subtitle: state.splitMode ? `${state.splitTabIds.length + 1} panes open` : "Show Chromium webviews side by side",
      shortcut: shortcutLabels.splitToggle,
      run: actions.toggleSplitMode
    },
    ...unsplitCommands,
    ...focusSplitPaneCommands,
    {
      title: "Fill split grid",
      subtitle: "Open up to four tabs in Zen-style split view",
      shortcut: shortcutLabels.splitGrid,
      run: actions.fillSplitView
    },
    {
      title: "Split layout horizontal",
      subtitle: "Arrange split tabs side by side",
      shortcut: shortcutLabels.splitHorizontal,
      run: () => actions.setSplitLayout("horizontal")
    },
    {
      title: "Split layout vertical",
      subtitle: "Stack split tabs top to bottom",
      shortcut: shortcutLabels.splitVertical,
      run: () => actions.setSplitLayout("vertical")
    },
    {
      title: "Split layout grid",
      subtitle: "Arrange split tabs in a grid",
      shortcut: shortcutLabels.splitGrid,
      run: () => actions.setSplitLayout("grid")
    },
    ...splitTabCommands
  ];
}
