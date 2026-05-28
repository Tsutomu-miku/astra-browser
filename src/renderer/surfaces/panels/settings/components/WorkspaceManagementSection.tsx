export function WorkspaceManagementSection({
  activeWorkspaceId,
  onAddWorkspace,
  onDeleteWorkspace,
  workspaceCount
}: {
  activeWorkspaceId: string;
  onAddWorkspace: () => void;
  onDeleteWorkspace: (workspaceId: string) => void;
  workspaceCount: number;
}) {
  return (
    <section className="settings-pane" aria-label="Workspace management">
      <section className="settings-section">
        <div className="section-copy">
          <span>Workspaces</span>
          <span>{workspaceCount} spaces</span>
        </div>
        <div className="button-cluster">
          <button className="toolbar-button" type="button" onClick={onAddWorkspace}>New</button>
          <button
            className="toolbar-button"
            type="button"
            disabled={workspaceCount <= 1}
            onClick={() => onDeleteWorkspace(activeWorkspaceId)}
          >
            Delete
          </button>
        </div>
      </section>
    </section>
  );
}
