import { type BrowserTab, type TabGroup, type Workspace } from "../../../domain/browser";
import { isFavoriteGroup } from "../../../domain/common/favoriteTabs";
import { getGroupedTabs } from "../../../domain/tabs/groups";

export type SidebarFavoriteItem =
  | { kind: "tab"; tab: BrowserTab }
  | { kind: "group"; group: TabGroup; tabs: BrowserTab[] };

export interface SidebarTabFolders {
  favoriteItems: SidebarFavoriteItem[];
  groupedTabs: Array<{ group: TabGroup; tabs: BrowserTab[] }>;
  regularTabs: BrowserTab[];
}

export function getSidebarTabFolders(workspace: Workspace): SidebarTabFolders {
  const favoriteTabIds = new Set(
    workspace.tabs.filter((tab) => tab.isFavorite).map((tab) => tab.id)
  );

  // Non-favorite side: pinned tabs (never grouped) then regular tabs and groups.
  const pinnedTabs = workspace.tabs.filter(
    (tab) => tab.isPinned && !favoriteTabIds.has(tab.id)
  );
  const groupedTabs = getGroupedTabs(workspace)
    .map(({ group, tabs }) => ({
      group,
      tabs: tabs.filter((tab) => !favoriteTabIds.has(tab.id) && !tab.isPinned)
    }))
    .filter((entry) => entry.tabs.length > 0);
  const groupedTabIds = new Set(
    groupedTabs.flatMap((entry) => entry.tabs.map((tab) => tab.id))
  );

  // Favorites side: build an ordered list that interleaves individual tabs
  // with groups, following `favoriteOrder`. Groups appear once keyed by their
  // first member's position in `favoriteOrder`.
  const favoriteItems: SidebarFavoriteItem[] = [];
  const emittedGroupIds = new Set<string>();
  const favoriteTabIdsRendered = new Set<string>();
  const favoriteGroupsByTabId = new Map<string, { group: TabGroup; tabs: BrowserTab[] }>();
  for (const group of workspace.tabGroups) {
    const members = workspace.tabs.filter(
      (tab) => tab.groupId === group.id && tab.isFavorite
    );
    if (members.length > 0 && isFavoriteGroup(workspace, group)) {
      for (const member of members) {
        favoriteGroupsByTabId.set(member.id, { group, tabs: members });
      }
    }
  }
  for (const id of workspace.favoriteOrder) {
    const groupEntry = favoriteGroupsByTabId.get(id);
    if (groupEntry) {
      if (!emittedGroupIds.has(groupEntry.group.id)) {
        favoriteItems.push({ kind: "group", ...groupEntry });
        for (const t of groupEntry.tabs) favoriteTabIdsRendered.add(t.id);
        emittedGroupIds.add(groupEntry.group.id);
      }
      continue;
    }
    const tab = workspace.tabs.find((candidate) => candidate.id === id && candidate.isFavorite);
    if (tab) {
      favoriteItems.push({ kind: "tab", tab });
      favoriteTabIdsRendered.add(tab.id);
    }
  }

  const unpinnedRegularTabs = workspace.tabs.filter((tab) => (
    !tab.isPinned &&
    !groupedTabIds.has(tab.id) &&
    !favoriteTabIdsRendered.has(tab.id)
  ));

  return {
    favoriteItems,
    groupedTabs,
    regularTabs: [...pinnedTabs, ...unpinnedRegularTabs]
  };
}