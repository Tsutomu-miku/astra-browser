import { useRef } from "react";
import { FiArrowRight, FiColumns, FiEye, FiGrid, FiLink, FiPlus, FiTrash2, FiType } from "react-icons/fi";

import { handleMenuKeyboardNavigation, useMenuInitialFocus } from "../../../../common/context-menu/menuKeyboardNavigation";
import type { Favorite } from "../../../../domain/browser";
import type { MoveWorkspaceTarget } from "../../model/tabContextMenuState";
import { SidebarMenuItem } from "./SidebarMenuItem";

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
  onOpen: (item: Favorite, kind: "essential" | "favorite") => void;
  onOpenInSplit: (item: Favorite, kind: "essential" | "favorite") => void;
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
      <SidebarMenuItem icon={FiArrowRight} role="menuitem" onClick={() => run(() => onOpen(item, kind))}>
        Open
      </SidebarMenuItem>
      <SidebarMenuItem icon={FiEye} role="menuitem" onClick={() => run(() => onPreview(item.url, item.title))}>
        Preview in Glance
      </SidebarMenuItem>
      <SidebarMenuItem icon={FiColumns} role="menuitem" onClick={() => run(() => onOpenInSplit(item, kind))}>
        Open in split view
      </SidebarMenuItem>
      {canMoveFavorite && (
        <>
          <span className="tab-context-menu-separator" />
          {moveWorkspaceTargets?.map((workspace) => (
            <SidebarMenuItem
              key={workspace.id}
              icon={FiGrid}
              role="menuitem"
              onClick={() => run(() => onMoveToWorkspace?.(item.id, workspace.id))}
            >
              Move to {workspace.name}
            </SidebarMenuItem>
          ))}
          <SidebarMenuItem icon={FiPlus} role="menuitem" onClick={() => run(() => onMoveToNewWorkspace?.(item.id))}>
            Move to New Space
          </SidebarMenuItem>
        </>
      )}
      <SidebarMenuItem icon={FiLink} role="menuitem" onClick={() => run(() => onCopyText(item.url))}>
        Copy URL
      </SidebarMenuItem>
      <SidebarMenuItem icon={FiType} role="menuitem" onClick={() => run(() => onCopyText(item.title || item.url))}>
        Copy title
      </SidebarMenuItem>
      <span className="tab-context-menu-separator" />
      <SidebarMenuItem icon={FiTrash2} role="menuitem" className="danger" onClick={() => run(() => onRemove(kind === "favorite" ? item.id : item.url))}>
        Remove {label}
      </SidebarMenuItem>
    </div>
  );
}
