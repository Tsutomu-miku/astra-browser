import type { DragEvent } from "react";

import {
  clearDropPlacement,
  type DropPlacementTarget
} from "../../../common/drag-drop/dropPlacement";
import {
  readSidebarGroupDragId,
  readSidebarTabDragId,
  type SidebarDragState
} from "./sidebarDragSources";
import {
  acceptSidebarRowReorderDrag,
  resolveSidebarRowReorderDrop
} from "./sidebarRowReorderDrop";

export type SidebarTabGroupHeaderDropIntent =
  | { type: "currentGroup"; groupId: string }
  | { type: "group"; groupId: string }
  | { type: "tab"; tabId: string };

export function acceptSidebarTabGroupHeaderDrag(
  event: DragEvent<HTMLElement>,
  state: Pick<SidebarDragState, "draggingGroupId" | "draggingTabId">,
  targetGroupId: string
): SidebarTabGroupHeaderDropIntent | null {
  const groupId = acceptSidebarRowReorderDrag(event, {
    readDragId: (currentEvent) => readGroupDragId(currentEvent, state),
    targetId: targetGroupId
  });
  if (groupId) return { type: "group", groupId };

  const tabId = readTabDragId(event, state);
  if (!tabId) return null;

  event.preventDefault();
  event.dataTransfer.dropEffect = "move";
  event.currentTarget.dataset.dropPlacement = "onto";
  return { type: "tab", tabId };
}

export function resolveSidebarTabGroupHeaderDrop(
  event: DragEvent<HTMLElement>,
  state: Pick<SidebarDragState, "draggingGroupId" | "draggingTabId">,
  targetGroupId: string
): SidebarTabGroupHeaderDropIntent | null {
  const groupId = resolveSidebarRowReorderDrop(event, {
    readDragId: (currentEvent) => readGroupDragId(currentEvent, state),
    targetId: targetGroupId
  });
  if (groupId) return { type: "group", groupId };

  const currentGroupId = readGroupDragId(event, state);
  if (currentGroupId === targetGroupId) {
    event.preventDefault();
    return { type: "currentGroup", groupId: currentGroupId };
  }

  const tabId = readTabDragId(event, state);
  if (!tabId) return null;

  clearDropPlacement(event.currentTarget as DropPlacementTarget);
  event.preventDefault();
  return { type: "tab", tabId };
}

function readGroupDragId(
  event: DragEvent<HTMLElement>,
  state: Pick<SidebarDragState, "draggingGroupId">
) {
  return readSidebarGroupDragId(state, (type) => event.dataTransfer.getData(type));
}

function readTabDragId(
  event: DragEvent<HTMLElement>,
  state: Pick<SidebarDragState, "draggingTabId">
) {
  return readSidebarTabDragId(state, (type) => event.dataTransfer.getData(type));
}
