import type { DragEvent } from "react";

import {
  clearDropPlacement,
  updateDropPlacement,
  updateDropZone,
  type DropAxis,
  type DropZonePlacement
} from "../../../common/drag-drop/dropPlacement";

export type SidebarRowDropResult = {
  draggedId: string;
  placement: DropZonePlacement;
};

type SidebarRowDragIdReader = (event: DragEvent<HTMLElement>) => string | null | undefined;

interface SidebarRowReorderOptions {
  axis?: DropAxis;
  crossFolder?: boolean | ((draggedId: string) => boolean);
  dropEffect?: DataTransfer["dropEffect"];
  ontoRatio?: number;
  readDragId: SidebarRowDragIdReader;
  targetId: string;
  targetKind?: "group" | "row";
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

export function acceptSidebarTabRowDrag(
  event: DragEvent<HTMLElement>,
  {
    axis = "vertical",
    crossFolder = false,
    dropEffect = "move",
    ontoRatio = 0.33,
    readDragId,
    targetId
  }: SidebarRowReorderOptions
): SidebarRowDropResult | null {
  const draggedId = readDragId(event);
  if (!draggedId || draggedId === targetId) return null;

  event.preventDefault();
  event.dataTransfer.dropEffect = dropEffect;
  const isCross = typeof crossFolder === "function" ? crossFolder(draggedId) : crossFolder;
  const placement = updateDropZone(event.currentTarget, event, axis, ontoRatio, isCross);
  return { draggedId, placement };
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

export function resolveSidebarTabRowDrop(
  event: DragEvent<HTMLElement>,
  {
    axis = "vertical",
    ontoRatio = 0.33,
    readDragId,
    targetId
  }: SidebarRowReorderOptions
): SidebarRowDropResult | null {
  const target = event.currentTarget;
  const draggedId = readDragId(event);
  const savedPlacement = target.dataset.dropPlacement as DropZonePlacement | undefined;
  clearDropPlacement(target);
  if (!draggedId || draggedId === targetId) return null;
  // 保守地把未知 placement 当作 after（纯位置移动），避免意外 group。
  const placement: DropZonePlacement = savedPlacement === "before" || savedPlacement === "onto" || savedPlacement === "after"
    ? savedPlacement
    : "after";
  return { draggedId, placement };
}

export function clearSidebarRowReorderDrop(event: DragEvent<HTMLElement>) {
  clearDropPlacement(event.currentTarget);
}
