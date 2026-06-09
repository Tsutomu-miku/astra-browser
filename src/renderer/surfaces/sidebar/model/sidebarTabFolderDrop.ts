import { clearDropTargetActive, markDropTargetActive } from "../../../common/drag-drop/dropPlacement";
import { readSidebarTabDragId } from "./sidebarDragSources";

export interface SidebarTabFolderDropEvent {
  currentTarget: HTMLElement;
  dataTransfer: {
    dropEffect: string;
    getData: (type: string) => string;
  };
  preventDefault: () => void;
}

export function getSidebarTabFolderDragId(event: SidebarTabFolderDropEvent, draggingTabId: string | null) {
  return readSidebarTabDragId({ draggingTabId }, (type) => event.dataTransfer.getData(type));
}

export function acceptSidebarTabFolderDrag(
  event: SidebarTabFolderDropEvent,
  draggingTabId: string | null,
  dropEffect: DataTransfer["dropEffect"] = "move"
) {
  if (!getSidebarTabFolderDragId(event, draggingTabId)) return false;

  event.preventDefault();
  event.dataTransfer.dropEffect = dropEffect;
  markDropTargetActive(event.currentTarget);
  return true;
}

export function clearSidebarTabFolderDrop(event: { currentTarget: HTMLElement }) {
  clearDropTargetActive(event.currentTarget);
}
