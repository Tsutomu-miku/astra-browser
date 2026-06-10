import {
  BrowserState,
  createTab,
  DownloadEntry,
  getReadableUrlTitle,
  normalizeAddress
} from "../browser";
import { isInternalPageUrl } from "../browser/internalPages";
import { getActiveTab, getActiveWorkspace } from "../browser/selectors";
import { updateBrowserState } from "../browser/updateState";

const MAX_HISTORY_ITEMS = 200;

export function openUrlInActiveWorkspace(state: BrowserState, url: string, title = url): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = getActiveWorkspace(draft);
    const tab = createTab(title, normalizeAddress(url, draft.settings.searchEngine));
    workspace.tabs.push(tab);
    workspace.activeTabId = tab.id;
  });
}

export function navigateActiveTab(state: BrowserState, url: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const normalizedUrl = normalizeAddress(url, draft.settings.searchEngine);
    Object.assign(getActiveTab(getActiveWorkspace(draft)), {
      url: normalizedUrl,
      title: normalizedUrl,
      isSleeping: false
    });
  });
}

export function recordHistory(state: BrowserState, tabId: string, url: string): BrowserState {
  if (!url || url === "about:blank" || url.startsWith("data:") || isInternalPageUrl(url)) return state;
  return updateBrowserState(state, (draft) => {
    const workspace = draft.workspaces.find((candidate) => candidate.tabs.some((tab) => tab.id === tabId));
    const tab = workspace?.tabs.find((candidate) => candidate.id === tabId);
    if (!workspace || !tab) return;
    const title = tab.title && tab.title !== url ? tab.title : getReadableUrlTitle(url);
    const previous = draft.history[0];
    previous?.url === url && previous?.title === title
      ? previous.visitedAt = Date.now()
      : draft.history.unshift({ id: crypto.randomUUID(), title, url, workspaceId: workspace.id, visitedAt: Date.now() });
    draft.history = draft.history.slice(0, MAX_HISTORY_ITEMS);
  });
}

export function clearBrowsingData(state: BrowserState): BrowserState {
  return updateBrowserState(state, (draft) => {
    draft.history = [];
    draft.downloads = [];
    draft.sitePermissions = [];
  });
}

export function clearWorkspaceBrowsingData(state: BrowserState, workspaceId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    const workspace = draft.workspaces.find((candidate) => candidate.id === workspaceId);
    if (!workspace) return;

    draft.history = draft.history.filter((entry) => entry.workspaceId !== workspace.id);
    draft.sitePermissions = draft.sitePermissions.filter((rule) => rule.profileId !== workspace.profileId);
  });
}

export function clearHistory(state: BrowserState): BrowserState {
  return updateBrowserState(state, (draft) => {
    draft.history = [];
  });
}

export function removeHistoryEntry(state: BrowserState, historyId: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    draft.history = draft.history.filter((entry) => entry.id !== historyId);
  });
}

export function upsertDownload(state: BrowserState, download: DownloadEntry): BrowserState {
  return updateBrowserState(state, (draft) => {
    const existingIndex = draft.downloads.findIndex((entry) => entry.id === download.id);
    existingIndex >= 0
      ? draft.downloads[existingIndex] = { ...draft.downloads[existingIndex], ...download }
      : draft.downloads.unshift(download);
    draft.downloads = draft.downloads.slice(0, 80);
  });
}

export function removeDownload(state: BrowserState, id: string): BrowserState {
  return updateBrowserState(state, (draft) => {
    draft.downloads = draft.downloads.filter((entry) => entry.id !== id);
  });
}
