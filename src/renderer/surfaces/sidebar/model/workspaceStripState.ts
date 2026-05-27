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

export function getWorkspaceWheelDirection(deltaX: number, deltaY: number): 1 | -1 | 0 {
  const dominantDelta = Math.abs(deltaY) >= Math.abs(deltaX) ? deltaY : deltaX;
  if (Math.abs(dominantDelta) < 1) return 0;
  return dominantDelta > 0 ? 1 : -1;
}

export function getAdjacentWorkspaceId(
  workspaces: Pick<Workspace, "id">[],
  activeWorkspaceId: string,
  direction: 1 | -1
): string | null {
  if (workspaces.length < 2) return null;

  const activeIndex = workspaces.findIndex((workspace) => workspace.id === activeWorkspaceId);
  const currentIndex = activeIndex < 0 ? 0 : activeIndex;
  return workspaces[(currentIndex + direction + workspaces.length) % workspaces.length].id;
}

function getWorkspaceDisplayName(workspace: Pick<Workspace, "name">): string {
  return workspace.name.trim() || "Space";
}
