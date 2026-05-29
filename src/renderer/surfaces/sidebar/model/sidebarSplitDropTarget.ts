import { readSidebarTabDragData } from "../../../common/drag-drop/sidebarDragPayload";
import type { BrowserTab, ClosedTab, Favorite } from "../../../domain/browser";

export type SidebarSplitDropSource =
  | { type: "tab"; tabId: string; title: string }
  | { type: "url"; title: string; url: string };

export interface SidebarSplitDropState {
  activeTabId: string;
  closedTabs: ClosedTab[];
  draggingClosedTabIndex: number | null;
  draggingEssentialId: string | null;
  draggingFavoriteId: string | null;
  draggingTabId: string | null;
  essentials: Favorite[];
  favorites: Favorite[];
  tabs: BrowserTab[];
}

export function getSidebarSplitDropSource(
  state: SidebarSplitDropState,
  getData: (type: string) => string = () => ""
): SidebarSplitDropSource | null {
  const tabId = state.draggingTabId || readSidebarTabDragData(getData);
  const tab = tabId ? state.tabs.find((candidate) => candidate.id === tabId) : undefined;
  if (tab && tab.id !== state.activeTabId) {
    return { type: "tab", tabId: tab.id, title: tab.title };
  }

  const essentialId = state.draggingEssentialId || getData("text/essential-id");
  const essential = essentialId ? state.essentials.find((candidate) => candidate.id === essentialId) : undefined;
  if (essential) return createUrlDropSource(essential);

  const favoriteId = state.draggingFavoriteId || getData("text/favorite-id");
  const favorite = favoriteId ? state.favorites.find((candidate) => candidate.id === favoriteId) : undefined;
  if (favorite) return createUrlDropSource(favorite);

  const closedTabIndex = getClosedTabIndex(state.draggingClosedTabIndex, getData("text/closed-tab-index"));
  const closedTab = Number.isInteger(closedTabIndex) ? state.closedTabs[closedTabIndex] : undefined;
  if (closedTab) return createUrlDropSource(closedTab);

  return null;
}

function createUrlDropSource(source: Pick<Favorite | ClosedTab, "title" | "url">): SidebarSplitDropSource {
  return {
    title: source.title,
    type: "url",
    url: source.url
  };
}

function getClosedTabIndex(draggingClosedTabIndex: number | null, rawIndex: string) {
  if (draggingClosedTabIndex !== null) return draggingClosedTabIndex;
  if (!rawIndex) return Number.NaN;

  return Number.parseInt(rawIndex, 10);
}
