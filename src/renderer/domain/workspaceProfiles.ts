import type { BrowserState, Workspace } from "./browser-types";

const DEFAULT_PROFILE_ID = "default";
const PARTITION_PREFIX = "persist:astra-";

export function getWorkspacePartition(workspace: Pick<Workspace, "profileId">): string {
  return `${PARTITION_PREFIX}${normalizeWorkspaceProfileId(workspace.profileId)}`;
}

export function getBrowserPartitions(state: Pick<BrowserState, "workspaces">): string[] {
  return Array.from(new Set(state.workspaces.map(getWorkspacePartition)));
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
