import type { BrowserState, BrowserTab, Workspace } from "../../domain/browser";
import type { ProcessMemorySnapshot } from "../../types/electron";

const ACTIVE_TAB_BASE_MB = 48;
const BACKGROUND_TAB_BASE_MB = 18;
const SLEEPING_TAB_BASE_MB = 2;
const MEDIA_TAB_EXTRA_MB = 60;

export interface TabMemoryEstimate {
  estimatedBytes: number;
  tab: BrowserTab;
}

export interface WorkspaceMemoryEstimate {
  cacheBytes: number;
  estimatedBytes: number;
  tabs: TabMemoryEstimate[];
  totalBytes: number;
  workspace: Workspace;
}

export interface MemoryUsageBreakdown {
  browserBytes: number;
  estimatedAt: number;
  perWorkspace: WorkspaceMemoryEstimate[];
  processSnapshot: ProcessMemorySnapshot | null;
  totalBytes: number;
  webviewBytes: number;
}

export function estimateTabMemoryMb(tab: BrowserTab): number {
  if (tab.isSleeping) {
    return SLEEPING_TAB_BASE_MB;
  }

  const base = tab.isLoading ? BACKGROUND_TAB_BASE_MB : ACTIVE_TAB_BASE_MB;
  const mediaOverhead = tab.isMediaPlaying || tab.isCameraOn || tab.isMicrophoneOn ? MEDIA_TAB_EXTRA_MB : 0;
  return base + mediaOverhead;
}

export function buildMemoryBreakdown(
  workspaces: Workspace[],
  profileCacheByPartition: Map<string, number>,
  processSnapshot: ProcessMemorySnapshot | null,
  sampledAt: number = Date.now()
): MemoryUsageBreakdown {
  let webviewBytes = 0;
  const perWorkspace: WorkspaceMemoryEstimate[] = workspaces.map((workspace) => {
    const tabs: TabMemoryEstimate[] = workspace.tabs.map((tab) => {
      const estimatedBytes = estimateTabMemoryMb(tab) * 1024 * 1024;
      return { estimatedBytes, tab };
    });
    const estimatedBytes = tabs.reduce((sum, row) => sum + row.estimatedBytes, 0);
    webviewBytes += estimatedBytes;
    const partition = `persist:${workspace.profileId}`;
    const cacheBytes = profileCacheByPartition.get(partition) ?? 0;
    return {
      cacheBytes,
      estimatedBytes,
      tabs,
      totalBytes: estimatedBytes + cacheBytes,
      workspace
    };
  });

  const measuredTotal = processSnapshot?.totalBytes ?? 0;
  const measuredBrowser = processSnapshot?.appRssBytes ?? 0;
  const browserBytes = measuredBrowser || Math.max(0, measuredTotal - webviewBytes);
  const totalBytes = measuredTotal || browserBytes + webviewBytes;

  return {
    browserBytes,
    estimatedAt: sampledAt,
    perWorkspace,
    processSnapshot,
    totalBytes,
    webviewBytes: processSnapshot?.webviewWorkingSetBytes ?? webviewBytes
  };
}

export function getBrowserStateMemoryBreakdown(
  state: Pick<BrowserState, "workspaces"> & { history?: unknown[]; downloads?: unknown[] },
  profileCacheByPartition: Map<string, number>,
  processSnapshot: ProcessMemorySnapshot | null
): MemoryUsageBreakdown {
  return buildMemoryBreakdown(state.workspaces, profileCacheByPartition, processSnapshot);
}

export function formatBytes(bytes: number): string {
  if (!Number.isFinite(bytes) || bytes <= 0) return "0 B";
  const units = ["B", "KB", "MB", "GB", "TB"];
  const magnitude = Math.min(units.length - 1, Math.floor(Math.log(bytes) / Math.log(1024)));
  const value = bytes / Math.pow(1024, magnitude);
  const fractionDigits = magnitude === 0 ? 0 : magnitude === 1 ? 0 : magnitude >= 3 ? 2 : 1;
  return `${value.toFixed(fractionDigits)} ${units[magnitude]}`;
}
