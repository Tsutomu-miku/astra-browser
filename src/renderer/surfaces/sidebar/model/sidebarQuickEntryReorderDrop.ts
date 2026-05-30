import type { DragEvent } from "react";

import { getPointerDropPlacement, type DropAxis, type DropPlacement } from "../../../common/drag-drop/dropPlacement";
import {
  readSidebarEssentialDragId,
  readSidebarFavoriteDragId,
  type SidebarDragState
} from "./sidebarDragSources";

export type SidebarQuickEntryReorderKind = "essential" | "favorite";

export interface SidebarQuickEntryReorderDropIntent {
  placement: DropPlacement;
  quickEntryId: string;
  targetQuickEntryId: string;
}

export function resolveSidebarQuickEntryReorderDrop(
  event: DragEvent<HTMLElement>,
  state: Pick<SidebarDragState, "draggingEssentialId" | "draggingFavoriteId">,
  {
    axis = "vertical",
    kind,
    targetQuickEntryId
  }: {
    axis?: DropAxis;
    kind: SidebarQuickEntryReorderKind;
    targetQuickEntryId: string;
  }
): SidebarQuickEntryReorderDropIntent | null {
  event.preventDefault();
  event.stopPropagation();

  const quickEntryId = readQuickEntryDragId(event, state, kind);
  if (!quickEntryId || quickEntryId === targetQuickEntryId) return null;

  return {
    placement: getPointerDropPlacement(event.currentTarget, event, axis),
    quickEntryId,
    targetQuickEntryId
  };
}

function readQuickEntryDragId(
  event: DragEvent<HTMLElement>,
  state: Pick<SidebarDragState, "draggingEssentialId" | "draggingFavoriteId">,
  kind: SidebarQuickEntryReorderKind
) {
  const readData = (type: string) => event.dataTransfer.getData(type);
  return kind === "essential"
    ? readSidebarEssentialDragId(state, readData)
    : readSidebarFavoriteDragId(state, readData);
}
