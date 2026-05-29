import type { DragEvent } from "react";

import { readSidebarTabDragEventId } from "./sidebarDragSources";

export function getSidebarTabFolderDragId(event: DragEvent<HTMLElement>, draggingTabId: string | null) {
  return readSidebarTabDragEventId({ draggingTabId }, event.dataTransfer);
}

export function acceptSidebarTabFolderDrag(event: DragEvent<HTMLElement>, draggingTabId: string | null, dropEffect: DataTransfer["dropEffect"] = "move") {
  if (!getSidebarTabFolderDragId(event, draggingTabId)) return false;

  event.preventDefault();
  event.dataTransfer.dropEffect = dropEffect;
  return true;
}
