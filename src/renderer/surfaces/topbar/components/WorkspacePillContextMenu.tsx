export function WorkspacePillContextMenu({
  canDelete,
  left,
  onClose,
  onDeleteWorkspace,
  onNewWorkspace,
  onOpenSettings,
  top,
  workspaceName
}: {
  canDelete: boolean;
  left: number;
  onClose: () => void;
  onDeleteWorkspace: () => void;
  onNewWorkspace: () => void;
  onOpenSettings: () => void;
  top: number;
  workspaceName: string;
}) {
  const run = (action: () => void) => {
    action();
    onClose();
  };

  return (
    <div
      className="workspace-pill-context-menu"
      role="menu"
      style={{ left, top }}
      onContextMenu={(event) => event.preventDefault()}
    >
      <button type="button" role="menuitem" onClick={() => run(onOpenSettings)}>
        Space settings
      </button>
      <button type="button" role="menuitem" onClick={() => run(onNewWorkspace)}>
        New Space
      </button>
      <span className="workspace-pill-context-menu-separator" />
      <button
        type="button"
        role="menuitem"
        className="danger"
        disabled={!canDelete}
        onClick={() => run(onDeleteWorkspace)}
      >
        Delete {workspaceName}
      </button>
    </div>
  );
}
