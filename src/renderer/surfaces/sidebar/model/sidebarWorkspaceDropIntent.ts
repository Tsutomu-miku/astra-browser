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

export interface SidebarWorkspaceDropEvent {
  dataTransfer: {
    dropEffect: string;
    getData: (type: string) => string;
  };
  preventDefault: () => void;
}

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

export function acceptSidebarNewWorkspaceDropTarget(
  event: SidebarWorkspaceDropEvent,
  state: SidebarDragState
): Exclude<SidebarWorkspaceDropIntent, { type: "workspace" }> | null {
  const intent = getSidebarNewWorkspaceDropIntent(state, (type) => event.dataTransfer.getData(type));
  if (!intent) return null;

  event.preventDefault();
  event.dataTransfer.dropEffect = "move";
  return intent;
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
