import { useState, type DragEvent } from "react";

import { getPointerDropPlacement, type DropAxis } from "../../../common/drag-drop/dropPlacement";
import type { BrowserController } from "../../../app/controller/types";
import { isEssential, type BrowserState, type Workspace } from "../../../domain/browser";
import {
  SIDEBAR_DRAG_DATA,
  readSidebarEssentialDragId,
  readSidebarFavoriteDragId,
  readSidebarTabDragEventId
} from "../model/sidebarDragSources";

export function useSidebarQuickEntryDrag({
  actions,
  activeWorkspace,
  draggingTabId,
  setDraggingTabId,
  state
}: {
  actions: BrowserController["actions"];
  activeWorkspace: Workspace;
  draggingTabId: string | null;
  setDraggingTabId: (tabId: string | null) => void;
  state: BrowserState;
}) {
  const [draggingEssentialId, setDraggingEssentialId] = useState<string | null>(null);
  const [draggingFavoriteId, setDraggingFavoriteId] = useState<string | null>(null);
  const getDroppedTabId = (event: DragEvent<HTMLElement>) => readSidebarTabDragEventId({ draggingTabId }, event.dataTransfer);

  const handleEssentialDrop = (event: DragEvent<HTMLElement>) => {
    if (draggingEssentialId) {
      setDraggingEssentialId(null);
      return;
    }

    const tab = getDroppedTab(event);
    if (!tab) return;

    event.preventDefault();
    if (!isEssential(state, tab.url)) actions.toggleTabEssential(tab.id);
    setDraggingTabId(null);
  };

  const handleEssentialDragStart = (event: DragEvent<HTMLElement>, essentialId: string) => {
    setDraggingEssentialId(essentialId);
    event.dataTransfer.effectAllowed = "move";
    event.dataTransfer.setData(SIDEBAR_DRAG_DATA.essentialId, essentialId);
  };

  const handleFavoriteDragStart = (event: DragEvent<HTMLElement>, favoriteId: string) => {
    setDraggingFavoriteId(favoriteId);
    event.dataTransfer.effectAllowed = "move";
    event.dataTransfer.setData(SIDEBAR_DRAG_DATA.favoriteId, favoriteId);
  };

  const handleEssentialReorderDrop = (event: DragEvent<HTMLElement>, targetEssentialId: string, axis: DropAxis = "vertical") => {
    const essentialId = readSidebarEssentialDragId({ draggingEssentialId }, (type) => event.dataTransfer.getData(type));
    runQuickEntryReorder(event, essentialId, targetEssentialId, axis, actions.reorderEssential, setDraggingEssentialId);
  };

  const handleFavoriteReorderDrop = (event: DragEvent<HTMLElement>, targetFavoriteId: string, axis: DropAxis = "vertical") => {
    const favoriteId = readSidebarFavoriteDragId({ draggingFavoriteId }, (type) => event.dataTransfer.getData(type));
    runQuickEntryReorder(event, favoriteId, targetFavoriteId, axis, actions.reorderWorkspaceFavorite, setDraggingFavoriteId);
  };

  function getDroppedTab(event: DragEvent<HTMLElement>) {
    const tabId = getDroppedTabId(event);
    return activeWorkspace.tabs.find((candidate) => candidate.id === tabId);
  }

  return {
    draggingEssentialId,
    draggingFavoriteId,
    handleEssentialDragStart,
    handleEssentialDrop,
    handleEssentialReorderDrop,
    handleFavoriteDragStart,
    handleFavoriteReorderDrop,
    setDraggingEssentialId,
    setDraggingFavoriteId
  };
}

function runQuickEntryReorder(
  event: DragEvent<HTMLElement>,
  quickEntryId: string,
  targetQuickEntryId: string,
  axis: DropAxis,
  reorder: (quickEntryId: string, targetQuickEntryId: string, placement: "before" | "after") => void,
  clearDraggingId: (quickEntryId: string | null) => void
) {
  event.preventDefault();
  event.stopPropagation();
  if (!quickEntryId || quickEntryId === targetQuickEntryId) {
    clearDraggingId(null);
    return;
  }

  const placement = getPointerDropPlacement(event.currentTarget, event, axis);
  reorder(quickEntryId, targetQuickEntryId, placement);
  clearDraggingId(null);
}
