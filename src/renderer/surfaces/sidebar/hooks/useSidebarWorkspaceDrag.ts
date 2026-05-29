import { useState, type DragEvent } from "react";

import { getPointerDropPlacement } from "../../../common/drag-drop/dropPlacement";
import type { BrowserController } from "../../../app/controller/types";
import {
  SIDEBAR_DRAG_DATA,
  readSidebarClosedTabDragIndex,
  readSidebarGroupDragId,
  readSidebarTabDragEventId,
  readSidebarWorkspaceDragId
} from "../model/sidebarDragSources";

type SidebarWorkspaceDragActions = Pick<
  BrowserController["actions"],
  | "moveTabGroupToNewWorkspace"
  | "moveTabGroupToWorkspace"
  | "moveTabToNewWorkspace"
  | "moveTabToWorkspace"
  | "reorderWorkspace"
  | "restoreClosedTabToNewWorkspace"
  | "restoreClosedTabToWorkspace"
>;

export function useSidebarWorkspaceDrag({
  actions,
  activeWorkspaceId
}: {
  actions: SidebarWorkspaceDragActions;
  activeWorkspaceId: string;
}) {
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

  const handleWorkspaceDragOver = (event: DragEvent<HTMLButtonElement>, workspaceId: string) => {
    const getData = (type: string) => event.dataTransfer.getData(type);
    const tabId = readSidebarTabDragEventId({ draggingTabId }, event.dataTransfer);
    const groupId = readSidebarGroupDragId({ draggingGroupId }, getData);
    const closedTabIndex = readSidebarClosedTabDragIndex({ draggingClosedTabIndex }, getData);
    const workspaceDragId = readSidebarWorkspaceDragId({ draggingWorkspaceId }, getData);
    const isClosedTabTarget = closedTabIndex !== null;
    const isGroupTarget = groupId && workspaceId !== activeWorkspaceId;
    const isTabTarget = tabId && workspaceId !== activeWorkspaceId;
    const isWorkspaceTarget = workspaceDragId && workspaceId !== workspaceDragId;
    if (isClosedTabTarget || isGroupTarget || isTabTarget || isWorkspaceTarget) {
      event.preventDefault();
      event.dataTransfer.dropEffect = "move";
    }
  };

  const handleWorkspaceDrop = (event: DragEvent<HTMLButtonElement>, workspaceId: string) => {
    event.preventDefault();
    const droppedWorkspaceId = readSidebarWorkspaceDragId({ draggingWorkspaceId }, (type) => event.dataTransfer.getData(type));
    if (droppedWorkspaceId && droppedWorkspaceId !== workspaceId) {
      const placement = getPointerDropPlacement(event.currentTarget, event, "vertical");
      actions.reorderWorkspace(droppedWorkspaceId, workspaceId, placement);
      setDraggingWorkspaceId(null);
      return;
    }

    const closedTabIndex = readSidebarClosedTabDragIndex({ draggingClosedTabIndex }, (type) => event.dataTransfer.getData(type));
    if (closedTabIndex !== null) {
      actions.restoreClosedTabToWorkspace(closedTabIndex, workspaceId);
      setDraggingClosedTabIndex(null);
      return;
    }

    const groupId = readSidebarGroupDragId({ draggingGroupId }, (type) => event.dataTransfer.getData(type));
    if (groupId && workspaceId !== activeWorkspaceId) {
      actions.moveTabGroupToWorkspace(groupId, workspaceId);
      setDraggingGroupId(null);
      return;
    }

    const tabId = readSidebarTabDragEventId({ draggingTabId }, event.dataTransfer);
    if (tabId && workspaceId !== activeWorkspaceId) {
      actions.moveTabToWorkspace(tabId, workspaceId);
    }
    setDraggingGroupId(null);
    setDraggingTabId(null);
  };

  const handleNewWorkspaceDrop = (event: DragEvent<HTMLButtonElement>) => {
    const getData = (type: string) => event.dataTransfer.getData(type);
    const closedTabIndex = readSidebarClosedTabDragIndex({ draggingClosedTabIndex }, getData);
    const groupId = readSidebarGroupDragId({ draggingGroupId }, getData);
    const tabId = readSidebarTabDragEventId({ draggingTabId }, event.dataTransfer);
    if (closedTabIndex === null && !groupId && !tabId) return;

    event.preventDefault();
    if (closedTabIndex !== null) {
      actions.restoreClosedTabToNewWorkspace(closedTabIndex);
    } else if (groupId) {
      actions.moveTabGroupToNewWorkspace(groupId);
    } else if (tabId) {
      actions.moveTabToNewWorkspace(tabId);
    }
    setDraggingClosedTabIndex(null);
    setDraggingGroupId(null);
    setDraggingTabId(null);
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
    handleNewWorkspaceDrop,
    handleWorkspaceDragOver,
    handleWorkspaceDragStart,
    handleWorkspaceDrop,
    setDraggingClosedTabIndex,
    setDraggingGroupId,
    setDraggingTabId
  };
}
