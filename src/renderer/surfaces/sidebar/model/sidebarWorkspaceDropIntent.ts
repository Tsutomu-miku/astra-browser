import {
  readSidebarClosedTabDragIndex,
  readSidebarFavoriteDragId,
  readSidebarGroupDragId,
  readSidebarTabDragId,
  readSidebarWorkspaceDragId,
  type SidebarDragState
} from "./sidebarDragSources";

export type SidebarWorkspaceDropIntent =
  | { type: "closedTab"; closedTabIndex: number }
  | { type: "favorite"; favoriteId: string }
  | { type: "group"; groupId: string }
  | { type: "tab"; tabId: string }
  | { type: "workspace"; workspaceId: string };

export function getSidebarWorkspaceDropIntent(
  state: SidebarDragState & {
    activeWorkspaceId: string;
    targetWorkspaceId: string;
  },
  getData: (type: string) => string = () => ""
): SidebarWorkspaceDropIntent | null {
  const workspaceId = readSidebarWorkspaceDragId(state, getData);
  if (workspaceId && workspaceId !== state.targetWorkspaceId) {
    return { type: "workspace", workspaceId };
  }

  const closedTabIndex = readSidebarClosedTabDragIndex(state, getData);
  if (closedTabIndex !== null) {
    return { type: "closedTab", closedTabIndex };
  }

  const isOtherWorkspace = state.targetWorkspaceId !== state.activeWorkspaceId;
  if (!isOtherWorkspace) return null;

  const favoriteId = readSidebarFavoriteDragId(state, getData);
  if (favoriteId) return { type: "favorite", favoriteId };

  const groupId = readSidebarGroupDragId(state, getData);
  if (groupId) return { type: "group", groupId };

  const tabId = readSidebarTabDragId(state, getData);
  if (tabId) return { type: "tab", tabId };

  return null;
}

export function getSidebarNewWorkspaceDropIntent(
  state: SidebarDragState,
  getData: (type: string) => string = () => ""
): Exclude<SidebarWorkspaceDropIntent, { type: "workspace" }> | null {
  const closedTabIndex = readSidebarClosedTabDragIndex(state, getData);
  if (closedTabIndex !== null) {
    return { type: "closedTab", closedTabIndex };
  }

  const favoriteId = readSidebarFavoriteDragId(state, getData);
  if (favoriteId) return { type: "favorite", favoriteId };

  const groupId = readSidebarGroupDragId(state, getData);
  if (groupId) return { type: "group", groupId };

  const tabId = readSidebarTabDragId(state, getData);
  if (tabId) return { type: "tab", tabId };

  return null;
}
