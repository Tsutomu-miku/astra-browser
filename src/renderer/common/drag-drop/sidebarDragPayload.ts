export const SIDEBAR_TAB_DRAG_TYPE = "application/x-astra-sidebar-tab-id";

export function writeSidebarTabDragPayload(dataTransfer: DataTransfer, tabId: string) {
  dataTransfer.effectAllowed = "move";
  dataTransfer.setData(SIDEBAR_TAB_DRAG_TYPE, tabId);
  dataTransfer.setData("text/plain", tabId);
}

export function readSidebarTabDragPayload(dataTransfer: DataTransfer): string {
  return readSidebarTabDragData((type) => dataTransfer.getData(type));
}

export function readSidebarTabDragData(getData: (type: string) => string): string {
  return getData(SIDEBAR_TAB_DRAG_TYPE) || getData("text/plain");
}
