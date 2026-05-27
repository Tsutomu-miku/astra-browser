import { useEffect } from "react";

import {
  getBrowserPartitions,
  getWorkspacePartition,
  type DownloadEntry,
  type SitePermissionRule,
  type Workspace
} from "../domain/browser-core";
import type { PermissionRequestEvent } from "../types/electron";
import { resolveShortcut, type ShortcutIntent } from "../common/shortcuts/keyboardShortcuts";

interface BrowserEffectsOptions {
  ingestDownload: (download: DownloadEntry) => void;
  ingestPermissionRequest: (request: PermissionRequestEvent) => void;
  onShortcut: (intent: ShortcutIntent) => void;
  sitePermissions: SitePermissionRule[];
  workspaces: Workspace[];
}

export function useBrowserEffects({
  ingestDownload,
  ingestPermissionRequest,
  onShortcut,
  sitePermissions,
  workspaces
}: BrowserEffectsOptions) {
  useEffect(() => window.astraShell?.onDownloadEvent((download) => {
    ingestDownload(download);
  }), [ingestDownload]);

  useEffect(() => window.astraShell?.onPermissionRequest((request) => {
    ingestPermissionRequest(request);
  }), [ingestPermissionRequest]);

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
}
