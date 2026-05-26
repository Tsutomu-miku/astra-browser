import type { Favorite, Workspace } from "../../domain/browser-core";

export type NumberShortcutTarget =
  | { type: "essential"; title: string; url: string }
  | { type: "tab"; tabId: string };

export function getNumberShortcutTarget(
  essentials: Favorite[],
  workspace: Pick<Workspace, "tabs">,
  index: number
): NumberShortcutTarget | null {
  if (index < 0) return null;

  const essential = essentials[index];
  if (essential) {
    return { type: "essential", title: essential.title, url: essential.url };
  }

  const tabIndex = index - essentials.length;
  const orderedTabs = [
    ...workspace.tabs.filter((tab) => tab.isPinned),
    ...workspace.tabs.filter((tab) => !tab.isPinned)
  ];
  const tab = orderedTabs[tabIndex];

  return tab ? { type: "tab", tabId: tab.id } : null;
}
