import type { DragEvent } from "react";

import {
  clearDropPlacement,
  updateDropPlacement,
  type DropAxis
} from "../../../common/drag-drop/dropPlacement";

type SidebarRowDragIdReader = (event: DragEvent<HTMLElement>) => string | null | undefined;

interface SidebarRowReorderOptions {
  axis?: DropAxis;
  dropEffect?: DataTransfer["dropEffect"];
  readDragId: SidebarRowDragIdReader;
  targetId: string;
}

export function acceptSidebarRowReorderDrag(
  event: DragEvent<HTMLElement>,
  {
    axis = "vertical",
    dropEffect = "move",
    readDragId,
    targetId
  }: SidebarRowReorderOptions
) {
  const draggedId = readDragId(event);
  if (!draggedId || draggedId === targetId) return null;

  event.preventDefault();
  event.dataTransfer.dropEffect = dropEffect;
  updateDropPlacement(event.currentTarget, event, axis);
  return draggedId;
}

export function resolveSidebarRowReorderDrop(
  event: DragEvent<HTMLElement>,
  {
    readDragId,
    targetId
  }: SidebarRowReorderOptions
) {
  clearDropPlacement(event.currentTarget);
  const draggedId = readDragId(event);
  return draggedId && draggedId !== targetId ? draggedId : null;
}

export function clearSidebarRowReorderDrop(event: DragEvent<HTMLElement>) {
  clearDropPlacement(event.currentTarget);
}
