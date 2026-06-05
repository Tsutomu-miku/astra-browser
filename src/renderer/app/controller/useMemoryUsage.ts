import { useCallback, useEffect, useMemo, useRef, useState } from "react";

import {
  buildMemoryBreakdown,
  type MemoryUsageBreakdown
} from "../../common/memory/memoryUsage";
import type { Workspace } from "../../domain/browser";
import type { ProcessMemorySnapshot, ProfileStorageUsage } from "../../types/electron";

export type MemoryUsageStatus = "idle" | "loading" | "ready" | "unavailable" | "error";

const REFRESH_MS = 4000;

function buildFallbackBreakdown(workspaces: Workspace[]): MemoryUsageBreakdown {
  return buildMemoryBreakdown(workspaces, new Map(), null);
}

export function useMemoryUsage(
  workspaces: Workspace[],
  profileStorage: Array<Pick<ProfileStorageUsage, "partition" | "cacheBytes">>
) {
  const [status, setStatus] = useState<MemoryUsageStatus>("idle");
  const [breakdown, setBreakdown] = useState<MemoryUsageBreakdown>(() => buildFallbackBreakdown(workspaces));
  const [error, setError] = useState<string | null>(null);
  const historyRef = useRef<number[]>([]);

  const cacheByPartition = useMemo(() => {
    const map = new Map<string, number>();
    for (const entry of profileStorage) {
      map.set(entry.partition, entry.cacheBytes);
    }
    return map;
  }, [profileStorage]);

  const refresh = useCallback(async () => {
    if (!window.astraShell?.getProcessMemory) {
      setStatus("unavailable");
      setBreakdown(buildMemoryBreakdown(workspaces, cacheByPartition, null));
      return;
    }

    setStatus("loading");
    setError(null);
    try {
      const snapshot: ProcessMemorySnapshot = await window.astraShell.getProcessMemory();
      const next = buildMemoryBreakdown(workspaces, cacheByPartition, snapshot, snapshot.sampledAt);
      historyRef.current = [...historyRef.current.slice(-23), next.totalBytes];
      setBreakdown(next);
      setStatus("ready");
    } catch (caught) {
      setError(caught instanceof Error ? caught.message : "Unable to inspect process memory");
      setBreakdown(buildMemoryBreakdown(workspaces, cacheByPartition, null));
      setStatus("error");
    }
  }, [cacheByPartition, workspaces]);

  useEffect(() => {
    void refresh();
    const timer = window.setInterval(() => {
      void refresh();
    }, REFRESH_MS);
    return () => window.clearInterval(timer);
  }, [refresh]);

  const history = historyRef.current;

  return { breakdown, error, history, refresh, status };
}
