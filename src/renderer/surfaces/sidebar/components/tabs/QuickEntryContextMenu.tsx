import { useRef } from "react";

import { handleMenuKeyboardNavigation, useMenuInitialFocus } from "../../../../common/context-menu/menuKeyboardNavigation";
import type { Favorite } from "../../../../domain/browser";
import type { MoveWorkspaceTarget } from "../../model/tabContextMenuState";

export function QuickEntryContextMenu({
  item,
  kind,
  left,
  moveWorkspaceTargets,
  onClose,
  onCopyText,
  onMoveToNewWorkspace,
  onMoveToWorkspace,
  onOpen,
  onOpenInSplit,
  onPreview,
  onRemove,
  top
}: {
  item: Favorite;
  kind: "essential" | "favorite";
  left: number;
  moveWorkspaceTargets?: MoveWorkspaceTarget[];
  onClose: () => void;
  onCopyText: (text: string) => void;
  onMoveToNewWorkspace?: (favoriteId: string) => void;
  onMoveToWorkspace?: (favoriteId: string, workspaceId: string) => void;
  onOpen: (url: string, title?: string) => void;
  onOpenInSplit: (url: string, title?: string) => void;
  onPreview: (url: string, title?: string) => void;
  onRemove: (url: string) => void;
  top: number;
}) {
  const menuRef = useRef<HTMLDivElement | null>(null);
  useMenuInitialFocus(menuRef);
  const label = kind === "essential" ? "Essential" : "Favorite";
  const canMoveFavorite = kind === "favorite" && (Boolean(onMoveToNewWorkspace) || Boolean(onMoveToWorkspace));
  const run = (action: () => void) => {
    action();
    onClose();
  };

  return (
    <div
      ref={menuRef}
      className="tab-context-menu quick-entry-context-menu"
      role="menu"
      style={{ left, top }}
      onContextMenu={(event) => event.preventDefault()}
      onKeyDown={handleMenuKeyboardNavigation}
    >
      <button type="button" role="menuitem" onClick={() => run(() => onOpen(item.url, item.title))}>
        Open
      </button>
      <button type="button" role="menuitem" onClick={() => run(() => onPreview(item.url, item.title))}>
        Preview in Glance
      </button>
      <button type="button" role="menuitem" onClick={() => run(() => onOpenInSplit(item.url, item.title))}>
        Open in split view
      </button>
      {canMoveFavorite && (
        <>
          <span className="tab-context-menu-separator" />
          {moveWorkspaceTargets?.map((workspace) => (
            <button
              key={workspace.id}
              type="button"
              role="menuitem"
              onClick={() => run(() => onMoveToWorkspace?.(item.id, workspace.id))}
            >
              Move to {workspace.name}
            </button>
          ))}
          <button type="button" role="menuitem" onClick={() => run(() => onMoveToNewWorkspace?.(item.id))}>
            Move to New Space
          </button>
        </>
      )}
      <button type="button" role="menuitem" onClick={() => run(() => onCopyText(item.url))}>
        Copy URL
      </button>
      <button type="button" role="menuitem" onClick={() => run(() => onCopyText(item.title || item.url))}>
        Copy title
      </button>
      <span className="tab-context-menu-separator" />
      <button type="button" role="menuitem" className="danger" onClick={() => run(() => onRemove(item.url))}>
        Remove {label}
      </button>
    </div>
  );
}
