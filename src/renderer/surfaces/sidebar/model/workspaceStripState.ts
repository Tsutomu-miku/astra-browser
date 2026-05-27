import type { Workspace } from "../../../domain/browser-core";

export function getWorkspaceInitial(workspace: Pick<Workspace, "name">): string {
  return getWorkspaceDisplayName(workspace).slice(0, 1).toUpperCase();
}

export function getWorkspaceTabCount(workspace: Pick<Workspace, "tabs">): number {
  return workspace.tabs.length;
}

export function getWorkspaceButtonLabel(workspace: Pick<Workspace, "name" | "tabs">): string {
  const tabCount = getWorkspaceTabCount(workspace);
  return `${getWorkspaceDisplayName(workspace)}, ${tabCount} ${tabCount === 1 ? "tab" : "tabs"}`;
}

function getWorkspaceDisplayName(workspace: Pick<Workspace, "name">): string {
  return workspace.name.trim() || "Space";
}
