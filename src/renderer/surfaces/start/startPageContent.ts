import type { BrowserState, Workspace } from "../../domain/browser";

const START_PAGE_TILE_LIMIT = 8;
const START_PAGE_HISTORY_LIMIT = 5;

export function getStartPageContent(state: BrowserState, workspace: Workspace) {
  return {
    essentials: state.essentials.slice(0, START_PAGE_TILE_LIMIT),
    favorites: workspace.favorites.slice(0, START_PAGE_TILE_LIMIT),
    recentHistory: state.history
      .filter((entry) => entry.workspaceId === workspace.id)
      .slice(0, START_PAGE_HISTORY_LIMIT)
  };
}
