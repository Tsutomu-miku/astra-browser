import type { BrowserState, Workspace } from "../../domain/browser";
import type { StartTileItem } from "./components/StartTileGrid";

const START_PAGE_TILE_LIMIT = 8;
const START_PAGE_HISTORY_LIMIT = 5;

export function getStartPageContent(state: BrowserState, workspace: Workspace) {
  const tabById = new Map(workspace.tabs.map((tab) => [tab.id, tab]));
  const favoriteTabs: StartTileItem[] = workspace.favoriteOrder
    .map((tabId) => tabById.get(tabId))
    .filter((tab): tab is NonNullable<typeof tab> => Boolean(tab && tab.isFavorite))
    .slice(0, START_PAGE_TILE_LIMIT)
    .map((tab) => ({
      id: tab.id,
      tabId: tab.id,
      title: tab.title || tab.url,
      url: tab.url
    }));

  return {
    essentials: state.essentials.slice(0, START_PAGE_TILE_LIMIT),
    favorites: favoriteTabs,
    recentHistory: state.history
      .filter((entry) => entry.workspaceId === workspace.id)
      .slice(0, START_PAGE_HISTORY_LIMIT)
  };
}
