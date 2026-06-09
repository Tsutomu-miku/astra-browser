import type { BrowserTab, ClosedTab, Favorite } from "../../../domain/browser";
import {
  readSidebarClosedTabDragIndex,
  readSidebarEssentialDragId,
  readSidebarTabDragId,
  type SidebarDragState
} from "./sidebarDragSources";

export type SidebarSplitDropSource =
  | { type: "tab"; tabId: string; title: string }
  | { type: "url"; title: string; url: string };

export interface SidebarSplitDropEvent {
  dataTransfer: {
    dropEffect: string;
    getData: (type: string) => string;
  };
  preventDefault: () => void;
}

export interface SidebarSplitDropState extends Required<Pick<
  SidebarDragState,
  "draggingClosedTabIndex" | "draggingEssentialId" | "draggingFavoriteId" | "draggingTabId"
>> {
  activeTabId: string;
  closedTabs: ClosedTab[];
  essentials: Favorite[];
  favoriteTabs: BrowserTab[];
  tabs: BrowserTab[];
}

export function getSidebarSplitDropSource(
  state: SidebarSplitDropState,
  getData: (type: string) => string = () => ""
): SidebarSplitDropSource | null {
  // Workspace favorites now drag with tab identity, so the tab path below
  // covers both regular and favorite tabs.
  const tabId = readSidebarTabDragId(state, getData);
  const tab = tabId ? state.tabs.find((candidate) => candidate.id === tabId) : undefined;
  if (tab && tab.id !== state.activeTabId) {
    return { type: "tab", tabId: tab.id, title: tab.title };
  }

  const essentialId = readSidebarEssentialDragId(state, getData);
  const essential = essentialId ? state.essentials.find((candidate) => candidate.id === essentialId) : undefined;
  if (essential) return createUrlDropSource(essential);

  const closedTabIndex = readSidebarClosedTabDragIndex(state, getData);
  const closedTab = closedTabIndex === null ? undefined : state.closedTabs[closedTabIndex];
  if (closedTab) return createUrlDropSource(closedTab);

  return null;
}

export function acceptSidebarSplitDropTarget(
  event: SidebarSplitDropEvent,
  state: SidebarSplitDropState
): SidebarSplitDropSource | null {
  const source = getSidebarSplitDropSourceFromEvent(event, state);
  if (!source) return null;

  event.preventDefault();
  event.dataTransfer.dropEffect = "move";
  return source;
}

export function resolveSidebarSplitDrop(
  event: SidebarSplitDropEvent,
  state: SidebarSplitDropState
): SidebarSplitDropSource | null {
  const source = getSidebarSplitDropSourceFromEvent(event, state);
  if (!source) return null;

  event.preventDefault();
  return source;
}

function getSidebarSplitDropSourceFromEvent(
  event: SidebarSplitDropEvent,
  state: SidebarSplitDropState
): SidebarSplitDropSource | null {
  return getSidebarSplitDropSource(state, (type) => event.dataTransfer.getData(type));
}

function createUrlDropSource(source: Pick<Favorite | ClosedTab, "title" | "url">): SidebarSplitDropSource {
  return {
    title: source.title,
    type: "url",
    url: source.url
  };
}
