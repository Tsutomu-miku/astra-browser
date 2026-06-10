import type { BrowserState, Workspace } from "../browser/types";

const DEFAULT_PROFILE_ID = "default";
const PARTITION_PREFIX = "persist:astra-";
/**
 * 无痕/访客模式 partition（Electron 的 in-memory session）。
 * PRD §5 M0 交付 K-12 MVP：不写 cookie、localStorage、cache 到磁盘。
 */
const IN_MEMORY_PARTITION_PREFIX = "in-memory:astra-incognito-";

export type PartitionKind = "persistent" | "incognito";

export function getWorkspacePartition(workspace: Pick<Workspace, "profileId">, kind: PartitionKind = "persistent"): string {
  if (kind === "incognito") {
    return `${IN_MEMORY_PARTITION_PREFIX}${normalizeWorkspaceProfileId(workspace.profileId)}-${Date.now().toString(36)}`;
  }
  return `${PARTITION_PREFIX}${normalizeWorkspaceProfileId(workspace.profileId)}`;
}

export function isIncognitoPartition(partition: string): boolean {
  return partition.startsWith(IN_MEMORY_PARTITION_PREFIX) || partition.startsWith("in-memory:");
}

export function getBrowserPartitions(state: Pick<BrowserState, "workspaces">): string[] {
  return Array.from(new Set(state.workspaces.map((ws) => getWorkspacePartition(ws))));
}

export function getProfileIdForPartition(
  state: Pick<BrowserState, "workspaces">,
  partition: string | undefined
): string | undefined {
  return state.workspaces.find((workspace) => getWorkspacePartition(workspace) === partition)?.profileId;
}

export function normalizeWorkspaceProfile(
  workspace: Partial<Pick<Workspace, "id" | "name" | "profileId" | "profileName">>
): Pick<Workspace, "profileId" | "profileName"> {
  const profileName = normalizeWorkspaceProfileName(workspace.profileName || workspace.name || "Default");
  const profileId = normalizeWorkspaceProfileId(workspace.profileId || workspace.id || profileName);
  return { profileId, profileName };
}

export function normalizeWorkspaceProfileId(value: unknown): string {
  const normalized = String(value ?? "")
    .trim()
    .toLowerCase()
    .replace(/[^a-z0-9_-]+/g, "-")
    .replace(/^-+|-+$/g, "");
  return normalized || DEFAULT_PROFILE_ID;
}

export function normalizeWorkspaceProfileName(value: unknown): string {
  return String(value ?? "").trim() || "Default";
}
