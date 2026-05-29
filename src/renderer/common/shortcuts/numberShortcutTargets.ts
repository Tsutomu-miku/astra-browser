import type { BrowserTab, Favorite, Workspace } from "../../domain/browser";
import { getGroupedTabs } from "../../domain/tabs/groups";

export type NumberShortcutTarget =
  | { type: "essential"; title: string; url: string }
  | { type: "tab"; tabId: string };

type NumberShortcutWorkspace = Pick<Workspace, "tabGroups" | "tabs">;

export function getNumberShortcutTarget(
  essentials: Favorite[],
  workspace: NumberShortcutWorkspace,
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
  workspace: NumberShortcutWorkspace
): Extract<NumberShortcutTarget, { type: "tab" }> | null {
  const tab = getNumberShortcutTabs(workspace).at(-1);
  return tab ? { type: "tab", tabId: tab.id } : null;
}

export function getNumberShortcutTabs(workspace: NumberShortcutWorkspace): BrowserTab[] {
  const groupIds = new Set(workspace.tabGroups.map((group) => group.id));
  const visibleGroupedTabs = getGroupedTabs(workspace as Workspace)
    .filter(({ group }) => !group.isCollapsed)
    .flatMap(({ tabs }) => tabs);

  return [
    ...workspace.tabs.filter((tab) => tab.isPinned),
    ...visibleGroupedTabs,
    ...workspace.tabs.filter((tab) => !tab.isPinned && !groupIds.has(tab.groupId ?? ""))
  ];
}
