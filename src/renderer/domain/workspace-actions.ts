import {
  BrowserState,
  createTab,
  getHomepageUrl,
  getWorkspaceHomepageUrl,
  getNextWorkspaceAccent,
  Workspace
} from "./browser-core";
import { getActiveWorkspace } from "./selectors";
import { clearSplitView } from "./split-view";
import { updateBrowserState } from "./action-core";
import { normalizeWorkspaceProfile } from "./workspaceProfiles";

export type WorkspaceDropPlacement = "before" | "after";

export function switchWorkspace(state: BrowserState, workspaceId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    draft.activeWorkspaceId = workspaceId;
    clearSplitView(draft);
  });
}

export function addWorkspace(state: BrowserState): BrowserState {
  return updateBrowserState(state, (draft) => {
    const index = draft.workspaces.length + 1;
    const id = crypto.randomUUID();
    const name = `Space ${index}`;
    const tab = createTab("New Tab", getHomepageUrl(draft));
    draft.workspaces.push({
      id,
      name,
      accent: getNextWorkspaceAccent(index),
      homepage: getHomepageUrl(draft),
      ...normalizeWorkspaceProfile({ id, name }),
      closedTabs: [],
      favorites: [],
      tabGroups: [],
      tabs: [tab],
      activeTabId: tab.id
    });
    draft.activeWorkspaceId = draft.workspaces.at(-1)!.id;
  });
}

export function deleteWorkspace(state: BrowserState, workspaceId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    if (draft.workspaces.length <= 1) return;

    const index = draft.workspaces.findIndex((workspace) => workspace.id === workspaceId);
    if (index < 0) return;

    const deletingActive = draft.activeWorkspaceId === workspaceId;
    draft.workspaces.splice(index, 1);
    if (deletingActive) {
      draft.activeWorkspaceId = draft.workspaces[Math.max(0, index - 1)].id;
      clearSplitView(draft);
    }
  });
}

export function reorderWorkspace(
  state: BrowserState,
  workspaceId: string,
  targetWorkspaceId: string,
  placement: WorkspaceDropPlacement
): BrowserState {
  if (workspaceId === targetWorkspaceId) return state;

  return updateBrowserState(state, (draft) => {
    const fromIndex = draft.workspaces.findIndex((workspace) => workspace.id === workspaceId);
    const targetIndex = draft.workspaces.findIndex((workspace) => workspace.id === targetWorkspaceId);
    if (fromIndex < 0 || targetIndex < 0) return;

    const [workspace] = draft.workspaces.splice(fromIndex, 1);
    const droppedOnIndex = draft.workspaces.findIndex((candidate) => candidate.id === targetWorkspaceId);
    const insertIndex = placement === "after" ? droppedOnIndex + 1 : droppedOnIndex;
    draft.workspaces.splice(insertIndex, 0, workspace);
  });
}

export function updateWorkspace(
  state: BrowserState,
  patch: Partial<Pick<Workspace, "name" | "accent" | "homepage" | "profileName">>
): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    Object.assign(workspace, patch);
    if (patch.homepage !== undefined) {
      workspace.homepage = getWorkspaceHomepageUrl(draft, workspace);
    }
  });
}
