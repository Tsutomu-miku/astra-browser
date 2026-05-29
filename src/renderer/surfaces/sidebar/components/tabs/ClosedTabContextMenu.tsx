import type { ClosedTab } from "../../../../domain/browser";
import type { MoveWorkspaceTarget } from "../../model/tabContextMenuState";

export function ClosedTabContextMenu({
  closedIndex,
  left,
  moveWorkspaceTargets,
  onClose,
  onCopyText,
  onOpenInSplit,
  onPreview,
  onRestore,
  onRestoreToNewWorkspace,
  onRestoreToWorkspace,
  tab,
  top
}: {
  closedIndex: number;
  left: number;
  moveWorkspaceTargets?: MoveWorkspaceTarget[];
  onClose: () => void;
  onCopyText: (text: string) => void;
  onOpenInSplit: (url: string, title?: string) => void;
  onPreview: (url: string, title?: string) => void;
  onRestore: (closedIndex: number) => void;
  onRestoreToNewWorkspace?: (closedIndex: number) => void;
  onRestoreToWorkspace?: (closedIndex: number, workspaceId: string) => void;
  tab: ClosedTab;
  top: number;
}) {
  const title = tab.title || tab.url;
  const run = (action: () => void) => {
    action();
    onClose();
  };

  return (
    <div
      className="tab-context-menu closed-tab-context-menu"
      role="menu"
      style={{ left, top }}
      onContextMenu={(event) => event.preventDefault()}
    >
      <button type="button" role="menuitem" onClick={() => run(() => onRestore(closedIndex))}>Restore</button>
      <button type="button" role="menuitem" onClick={() => run(() => onPreview(tab.url, tab.title))}>Preview in Glance</button>
      <button type="button" role="menuitem" onClick={() => run(() => onOpenInSplit(tab.url, tab.title))}>Open in split view</button>
      {(moveWorkspaceTargets?.length || onRestoreToNewWorkspace) && (
        <>
          <span className="tab-context-menu-separator" />
          {moveWorkspaceTargets?.map((workspace) => (
            <button
              key={workspace.id}
              type="button"
              role="menuitem"
              onClick={() => run(() => onRestoreToWorkspace?.(closedIndex, workspace.id))}
            >
              Restore to {workspace.name}
            </button>
          ))}
          {onRestoreToNewWorkspace && (
            <button type="button" role="menuitem" onClick={() => run(() => onRestoreToNewWorkspace(closedIndex))}>
              Restore to New Space
            </button>
          )}
        </>
      )}
      <button type="button" role="menuitem" onClick={() => run(() => onCopyText(tab.url))}>Copy URL</button>
      <button type="button" role="menuitem" onClick={() => run(() => onCopyText(title))}>Copy title</button>
    </div>
  );
}
