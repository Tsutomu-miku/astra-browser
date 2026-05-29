import { resolveFavoriteTab, type BrowserTab, type Workspace } from "../../../domain/browser";
import { getGroupedTabs } from "../../../domain/tabs/groups";

export interface SidebarTabLocations {
  favoriteTabIds: Set<string>;
  groupedTabs: Array<{ group: Workspace["tabGroups"][number]; tabs: BrowserTab[] }>;
  pinnedTabs: BrowserTab[];
  regularTabs: BrowserTab[];
}

export function getSidebarTabLocations(workspace: Workspace): SidebarTabLocations {
  const favoriteTabIds = getFavoriteTabIds(workspace);
  const pinnedTabs = workspace.tabs.filter((tab) => tab.isPinned && !favoriteTabIds.has(tab.id));
  const groupedTabs = getGroupedTabs(workspace)
    .map(({ group, tabs }) => ({
      group,
      tabs: tabs.filter((tab) => !favoriteTabIds.has(tab.id))
    }))
    .filter((entry) => entry.tabs.length > 0);
  const groupedTabIds = new Set(groupedTabs.flatMap((entry) => entry.tabs.map((tab) => tab.id)));
  const regularTabs = workspace.tabs.filter((tab) => (
    !tab.isPinned &&
    !groupedTabIds.has(tab.id) &&
    !favoriteTabIds.has(tab.id)
  ));

  return {
    favoriteTabIds,
    groupedTabs,
    pinnedTabs,
    regularTabs
  };
}

function getFavoriteTabIds(workspace: Workspace): Set<string> {
  return new Set(workspace.favorites
    .map((favorite) => resolveFavoriteTab(workspace, favorite)?.id)
    .filter((tabId): tabId is string => Boolean(tabId)));
}
