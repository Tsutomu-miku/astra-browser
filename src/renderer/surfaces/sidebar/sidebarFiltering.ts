import {
  clampListIndex,
  getNextListIndex,
  type ListNavigationKey
} from "../../common/navigation/listNavigation";
import { type BrowserTab, type Favorite, type TabGroup } from "../../domain/browser";

export interface SidebarGroupEntry {
  group: TabGroup;
  tabs: BrowserTab[];
}

export type SidebarFavoriteEntry =
  | { kind: "tab"; tab: BrowserTab }
  | { kind: "group"; group: TabGroup; tabs: BrowserTab[] };

export interface SidebarFilterInput {
  essentials: Favorite[];
  favorites: SidebarFavoriteEntry[];
  groupedTabs: SidebarGroupEntry[];
  pinnedTabs: BrowserTab[];
  regularTabs: BrowserTab[];
  workspaceTabs?: BrowserTab[];
}

export interface SidebarFilterResult extends SidebarFilterInput {
  hasMatches: boolean;
  isFiltering: boolean;
}

export type SidebarSearchTarget =
  | { type: "essential"; id: string; title: string; url: string }
  | { type: "favorite"; id: string; tabId: string; title: string; url: string }
  | { type: "tab"; id: string; title: string; url: string };

export type SidebarSearchNavigationKey = ListNavigationKey;

export interface SidebarSearchActionHint {
  id: "preview" | "split";
  label: string;
  modifier: string;
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
  const essentials = input.essentials.filter((essential) => matchesEssential(essential, normalizedQuery));
  const favorites = input.favorites
    .map((entry) => {
      if (entry.kind === "tab") {
        return matchesTab(entry.tab, normalizedQuery) ? entry : null;
      }
      const groupMatches = entry.group.name.toLowerCase().includes(normalizedQuery);
      const tabs = groupMatches ? entry.tabs : entry.tabs.filter((tab) => matchesTab(tab, normalizedQuery));
      return tabs.length > 0 ? { kind: "group" as const, group: entry.group, tabs } : null;
    })
    .filter((entry): entry is NonNullable<typeof entry> => entry !== null);
  const groupedTabs = input.groupedTabs
    .map(({ group, tabs }) => ({
      group,
      tabs: group.name.toLowerCase().includes(normalizedQuery)
        ? tabs
        : tabs.filter((tab) => matchesTab(tab, normalizedQuery))
    }))
    .filter((entry) => entry.tabs.length > 0);
  const regularTabs = input.regularTabs.filter((tab) => matchesTab(tab, normalizedQuery));
  const filtered = { essentials, favorites, groupedTabs, pinnedTabs, regularTabs, workspaceTabs: input.workspaceTabs };

  return {
    ...filtered,
    hasMatches: hasAnyItems(filtered),
    isFiltering: true
  };
}

export function getSidebarSearchTargets(input: SidebarFilterResult): SidebarSearchTarget[] {
  return [
    ...input.essentials.map((essential) => ({
      type: "essential" as const,
      id: essential.id,
      title: essential.title,
      url: essential.url
    })),
    ...input.pinnedTabs.map(toTabTarget),
    ...input.favorites.flatMap((entry) => (
      entry.kind === "tab" ? [toFavoriteTarget(entry.tab)] : entry.tabs.map(toFavoriteTarget)
    )),
    ...input.groupedTabs.flatMap((entry) => entry.tabs.map(toTabTarget)),
    ...input.regularTabs.map(toTabTarget)
  ];
}

export function getSidebarSearchActionHints(target: SidebarSearchTarget | undefined): SidebarSearchActionHint[] {
  if (!target) return [];
  return [
    { id: "preview", modifier: "Alt", label: "Preview" },
    { id: "split", modifier: "Shift", label: "Split" }
  ];
}

export function getSidebarSearchTargetElementId(target: SidebarSearchTarget): string {
  return `sidebar-search-${target.type}-${target.id}`;
}

export function clampSidebarSearchIndex(index: number, targetCount: number): number {
  return clampListIndex(index, targetCount);
}

export function getNextSidebarSearchIndex(
  currentIndex: number,
  targetCount: number,
  key: SidebarSearchNavigationKey
): number {
  return getNextListIndex(currentIndex, targetCount, key);
}

function hasAnyItems(input: SidebarFilterInput): boolean {
  return (
    input.essentials.length > 0 ||
    input.pinnedTabs.length > 0 ||
    input.favorites.length > 0 ||
    input.groupedTabs.some((entry) => entry.tabs.length > 0) ||
    input.regularTabs.length > 0
  );
}

function matchesTab(tab: BrowserTab, query: string): boolean {
  return tab.title.toLowerCase().includes(query) || tab.url.toLowerCase().includes(query);
}

function matchesEssential(favorite: Favorite, query: string): boolean {
  return favorite.title.toLowerCase().includes(query) || favorite.url.toLowerCase().includes(query);
}

function toFavoriteTarget(tab: BrowserTab): SidebarSearchTarget {
  return {
    type: "favorite",
    id: tab.id,
    tabId: tab.id,
    title: tab.title || tab.url,
    url: tab.url
  };
}

function toTabTarget(tab: BrowserTab): SidebarSearchTarget {
  return {
    type: "tab",
    id: tab.id,
    title: tab.title || tab.url,
    url: tab.url
  };
}
