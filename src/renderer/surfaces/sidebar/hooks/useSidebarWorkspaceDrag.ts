import { useState, type DragEvent } from "react";

import type { BrowserController } from "../../../app/controller/types";

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
    event.dataTransfer.setData("text/workspace-id", workspaceId);
  };

  const handleWorkspaceDragOver = (event: DragEvent<HTMLButtonElement>, workspaceId: string) => {
    const isClosedTabTarget = draggingClosedTabIndex !== null;
    const isGroupTarget = draggingGroupId && workspaceId !== activeWorkspaceId;
    const isTabTarget = draggingTabId && workspaceId !== activeWorkspaceId;
    const isWorkspaceTarget = draggingWorkspaceId && workspaceId !== draggingWorkspaceId;
    if (isClosedTabTarget || isGroupTarget || isTabTarget || isWorkspaceTarget) {
      event.preventDefault();
      event.dataTransfer.dropEffect = "move";
    }
  };

  const handleWorkspaceDrop = (event: DragEvent<HTMLButtonElement>, workspaceId: string) => {
    event.preventDefault();
    if (draggingWorkspaceId && draggingWorkspaceId !== workspaceId) {
      const rect = event.currentTarget.getBoundingClientRect();
      const placement = event.clientY > rect.top + rect.height / 2 ? "after" : "before";
      actions.reorderWorkspace(draggingWorkspaceId, workspaceId, placement);
      setDraggingWorkspaceId(null);
      return;
    }

    const closedTabIndex = draggingClosedTabIndex ?? Number.parseInt(event.dataTransfer.getData("text/closed-tab-index"), 10);
    if (Number.isInteger(closedTabIndex)) {
      actions.restoreClosedTabToWorkspace(closedTabIndex, workspaceId);
      setDraggingClosedTabIndex(null);
      return;
    }

    const groupId = draggingGroupId || event.dataTransfer.getData("text/group-id");
    if (groupId && workspaceId !== activeWorkspaceId) {
      actions.moveTabGroupToWorkspace(groupId, workspaceId);
      setDraggingGroupId(null);
      return;
    }

    const tabId = draggingTabId || event.dataTransfer.getData("text/plain");
    if (tabId && workspaceId !== activeWorkspaceId) {
      actions.moveTabToWorkspace(tabId, workspaceId);
    }
    setDraggingGroupId(null);
    setDraggingTabId(null);
  };

  const handleNewWorkspaceDrop = (event: DragEvent<HTMLButtonElement>) => {
    const closedTabIndex = draggingClosedTabIndex ?? Number.parseInt(event.dataTransfer.getData("text/closed-tab-index"), 10);
    const groupId = draggingGroupId || event.dataTransfer.getData("text/group-id");
    const tabId = draggingTabId || event.dataTransfer.getData("text/plain");
    if (!Number.isInteger(closedTabIndex) && !groupId && !tabId) return;

    event.preventDefault();
    if (Number.isInteger(closedTabIndex)) {
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
