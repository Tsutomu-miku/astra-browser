import { resolveFavoriteTab, type BrowserTab, type Workspace } from "../../../domain/browser";
import { getGroupedTabs } from "../../../domain/tabs/groups";

export interface SidebarTabFolders {
  groupedTabs: Array<{ group: Workspace["tabGroups"][number]; tabs: BrowserTab[] }>;
  pinnedTabs: BrowserTab[];
  regularTabs: BrowserTab[];
}

export function getSidebarTabFolders(workspace: Workspace): SidebarTabFolders {
  const favoriteFolderTabIds = getFavoriteFolderTabIds(workspace);
  const pinnedTabs = workspace.tabs.filter((tab) => tab.isPinned && !favoriteFolderTabIds.has(tab.id));
  const groupedTabs = getGroupedTabs(workspace)
    .map(({ group, tabs }) => ({
      group,
      tabs: tabs.filter((tab) => !favoriteFolderTabIds.has(tab.id))
    }))
    .filter((entry) => entry.tabs.length > 0);
  const groupedTabIds = new Set(groupedTabs.flatMap((entry) => entry.tabs.map((tab) => tab.id)));
  const regularTabs = workspace.tabs.filter((tab) => (
    !tab.isPinned &&
    !groupedTabIds.has(tab.id) &&
    !favoriteFolderTabIds.has(tab.id)
  ));

  return {
    groupedTabs,
    pinnedTabs,
    regularTabs
  };
}

function getFavoriteFolderTabIds(workspace: Workspace): Set<string> {
  return new Set(workspace.favorites
    .map((favorite) => resolveFavoriteTab(workspace, favorite)?.id)
    .filter((tabId): tabId is string => Boolean(tabId)));
}
