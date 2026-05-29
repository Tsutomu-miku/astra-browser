import type { BrowserTab, Favorite, Workspace } from "../../domain/browser";

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
  const orderedTabs = getNumberShortcutTabs(workspace);
  const tab = orderedTabs[tabIndex];

  return tab ? { type: "tab", tabId: tab.id } : null;
}

export function getLastNumberShortcutTabTarget(
  workspace: Pick<Workspace, "tabs">
): Extract<NumberShortcutTarget, { type: "tab" }> | null {
  const tab = getNumberShortcutTabs(workspace).at(-1);
  return tab ? { type: "tab", tabId: tab.id } : null;
}

export function getNumberShortcutTabs(workspace: Pick<Workspace, "tabs">): BrowserTab[] {
  return [
    ...workspace.tabs.filter((tab) => tab.isPinned),
    ...workspace.tabs.filter((tab) => !tab.isPinned)
  ];
}
