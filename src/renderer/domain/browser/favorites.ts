import type { BrowserTab, Favorite, Workspace } from "./types";

export function resolveFavoriteTab(
  workspace: Pick<Workspace, "tabs"> | undefined,
  favorite: Pick<Favorite, "tabId" | "url">
): BrowserTab | undefined {
  if (!workspace) return undefined;

  return (
    favorite.tabId
      ? workspace.tabs.find((tab) => tab.id === favorite.tabId)
      : undefined
  ) ?? workspace.tabs.find((tab) => tab.url === favorite.url);
}
