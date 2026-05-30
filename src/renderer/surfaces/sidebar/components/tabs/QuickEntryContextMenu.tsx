import { FiArrowRight, FiColumns, FiEye, FiGrid, FiLink, FiPlus, FiTrash2, FiType } from "react-icons/fi";

import type { Favorite } from "../../../../domain/browser";
import { SidebarMenuItem, SidebarMenuSeparator } from "../common/SidebarMenuItem";
import { SidebarMenuSurface } from "../common/SidebarMenuSurface";
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
  onOpen: (item: Favorite, kind: "essential" | "favorite") => void;
  onOpenInSplit: (item: Favorite, kind: "essential" | "favorite") => void;
  onPreview: (url: string, title?: string) => void;
  onRemove: (url: string) => void;
  top: number;
}) {
  const label = kind === "essential" ? "Essential" : "Favorite";
  const canMoveFavorite = kind === "favorite" && (Boolean(onMoveToNewWorkspace) || Boolean(onMoveToWorkspace));
  const run = (action: () => void) => {
    action();
    onClose();
  };

  return (
    <SidebarMenuSurface className="tab-context-menu quick-entry-context-menu" style={{ left, top }}>
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
          <SidebarMenuSeparator />
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
      <SidebarMenuSeparator />
      <SidebarMenuItem icon={FiTrash2} role="menuitem" className="danger" onClick={() => run(() => onRemove(kind === "favorite" ? item.id : item.url))}>
        Remove {label}
      </SidebarMenuItem>
    </SidebarMenuSurface>
  );
}
