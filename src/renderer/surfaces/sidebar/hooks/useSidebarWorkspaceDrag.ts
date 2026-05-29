import { useState, type DragEvent } from "react";

import { SIDEBAR_DRAG_DATA } from "../model/sidebarDragSources";

export function useSidebarWorkspaceDrag() {
  const [draggingGroupId, setDraggingGroupId] = useState<string | null>(null);
  const [draggingClosedTabIndex, setDraggingClosedTabIndex] = useState<number | null>(null);
  const [draggingTabId, setDraggingTabId] = useState<string | null>(null);
  const [draggingWorkspaceId, setDraggingWorkspaceId] = useState<string | null>(null);

  const handleWorkspaceDragStart = (event: DragEvent<HTMLButtonElement>, workspaceId: string) => {
    setDraggingClosedTabIndex(null);
    setDraggingGroupId(null);
    setDraggingTabId(null);
    setDraggingWorkspaceId(workspaceId);
    event.dataTransfer.effectAllowed = "move";
    event.dataTransfer.setData(SIDEBAR_DRAG_DATA.workspaceId, workspaceId);
  };

  const clearWorkspaceDrag = () => {
    setDraggingClosedTabIndex(null);
    setDraggingGroupId(null);
    setDraggingWorkspaceId(null);
    setDraggingTabId(null);
  };

  return {
    clearWorkspaceDrag,
    draggingClosedTabIndex,
    draggingGroupId,
    draggingTabId,
    draggingWorkspaceId,
    handleWorkspaceDragStart,
    setDraggingClosedTabIndex,
    setDraggingGroupId,
    setDraggingWorkspaceId,
    setDraggingTabId
  };
}
