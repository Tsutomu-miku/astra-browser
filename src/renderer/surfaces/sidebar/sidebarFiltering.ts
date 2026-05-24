import type { BrowserTab, Favorite, TabGroup } from "../../domain/browser-core";

export interface SidebarGroupEntry {
  group: TabGroup;
  tabs: BrowserTab[];
}

export interface SidebarFilterInput {
  favorites: Favorite[];
  groupedTabs: SidebarGroupEntry[];
  pinnedTabs: BrowserTab[];
  regularTabs: BrowserTab[];
}

export interface SidebarFilterResult extends SidebarFilterInput {
  hasMatches: boolean;
  isFiltering: boolean;
}

export function filterSidebarItems(input: SidebarFilterInput, query: string): SidebarFilterResult {
  const normalizedQuery = query.trim().toLowerCase();
  if (!normalizedQuery) {
    return {
      ...input,
      hasMatches: hasAnyItems(input),
      isFiltering: false
    };
  }

  const pinnedTabs = input.pinnedTabs.filter((tab) => matchesTab(tab, normalizedQuery));
  const favorites = input.favorites.filter((favorite) => matchesFavorite(favorite, normalizedQuery));
  const groupedTabs = input.groupedTabs
    .map(({ group, tabs }) => ({
      group,
      tabs: group.name.toLowerCase().includes(normalizedQuery)
        ? tabs
        : tabs.filter((tab) => matchesTab(tab, normalizedQuery))
    }))
    .filter((entry) => entry.tabs.length > 0);
  const regularTabs = input.regularTabs.filter((tab) => matchesTab(tab, normalizedQuery));
  const filtered = { favorites, groupedTabs, pinnedTabs, regularTabs };

  return {
    ...filtered,
    hasMatches: hasAnyItems(filtered),
    isFiltering: true
  };
}

function hasAnyItems(input: SidebarFilterInput): boolean {
  return (
    input.pinnedTabs.length > 0 ||
    input.favorites.length > 0 ||
    input.groupedTabs.some((entry) => entry.tabs.length > 0) ||
    input.regularTabs.length > 0
  );
}

function matchesTab(tab: BrowserTab, query: string): boolean {
  return tab.title.toLowerCase().includes(query) || tab.url.toLowerCase().includes(query);
}

function matchesFavorite(favorite: Favorite, query: string): boolean {
  return favorite.title.toLowerCase().includes(query) || favorite.url.toLowerCase().includes(query);
}
