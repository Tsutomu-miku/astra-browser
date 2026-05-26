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

export type SidebarSearchTarget =
  | { type: "favorite"; id: string; title: string; url: string }
  | { type: "tab"; id: string; title: string; url: string };

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

export function getSidebarSearchTargets(input: SidebarFilterResult): SidebarSearchTarget[] {
  return [
    ...input.pinnedTabs.map(toTabTarget),
    ...input.favorites.map((favorite) => ({
      type: "favorite" as const,
      id: favorite.id,
      title: favorite.title,
      url: favorite.url
    })),
    ...input.groupedTabs.flatMap((entry) => entry.tabs.map(toTabTarget)),
    ...input.regularTabs.map(toTabTarget)
  ];
}

export function clampSidebarSearchIndex(index: number, targetCount: number): number {
  if (targetCount <= 0) return 0;
  if (!Number.isFinite(index)) return 0;
  return Math.min(targetCount - 1, Math.max(0, index));
}

export function getNextSidebarSearchIndex(
  currentIndex: number,
  targetCount: number,
  key: "ArrowDown" | "ArrowUp" | "End" | "Home"
): number {
  if (targetCount <= 0) return 0;
  const current = clampSidebarSearchIndex(currentIndex, targetCount);

  switch (key) {
    case "ArrowDown":
      return (current + 1) % targetCount;
    case "ArrowUp":
      return (current - 1 + targetCount) % targetCount;
    case "End":
      return targetCount - 1;
    case "Home":
      return 0;
  }
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

function toTabTarget(tab: BrowserTab): SidebarSearchTarget {
  return {
    type: "tab",
    id: tab.id,
    title: tab.title || tab.url,
    url: tab.url
  };
}
