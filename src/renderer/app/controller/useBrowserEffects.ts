import { useEffect } from "react";

import {
  getBrowserPartitions,
  getWorkspacePartition,
  type DownloadEntry,
  type SitePermissionRule,
  type Workspace
} from "../../domain/browser";
import type { MainProcessAction, PermissionRequestEvent, SavePasswordRequestEvent } from "../../types/electron";
import { resolveShortcut, type ShortcutIntent } from "../../common/shortcuts/keyboardShortcuts";

interface BrowserActions {
  closeActiveTab: () => void;
  findInPage: (query: string, forward?: boolean) => void;
  focusAddressBar: () => void;
  newIncognitoWindow: () => void;
  newGuestWindow: () => void;
  newTab: () => void;
  openUrlInActiveWorkspace: (url: string, title?: string) => void;
  printActiveTab: (options?: Record<string, unknown>) => void | Promise<unknown>;
  resetActiveTabZoom: () => void;
  restoreLastClosedTab: () => void;
  runWebviewAction: (action: "goBack" | "goForward" | "reload" | "reloadIgnoringCache") => void;
  selectAdjacentTab: (direction: 1 | -1) => void;
  setCommandOpen: (open: boolean) => void;
  setFindOpen: (open: boolean) => void;
  setPanel: (panel: "downloads" | "history" | "settings" | null) => void;
  syncForceHttps: (enabled: boolean) => void;
  syncSafeBrowsing: (settings: { enabled: boolean; remoteLookupUrl?: string }) => void;
  toggleActiveDevTools: () => void;
  toggleActiveTabFavorite: () => void;
  toggleActiveTabMuted: () => void;
  toggleActivePictureInPicture: () => void;
  toggleSidebar: () => void;
  zoomIn: () => void;
  zoomOut: () => void;
}

interface BrowserEffectsOptions {
  actions: BrowserActions;
  findQuery: string;
  forceHttps: boolean;
  safeBrowsing: boolean;
  ingestDownload: (download: DownloadEntry) => void;
  ingestPermissionRequest: (request: PermissionRequestEvent) => void;
  ingestPasswordSavePrompt: (prompt: SavePasswordRequestEvent) => void;
  onShortcut: (intent: ShortcutIntent) => void;
  openUrlInNewTab: (url: string) => void;
  sitePermissions: SitePermissionRule[];
  sleepIdleTabs: () => void;
  workspaces: Workspace[];
}

function dispatchMainAction(actions: BrowserActions, findQuery: string, action: MainProcessAction, args: unknown[]) {
  switch (action) {
    case "new-tab":
      actions.newTab();
      break;
    case "close-active-tab":
      actions.closeActiveTab();
      break;
    case "restore-closed-tab":
      actions.restoreLastClosedTab();
      break;
    case "navigate-history":
      actions.runWebviewAction((args[0] as number) < 0 ? "goBack" : "goForward");
      break;
    case "reload-page":
      actions.runWebviewAction(args[0] ? "reloadIgnoringCache" : "reload");
      break;
    case "open-find":
      actions.setFindOpen(true);
      break;
    case "find-match": {
      const direction = args[0] as number;
      if (findQuery.trim()) {
        actions.findInPage(findQuery, direction > 0);
      } else {
        actions.setFindOpen(true);
      }
      break;
    }
    case "open-history":
      actions.setPanel("history");
      break;
    case "open-downloads":
      actions.setPanel("downloads");
      break;
    case "open-settings-panel":
      actions.setPanel("settings");
      break;
    case "print-page":
      actions.printActiveTab();
      break;
    case "open-command":
      actions.setCommandOpen(true);
      break;
    case "focus-address":
      actions.focusAddressBar();
      break;
    case "toggle-sidebar":
      actions.toggleSidebar();
      break;
    case "toggle-active-tab-muted":
      actions.toggleActiveTabMuted();
      break;
    case "toggle-active-tab-favorite":
      actions.toggleActiveTabFavorite();
      break;
    case "select-adjacent-tab":
      actions.selectAdjacentTab(args[0] as 1 | -1);
      break;
    case "zoom-in":
      actions.zoomIn();
      break;
    case "zoom-out":
      actions.zoomOut();
      break;
    case "reset-zoom":
      actions.resetActiveTabZoom();
      break;
    case "toggle-devtools":
      actions.toggleActiveDevTools();
      break;
    case "toggle-picture-in-picture":
      actions.toggleActivePictureInPicture();
      break;
    case "new-incognito-window":
      actions.newIncognitoWindow();
      break;
    case "new-guest-window":
      actions.newGuestWindow();
      break;
    default:
      break;
  }
}

export function useBrowserEffects({
  actions,
  findQuery,
  forceHttps,
  safeBrowsing,
  ingestDownload,
  ingestPermissionRequest,
  ingestPasswordSavePrompt,
  onShortcut,
  openUrlInNewTab,
  sitePermissions,
  sleepIdleTabs,
  workspaces
}: BrowserEffectsOptions) {
  useEffect(() => window.astraShell?.onDownloadEvent((download) => {
    ingestDownload(download);
  }), [ingestDownload]);

  useEffect(() => {
    actions.syncForceHttps(forceHttps);
  }, [actions, forceHttps]);

  useEffect(() => {
    actions.syncSafeBrowsing({ enabled: safeBrowsing });
  }, [actions, safeBrowsing]);

  useEffect(() => window.astraShell?.onOpenUrlInNewTab((url) => {
    openUrlInNewTab(url);
  }), [openUrlInNewTab]);

  // Renderer-side fallback: <webview> DOM "new-window" events (triggered by
  // <a target="_blank"> plain clicks on some Electron builds) dispatch this
  // CustomEvent so they open a new tab the same way main-process popups do.
  useEffect(() => {
    const onOpenUrl = (event: Event) => {
      const url = (event as CustomEvent<string>).detail;
      if (url) openUrlInNewTab(url);
    };
    window.addEventListener("astra:open-url-in-new-tab", onOpenUrl);
    return () => window.removeEventListener("astra:open-url-in-new-tab", onOpenUrl);
  }, [openUrlInNewTab]);

  useEffect(() => window.astraShell?.onMainAction((action, args) => {
    dispatchMainAction(actions, findQuery, action, args);
  }), [actions, findQuery]);

  useEffect(() => window.astraShell?.onPermissionRequest((request) => {
    ingestPermissionRequest(request);
  }), [ingestPermissionRequest]);

  useEffect(() => window.astraShell?.onSavePasswordRequest?.((prompt) => {
    ingestPasswordSavePrompt(prompt);
  }), [ingestPasswordSavePrompt]);

  useEffect(() => {
    window.astraShell?.setProfilePartitions(getBrowserPartitions({ workspaces }));
  }, [workspaces]);

  useEffect(() => {
    window.astraShell?.setPermissionRules(sitePermissions.map(({ profileId, origin, permission, decision }) => ({
      partition: getWorkspacePartition({ profileId }),
      origin,
      permission,
      decision
    })));
  }, [sitePermissions]);

  useEffect(() => {
    const onKeyDown = (event: KeyboardEvent) => {
      const intent = resolveShortcut(event);
      if (!intent) return;

      event.preventDefault();
      onShortcut(intent);
    };

    document.addEventListener("keydown", onKeyDown);
    return () => document.removeEventListener("keydown", onKeyDown);
  }, [onShortcut]);

  useEffect(() => {
    const intervalId = window.setInterval(sleepIdleTabs, 60_000);
    return () => window.clearInterval(intervalId);
  }, [sleepIdleTabs]);
}
