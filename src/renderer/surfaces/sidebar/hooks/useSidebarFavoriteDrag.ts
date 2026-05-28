import { useState, type DragEvent } from "react";

import type { BrowserController } from "../../../app/controller/types";
import { isFavorite, type Workspace } from "../../../domain/browser";

export function useSidebarFavoriteDrag({
  actions,
  activeWorkspace,
  draggingTabId,
  setDraggingTabId
}: {
  actions: BrowserController["actions"];
  activeWorkspace: Workspace;
  draggingTabId: string | null;
  setDraggingTabId: (tabId: string | null) => void;
}) {
  const [draggingFavoriteId, setDraggingFavoriteId] = useState<string | null>(null);
  const getDroppedTabId = (event: DragEvent<HTMLElement>) => draggingTabId || event.dataTransfer.getData("text/plain");
  const getDroppedFavoriteId = (event: DragEvent<HTMLElement>) =>
    draggingFavoriteId || event.dataTransfer.getData("text/favorite-id");

  const handleFavoriteDrop = (event: DragEvent<HTMLElement>) => {
    if (draggingFavoriteId) {
      setDraggingFavoriteId(null);
      return;
    }

    const tabId = getDroppedTabId(event);
    const tab = activeWorkspace.tabs.find((candidate) => candidate.id === tabId);
    if (!tab) return;

    event.preventDefault();
    if (!isFavorite(activeWorkspace, tab.url)) actions.toggleTabFavorite(tab.id);
    setDraggingTabId(null);
  };

  const handleFavoriteDragStart = (event: DragEvent<HTMLButtonElement>, favoriteId: string) => {
    setDraggingFavoriteId(favoriteId);
    event.dataTransfer.effectAllowed = "move";
    event.dataTransfer.setData("text/favorite-id", favoriteId);
  };

  const handleFavoriteReorderDrop = (event: DragEvent<HTMLElement>, targetFavoriteId: string) => {
    event.preventDefault();
    event.stopPropagation();
    const favoriteId = getDroppedFavoriteId(event);
    if (!favoriteId || favoriteId === targetFavoriteId) {
      setDraggingFavoriteId(null);
      return;
    }

    const rect = event.currentTarget.getBoundingClientRect();
    const placement = event.clientY > rect.top + rect.height / 2 ? "after" : "before";
    actions.reorderWorkspaceFavorite(favoriteId, targetFavoriteId, placement);
    setDraggingFavoriteId(null);
  };

  return {
    draggingFavoriteId,
    handleFavoriteDragStart,
    handleFavoriteDrop,
    handleFavoriteReorderDrop,
    setDraggingFavoriteId
  };
}
