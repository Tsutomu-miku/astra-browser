import { shortcutLabels } from "../../../common/shortcuts/shortcutLabels";
import { isEssential, isFavorite, type BrowserState, type BrowserTab, type Workspace } from "../../../domain/browser";
import type { Command, CommandActions } from "./commandTypes";

export function buildPageCommands(
  state: BrowserState,
  workspace: Workspace,
  activeTab: BrowserTab,
  actions: CommandActions
): Command[] {
  const pageTitle = activeTab.title || activeTab.url;
  const navigationCommands: Command[] = [
    ...(activeTab.canGoBack
      ? [{
        title: "Back",
        subtitle: activeTab.url,
        shortcut: shortcutLabels.back,
        run: () => actions.runWebviewAction("goBack")
      }]
      : []),
    ...(activeTab.canGoForward
      ? [{
        title: "Forward",
        subtitle: activeTab.url,
        shortcut: shortcutLabels.forward,
        run: () => actions.runWebviewAction("goForward")
      }]
      : []),
    {
      title: activeTab.isLoading ? "Stop loading" : "Reload page",
      subtitle: activeTab.url,
      shortcut: shortcutLabels.reload,
      run: () => actions.runWebviewAction(activeTab.isLoading ? "stop" : "reload")
    },
    {
      title: "Hard reload",
      subtitle: "Reload without cache",
      shortcut: shortcutLabels.hardReload,
      run: () => actions.runWebviewAction("reloadIgnoringCache")
    }
  ];

  return [
    ...navigationCommands,
    {
      title: isFavorite(workspace, activeTab.url) ? "Remove favorite" : "Add favorite",
      subtitle: activeTab.url,
      shortcut: shortcutLabels.favorite,
      run: actions.toggleActiveTabFavorite
    },
    {
      title: isEssential(state, activeTab.url) ? "Remove essential" : "Add essential",
      subtitle: "Show this page across Spaces",
      run: actions.toggleActiveTabEssential
    },
    {
      title: activeTab.isPinned ? "Unpin tab" : "Pin tab",
      subtitle: activeTab.url,
      run: actions.toggleActiveTabPinned
    },
    {
      title: activeTab.isMuted ? "Unmute tab" : "Mute tab",
      subtitle: activeTab.url,
      shortcut: shortcutLabels.mute,
      run: actions.toggleActiveTabMuted
    },
    {
      title: "Preview tab in Glance",
      subtitle: activeTab.url,
      run: () => actions.openGlance(activeTab.url, activeTab.title)
    },
    {
      title: "Copy current URL",
      subtitle: activeTab.url,
      run: () => actions.copyText(activeTab.url)
    },
    {
      title: "Copy current page title",
      subtitle: pageTitle,
      run: () => actions.copyText(pageTitle)
    }
  ];
}
