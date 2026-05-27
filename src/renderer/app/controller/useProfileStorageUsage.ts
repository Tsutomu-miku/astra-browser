import { useCallback, useEffect, useMemo, useState } from "react";

import { getWorkspacePartition, type Workspace } from "../../domain/browser";
import type { ProfileStorageUsage } from "../../types/electron";

export interface WorkspaceStorageUsage {
  cacheBytes: number;
  partition: string;
  profileId: string;
  profileName: string;
  storageBytes: number;
  storagePath: string | null;
  totalBytes: number;
  workspaceId: string;
  workspaceName: string;
}

type StorageStatus = "idle" | "loading" | "ready" | "unavailable" | "error";

export function useProfileStorageUsage(workspaces: Workspace[]) {
  const [status, setStatus] = useState<StorageStatus>("idle");
  const [entries, setEntries] = useState<WorkspaceStorageUsage[]>([]);
  const [error, setError] = useState<string | null>(null);
  const partitions = useMemo(() => Array.from(new Set(workspaces.map(getWorkspacePartition))), [workspaces]);

  const refresh = useCallback(async () => {
    if (!window.astraShell?.getProfileStorageUsage) {
      setStatus("unavailable");
      setEntries(createFallbackEntries(workspaces));
      return;
    }

    setStatus("loading");
    setError(null);
    try {
      const usage = await window.astraShell.getProfileStorageUsage(partitions);
      setEntries(createStorageEntries(workspaces, usage));
      setStatus("ready");
    } catch (caught) {
      setError(caught instanceof Error ? caught.message : "Unable to inspect profile storage");
      setStatus("error");
    }
  }, [partitions, workspaces]);

  useEffect(() => {
    void refresh();
  }, [refresh]);

  return { entries, error, refresh, status };
}

export function createStorageEntries(workspaces: Workspace[], usage: ProfileStorageUsage[]): WorkspaceStorageUsage[] {
  const usageByPartition = new Map(usage.map((entry) => [entry.partition, entry]));
  return workspaces.map((workspace) => {
    const partition = getWorkspacePartition(workspace);
    const entry = usageByPartition.get(partition);
    const cacheBytes = entry?.cacheBytes ?? 0;
    const storageBytes = entry?.storageBytes ?? 0;
    return {
      cacheBytes,
      partition,
      profileId: workspace.profileId,
      profileName: workspace.profileName,
      storageBytes,
      storagePath: entry?.storagePath ?? null,
      totalBytes: cacheBytes + storageBytes,
      workspaceId: workspace.id,
      workspaceName: workspace.name
    };
  });
}

export function createFallbackEntries(workspaces: Workspace[]): WorkspaceStorageUsage[] {
  return workspaces.map((workspace) => ({
    cacheBytes: 0,
    partition: getWorkspacePartition(workspace),
    profileId: workspace.profileId,
    profileName: workspace.profileName,
    storageBytes: 0,
    storagePath: null,
    totalBytes: 0,
    workspaceId: workspace.id,
    workspaceName: workspace.name
  }));
}
