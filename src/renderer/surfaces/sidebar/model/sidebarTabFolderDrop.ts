import type { DragEvent } from "react";

import { readSidebarTabDragPayload } from "../../../common/drag-drop/sidebarDragPayload";

export function getSidebarTabFolderDragId(event: DragEvent<HTMLElement>, draggingTabId: string | null) {
  return draggingTabId || readSidebarTabDragPayload(event.dataTransfer);
}

export function acceptSidebarTabFolderDrag(event: DragEvent<HTMLElement>, draggingTabId: string | null, dropEffect: DataTransfer["dropEffect"] = "move") {
  if (!getSidebarTabFolderDragId(event, draggingTabId)) return false;

  event.preventDefault();
  event.dataTransfer.dropEffect = dropEffect;
  return true;
}
