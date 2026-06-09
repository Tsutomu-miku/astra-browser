import { type BrowserTab, type Favorite, type Workspace } from "../../domain/browser";
import { getGroupedTabs } from "../../domain/tabs/groups";

export type NumberShortcutTarget =
  | { type: "essential"; title: string; url: string }
  | { type: "tab"; tabId: string };

type NumberShortcutWorkspace = Pick<Workspace, "favoriteOrder" | "tabGroups" | "tabs">;

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
  const favoriteTabs = getNumberShortcutFavoriteTabs(workspace);
  const favoriteTabIds = new Set(favoriteTabs.map((tab) => tab.id));
  const visibleGroupedTabs = getGroupedTabs(workspace as Workspace)
    .filter(({ group }) => !group.isCollapsed)
    .flatMap(({ tabs }) => tabs)
    .filter((tab) => !favoriteTabIds.has(tab.id));

  return [
    ...workspace.tabs.filter((tab) => tab.isPinned && !favoriteTabIds.has(tab.id)),
    ...favoriteTabs,
    ...visibleGroupedTabs,
    ...workspace.tabs.filter((tab) => (
      !tab.isPinned &&
      !favoriteTabIds.has(tab.id) &&
      !groupIds.has(tab.groupId ?? "")
    ))
  ];
}

function getNumberShortcutFavoriteTabs(workspace: NumberShortcutWorkspace): BrowserTab[] {
  const tabById = new Map(workspace.tabs.map((tab) => [tab.id, tab]));
  const tabs: BrowserTab[] = [];
  for (const tabId of workspace.favoriteOrder) {
    const tab = tabById.get(tabId);
    if (tab && tab.isFavorite) tabs.push(tab);
  }
  return tabs;
}
