import type { Workspace } from "../../../domain/browser-core";

export interface MoveWorkspaceTarget {
  id: string;
  name: string;
}

export function getMoveWorkspaceTargets(
  workspaces: Pick<Workspace, "id" | "name">[],
  activeWorkspaceId: string
): MoveWorkspaceTarget[] {
  return workspaces
    .filter((workspace) => workspace.id !== activeWorkspaceId)
    .map((workspace) => ({
      id: workspace.id,
      name: workspace.name.trim() || "Space"
    }));
}
