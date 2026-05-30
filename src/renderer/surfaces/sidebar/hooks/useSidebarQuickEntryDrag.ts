import { useState, type DragEvent } from "react";

import { type DropAxis } from "../../../common/drag-drop/dropPlacement";
import type { BrowserController } from "../../../app/controller/types";
import { isEssential, type BrowserState, type Workspace } from "../../../domain/browser";
import {
  SIDEBAR_DRAG_DATA,
  readSidebarTabDragEventId
} from "../model/sidebarDragSources";
import { resolveSidebarQuickEntryReorderDrop } from "../model/sidebarQuickEntryReorderDrop";

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
    const intent = resolveSidebarQuickEntryReorderDrop(event, { draggingEssentialId }, {
      axis,
      kind: "essential",
      targetQuickEntryId: targetEssentialId
    });
    if (intent) actions.reorderEssential(intent.quickEntryId, intent.targetQuickEntryId, intent.placement);
    setDraggingEssentialId(null);
  };

  const handleFavoriteReorderDrop = (event: DragEvent<HTMLElement>, targetFavoriteId: string, axis: DropAxis = "vertical") => {
    const intent = resolveSidebarQuickEntryReorderDrop(event, { draggingFavoriteId }, {
      axis,
      kind: "favorite",
      targetQuickEntryId: targetFavoriteId
    });
    if (intent) actions.reorderWorkspaceFavorite(intent.quickEntryId, intent.targetQuickEntryId, intent.placement);
    setDraggingFavoriteId(null);
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
