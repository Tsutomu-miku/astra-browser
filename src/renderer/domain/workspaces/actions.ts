import {
  createTab,
  getReadableUrlTitle,
  getHomepageUrl,
  getWorkspaceHomepageUrl,
  getNextWorkspaceAccent,
  type BrowserState,
  type BrowserTab,
  type Favorite,
  type SplitLayout,
  type TabGroup,
  type Workspace
} from "../browser";
import { getActiveWorkspace } from "../browser/selectors";
import { takeFavoriteBackingTab, takeTabFavorite } from "../common/favoriteTabs";
import { pruneEmptyTabGroups } from "../tabs/groups";
import { clearSplitView } from "../tabs/splitView";
import { updateBrowserState } from "../browser/updateState";
import { normalizeWorkspaceProfile } from "./profiles";

export type WorkspaceDropPlacement = "before" | "after";

export function switchWorkspace(state: BrowserState, workspaceId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    draft.activeWorkspaceId = workspaceId;
    clearSplitView(draft);
  });
}

export function addWorkspace(state: BrowserState): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = createWorkspace(draft, {
      name: `Space ${draft.workspaces.length + 1}`,
      tabs: [createTab("New Tab", getHomepageUrl(draft))]
    });
    draft.workspaces.push(workspace);
    draft.activeWorkspaceId = draft.workspaces.at(-1)!.id;
  });
}

export function moveTabToNewWorkspace(state: BrowserState, tabId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const source = draft.workspaces.find((workspace) => workspace.tabs.some((tab) => tab.id === tabId));
    if (!source) return;

    const index = source.tabs.findIndex((tab) => tab.id === tabId);
    const [tab] = source.tabs.splice(index, 1);
    tab.groupId = null;
    pruneEmptyTabGroups(source);
    replaceEmptyOrMovedActiveTab(draft, source, tabId, index);

    const favorite = takeTabFavorite(source, tab);
    const workspace = createWorkspace(draft, {
      favorites: favorite ? [favorite] : undefined,
      name: tab.title || getReadableUrlTitle(tab.url),
      tabs: [tab],
      activeTabId: tab.id
    });
    draft.workspaces.push(workspace);
    draft.activeWorkspaceId = workspace.id;
    clearSplitView(draft);
  });
}

export function moveTabGroupToNewWorkspace(state: BrowserState, groupId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const source = draft.workspaces.find((workspace) => workspace.tabGroups.some((group) => group.id === groupId));
    if (!source) return;

    const group = source.tabGroups.find((candidate) => candidate.id === groupId);
    const movingTabs = source.tabs.filter((tab) => tab.groupId === groupId);
    if (!group || movingTabs.length === 0) return;

    const firstMovedIndex = source.tabs.findIndex((tab) => tab.groupId === groupId);
    const movingIds = new Set(movingTabs.map((tab) => tab.id));
    const activeMovedTab = movingTabs.find((tab) => tab.id === source.activeTabId);
    source.tabs = source.tabs.filter((tab) => !movingIds.has(tab.id));
    pruneEmptyTabGroups(source);
    replaceEmptyOrMovedActiveTab(draft, source, activeMovedTab?.id ?? movingTabs[0].id, firstMovedIndex);

    const workspace = createWorkspace(draft, {
      name: group.name,
      tabGroups: [{ ...group }],
      tabs: movingTabs,
      activeTabId: activeMovedTab?.id ?? movingTabs[0].id
    });
    draft.workspaces.push(workspace);
    draft.activeWorkspaceId = workspace.id;
    clearSplitView(draft);
  });
}

export function restoreClosedTabToNewWorkspace(state: BrowserState, closedIndex: number): BrowserState {
  const source = getActiveWorkspace(state);
  if (!Number.isInteger(closedIndex) || closedIndex < 0 || !source.closedTabs[closedIndex]) {
    return state;
  }

  return updateBrowserState(state, (draft) => {
    const source = getActiveWorkspace(draft);
    const [closed] = source.closedTabs.splice(closedIndex, 1);
    if (!closed) return;

    const tab = {
      ...createTab(closed.title, closed.url),
      ...(closed.faviconUrl ? { faviconUrl: closed.faviconUrl } : {})
    };
    const workspace = createWorkspace(draft, {
      name: closed.title || getReadableUrlTitle(closed.url),
      tabs: [tab],
      activeTabId: tab.id
    });
    draft.workspaces.push(workspace);
    draft.activeWorkspaceId = workspace.id;
    clearSplitView(draft);
  });
}

export function moveWorkspaceFavoriteToNewWorkspace(state: BrowserState, favoriteId: string): BrowserState {
  const source = getActiveWorkspace(state);
  if (!source.favorites.some((favorite) => favorite.id === favoriteId)) {
    return state;
  }

  return updateBrowserState(state, (draft) => {
    const source = getActiveWorkspace(draft);
    const index = source.favorites.findIndex((favorite) => favorite.id === favoriteId);
    if (index < 0) return;

    const [favorite] = source.favorites.splice(index, 1);
    const tab = takeFavoriteBackingTab(draft, source, favorite);
    const workspace = createWorkspace(draft, {
      favorites: [favorite],
      name: favorite.title || getReadableUrlTitle(favorite.url),
      tabs: [tab],
      activeTabId: tab.id
    });
    draft.workspaces.push(workspace);
    draft.activeWorkspaceId = workspace.id;
    clearSplitView(draft);
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
  return updateWorkspaceById(state, state.activeWorkspaceId, patch);
}

export function updateWorkspaceById(
  state: BrowserState,
  workspaceId: string,
  patch: Partial<Pick<Workspace, "name" | "accent" | "homepage" | "profileName">>
): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = draft.workspaces.find((candidate) => candidate.id === workspaceId);
    if (!workspace) return;

    Object.assign(workspace, patch);
    if (patch.homepage !== undefined) {
      workspace.homepage = getWorkspaceHomepageUrl(draft, workspace);
    }
  });
}

function createWorkspace(
  state: BrowserState,
  options: {
    activeTabId?: string;
    favorites?: Favorite[];
    name: string;
    tabGroups?: TabGroup[];
    tabs: BrowserTab[];
  }
): Workspace {
  const index = state.workspaces.length + 1;
  const id = crypto.randomUUID();
  const name = options.name.trim() || `Space ${index}`;
  const tabs = options.tabs.length > 0 ? options.tabs : [createTab("New Tab", getHomepageUrl(state))];

  return {
    id,
    name,
    accent: getNextWorkspaceAccent(index),
    homepage: getHomepageUrl(state),
    ...normalizeWorkspaceProfile({ id, name }),
    splitLayout: "horizontal" as SplitLayout,
    closedTabs: [],
    favorites: options.favorites ?? [],
    tabGroups: options.tabGroups ?? [],
    tabs,
    activeTabId: options.activeTabId ?? tabs[0].id
  };
}

function replaceEmptyOrMovedActiveTab(
  state: BrowserState,
  workspace: Workspace,
  movedTabId: string,
  movedIndex: number
) {
  if (workspace.tabs.length === 0) {
    const replacement = createTab("New Tab", getWorkspaceHomepageUrl(state, workspace));
    workspace.tabs.push(replacement);
    workspace.activeTabId = replacement.id;
  } else if (workspace.activeTabId === movedTabId || !workspace.tabs.some((tab) => tab.id === workspace.activeTabId)) {
    workspace.activeTabId = workspace.tabs[Math.min(movedIndex, workspace.tabs.length - 1)].id;
  }
}
