import { useCallback, type DragEvent } from "react";

import { getPointerDropPlacement, type DropAxis } from "../../../common/drag-drop/dropPlacement";
import type { BrowserController } from "../../../app/controller/types";
import type { TabFolder } from "../../../domain/tabs";
import {
  readSidebarClosedTabDragIndex,
  readSidebarEssentialDragId,
  readSidebarFavoriteDragId,
  readSidebarGroupDragId,
  readSidebarTabDragEventId,
  type SidebarDragState
} from "../model/sidebarDragSources";
import type { SidebarWorkspaceDropIntent } from "../model/sidebarWorkspaceDropIntent";
import {
  getSidebarNewWorkspaceDropIntent,
  getSidebarWorkspaceDropIntent
} from "../model/sidebarWorkspaceDropIntent";
import { useSidebarWorkspaceDrag } from "./useSidebarWorkspaceDrag";
import { useSidebarQuickEntryDrag } from "./useSidebarQuickEntryDrag";

type DragState = Required<SidebarDragState> & { draggingEssentialId: string | null; draggingFavoriteId: string | null };

/** Centralises all sidebar drop-handling callbacks so the root Sidebar
 * component does not grow past the 300-line architectural cap. */
export function useSidebarDropHandlers(controller: BrowserController) {
  const {
    activeWorkspace,
    actions,
    state
  } = controller;
  const {
    clearWorkspaceDrag,
    draggingClosedTabIndex,
    draggingGroupId,
    draggingTabId,
    draggingWorkspaceId,
    handleWorkspaceDragStart,
    setDraggingClosedTabIndex,
    setDraggingGroupId,
    setDraggingTabId,
    setDraggingWorkspaceId
  } = useSidebarWorkspaceDrag();
  const {
    draggingEssentialId,
    draggingFavoriteId,
    handleEssentialDragStart,
    handleEssentialDrop,
    handleEssentialReorderDrop,
    handleFavoriteDragStart,
    handleFavoriteReorderDrop,
    setDraggingEssentialId,
    setDraggingFavoriteId
  } = useSidebarQuickEntryDrag({ actions, activeWorkspace, draggingTabId, setDraggingTabId, state });

  const getDroppedTabId = useCallback((event: DragEvent<HTMLElement>) => (
    readSidebarTabDragEventId({ draggingTabId }, event.dataTransfer)
  ), [draggingTabId]);

  const placeTab = useCallback((tabId: string, targetTabId: string, placement: "before" | "after") => {
    actions.moveTabToFolderPosition(tabId, targetTabId, placement);
  }, [actions]);

  const clearSidebarDropState = useCallback(() => {
    setDraggingClosedTabIndex(null);
    setDraggingEssentialId(null);
    setDraggingFavoriteId(null);
    setDraggingGroupId(null);
    setDraggingTabId(null);
    setDraggingWorkspaceId(null);
  }, [setDraggingClosedTabIndex, setDraggingEssentialId, setDraggingFavoriteId, setDraggingGroupId, setDraggingTabId, setDraggingWorkspaceId]);

  const handleTabDrop = useCallback((event: DragEvent<HTMLElement>, targetTabId: string, axis: DropAxis = "vertical") => {
    event.preventDefault();
    event.stopPropagation();
    const tabId = getDroppedTabId(event);
    if (!tabId || tabId === targetTabId) {
      setDraggingTabId(null);
      return;
    }
    const placement = getPointerDropPlacement(event.currentTarget, event, axis);
    placeTab(tabId, targetTabId, placement);
    setDraggingTabId(null);
    setDraggingFavoriteId(null);
  }, [getDroppedTabId, placeTab, setDraggingFavoriteId, setDraggingTabId]);

  const handleTabGroupCreate = useCallback((sourceTabId: string, targetTabId: string) => {
    actions.groupTabsTogether(sourceTabId, targetTabId);
    setDraggingTabId(null);
    setDraggingFavoriteId(null);
  }, [actions, setDraggingFavoriteId, setDraggingTabId]);

  const handleFavoriteTabDrop = useCallback((event: DragEvent<HTMLElement>, targetTabId: string, axis: DropAxis = "vertical") => {
    event.preventDefault();
    event.stopPropagation();
    const tabId = getDroppedTabId(event);
    if (!tabId || tabId === targetTabId) {
      setDraggingTabId(null);
      setDraggingFavoriteId(null);
      return;
    }
    const placement = getPointerDropPlacement(event.currentTarget, event, axis);
    actions.moveTabToFavoritePosition(tabId, targetTabId, placement);
    setDraggingTabId(null);
    setDraggingFavoriteId(null);
  }, [actions, getDroppedTabId, setDraggingFavoriteId, setDraggingTabId]);

  const handleTabFolderDrop = useCallback((event: DragEvent<HTMLElement>, folder: TabFolder) => {
    const tabId = getDroppedTabId(event);
    if (!tabId) return;
    event.preventDefault();
    event.stopPropagation();
    actions.moveTabToFolderEnd(tabId, folder);
    setDraggingTabId(null);
    setDraggingFavoriteId(null);
  }, [actions, getDroppedTabId, setDraggingFavoriteId, setDraggingTabId]);

  const handleFavoritesDrop = useCallback((event: DragEvent<HTMLElement>) => {
    handleTabFolderDrop(event, { type: "favorites" });
  }, [handleTabFolderDrop]);

  const handleTabsDrop = useCallback((event: DragEvent<HTMLElement>) => {
    handleTabFolderDrop(event, { type: "tabs" });
  }, [handleTabFolderDrop]);

  const hasScrollableSidebarDrag = useCallback((event: DragEvent<HTMLElement>) => {
    const readDragData = (type: string) => event.dataTransfer.getData(type);
    return Boolean(
      readSidebarTabDragEventId({ draggingTabId }, event.dataTransfer) ||
      readSidebarFavoriteDragId({ draggingFavoriteId }, readDragData) ||
      readSidebarEssentialDragId({ draggingEssentialId }, readDragData) ||
      readSidebarGroupDragId({ draggingGroupId }, readDragData) ||
      readSidebarClosedTabDragIndex({ draggingClosedTabIndex }, readDragData) !== null
    );
  }, [draggingClosedTabIndex, draggingEssentialId, draggingFavoriteId, draggingGroupId, draggingTabId]);

  const getWorkspaceDragState = useCallback((): DragState => ({
    draggingClosedTabIndex,
    draggingEssentialId,
    draggingFavoriteId,
    draggingGroupId,
    draggingTabId,
    draggingWorkspaceId
  }), [draggingClosedTabIndex, draggingEssentialId, draggingFavoriteId, draggingGroupId, draggingTabId, draggingWorkspaceId]);

  const runWorkspaceDropIntent = useCallback((
    intent: SidebarWorkspaceDropIntent,
    event: DragEvent<HTMLButtonElement>,
    workspaceId: string
  ) => {
    if (intent.type === "workspace") {
      actions.reorderWorkspace(intent.workspaceId, workspaceId, getPointerDropPlacement(event.currentTarget, event, "vertical"));
    } else if (intent.type === "closedTab") {
      actions.restoreClosedTabToWorkspace(intent.closedTabIndex, workspaceId);
    } else if (intent.type === "group") {
      actions.moveTabGroupToWorkspace(intent.groupId, workspaceId);
    } else {
      actions.moveTabToWorkspace(intent.tabId, workspaceId);
    }
  }, [actions]);

  const runNewWorkspaceDropIntent = useCallback((intent: Exclude<SidebarWorkspaceDropIntent, { type: "workspace" }>) => {
    if (intent.type === "closedTab") {
      actions.restoreClosedTabToNewWorkspace(intent.closedTabIndex);
    } else if (intent.type === "group") {
      actions.moveTabGroupToNewWorkspace(intent.groupId);
    } else {
      actions.moveTabToNewWorkspace(intent.tabId);
    }
  }, [actions]);

  const handleWorkspaceDragOver = useCallback((event: DragEvent<HTMLButtonElement>, workspaceId: string) => {
    const intent = getSidebarWorkspaceDropIntent({
      ...getWorkspaceDragState(),
      activeWorkspaceId: activeWorkspace.id,
      targetWorkspaceId: workspaceId
    }, (type) => event.dataTransfer.getData(type));
    if (intent) {
      event.preventDefault();
      event.dataTransfer.dropEffect = "move";
    }
  }, [activeWorkspace.id, getWorkspaceDragState]);

  const handleWorkspaceDrop = useCallback((event: DragEvent<HTMLButtonElement>, workspaceId: string) => {
    const intent = getSidebarWorkspaceDropIntent({
      ...getWorkspaceDragState(),
      activeWorkspaceId: activeWorkspace.id,
      targetWorkspaceId: workspaceId
    }, (type) => event.dataTransfer.getData(type));
    if (!intent) return;
    event.preventDefault();
    runWorkspaceDropIntent(intent, event, workspaceId);
    clearSidebarDropState();
  }, [activeWorkspace.id, clearSidebarDropState, getWorkspaceDragState, runWorkspaceDropIntent]);

  const handleNewWorkspaceDrop = useCallback((event: DragEvent<HTMLButtonElement>) => {
    const intent = getSidebarNewWorkspaceDropIntent(
      getWorkspaceDragState(),
      (type) => event.dataTransfer.getData(type)
    );
    if (!intent) return;
    event.preventDefault();
    runNewWorkspaceDropIntent(intent);
    clearSidebarDropState();
  }, [clearSidebarDropState, getWorkspaceDragState, runNewWorkspaceDropIntent]);

  return {
    draggingClosedTabIndex,
    draggingEssentialId,
    draggingFavoriteId,
    draggingGroupId,
    draggingTabId,
    draggingWorkspaceId,
    handleEssentialDragStart,
    handleEssentialDrop,
    handleEssentialReorderDrop,
    handleFavoriteDragStart,
    handleFavoriteReorderDrop,
    handleFavoriteTabDrop,
    handleFavoritesDrop,
    handleNewWorkspaceDrop,
    handleTabDrop,
    handleTabGroupCreate,
    handleTabsDrop,
    handleWorkspaceDragOver,
    handleWorkspaceDragStart,
    handleWorkspaceDrop,
    hasScrollableSidebarDrag,
    setDraggingClosedTabIndex,
    setDraggingEssentialId,
    setDraggingFavoriteId,
    setDraggingGroupId,
    setDraggingTabId,
    setDraggingWorkspaceId,
    clearWorkspaceDrag
  };
}
