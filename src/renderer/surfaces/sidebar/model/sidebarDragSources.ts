import { readSidebarTabDragData, readSidebarTabDragPayload } from "../../../common/drag-drop/sidebarDragPayload";

export const SIDEBAR_DRAG_DATA = {
  closedTabIndex: "text/closed-tab-index",
  essentialId: "text/essential-id",
  favoriteId: "text/favorite-id",
  groupId: "text/group-id",
  workspaceId: "text/workspace-id"
} as const;

export interface SidebarDragState {
  draggingClosedTabIndex?: number | null;
  draggingEssentialId?: string | null;
  draggingFavoriteId?: string | null;
  draggingGroupId?: string | null;
  draggingTabId?: string | null;
  draggingWorkspaceId?: string | null;
}

type DragDataReader = (type: string) => string;

export function readSidebarTabDragId(
  state: Pick<SidebarDragState, "draggingTabId">,
  getData: DragDataReader = () => ""
): string {
  return state.draggingTabId || readSidebarTabDragData(getData);
}

export function readSidebarTabDragEventId(
  state: Pick<SidebarDragState, "draggingTabId">,
  dataTransfer: DataTransfer
): string {
  return state.draggingTabId || readSidebarTabDragPayload(dataTransfer);
}

export function readSidebarGroupDragId(
  state: Pick<SidebarDragState, "draggingGroupId">,
  getData: DragDataReader = () => ""
): string {
  return state.draggingGroupId || getData(SIDEBAR_DRAG_DATA.groupId);
}

export function readSidebarFavoriteDragId(
  state: Pick<SidebarDragState, "draggingFavoriteId">,
  getData: DragDataReader = () => ""
): string {
  return state.draggingFavoriteId || getData(SIDEBAR_DRAG_DATA.favoriteId);
}

export function readSidebarEssentialDragId(
  state: Pick<SidebarDragState, "draggingEssentialId">,
  getData: DragDataReader = () => ""
): string {
  return state.draggingEssentialId || getData(SIDEBAR_DRAG_DATA.essentialId);
}

export function readSidebarWorkspaceDragId(
  state: Pick<SidebarDragState, "draggingWorkspaceId">,
  getData: DragDataReader = () => ""
): string {
  return state.draggingWorkspaceId || getData(SIDEBAR_DRAG_DATA.workspaceId);
}

export function readSidebarClosedTabDragIndex(
  state: Pick<SidebarDragState, "draggingClosedTabIndex">,
  getData: DragDataReader = () => ""
): number | null {
  if (state.draggingClosedTabIndex !== null && state.draggingClosedTabIndex !== undefined) {
    return state.draggingClosedTabIndex;
  }

  const rawIndex = getData(SIDEBAR_DRAG_DATA.closedTabIndex);
  if (!rawIndex) return null;

  const index = Number.parseInt(rawIndex, 10);
  return Number.isInteger(index) ? index : null;
}

export function hasNewWorkspaceDragSource(state: SidebarDragState): boolean {
  return Boolean(
    (state.draggingClosedTabIndex !== null && state.draggingClosedTabIndex !== undefined) ||
    state.draggingFavoriteId ||
    state.draggingGroupId ||
    state.draggingTabId
  );
}
