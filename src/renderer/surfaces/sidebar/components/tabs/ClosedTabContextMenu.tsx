import { FiArrowRight, FiColumns, FiEye, FiGrid, FiLink, FiPlus, FiType } from "react-icons/fi";

import type { ClosedTab } from "../../../../domain/browser";
import { SidebarMenuItem, SidebarMenuSeparator } from "../common/SidebarMenuItem";
import { SidebarMenuSurface } from "../common/SidebarMenuSurface";
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
    <SidebarMenuSurface className="tab-context-menu closed-tab-context-menu" style={{ left, top }}>
      <SidebarMenuItem icon={FiArrowRight} role="menuitem" onClick={() => run(() => onRestore(closedIndex))}>Restore</SidebarMenuItem>
      <SidebarMenuItem icon={FiEye} role="menuitem" onClick={() => run(() => onPreview(tab.url, tab.title))}>Preview in Glance</SidebarMenuItem>
      <SidebarMenuItem icon={FiColumns} role="menuitem" onClick={() => run(() => onOpenInSplit(tab.url, tab.title))}>Open in split view</SidebarMenuItem>
      {(moveWorkspaceTargets?.length || onRestoreToNewWorkspace) && (
        <>
          <SidebarMenuSeparator />
          {moveWorkspaceTargets?.map((workspace) => (
            <SidebarMenuItem
              key={workspace.id}
              icon={FiGrid}
              role="menuitem"
              onClick={() => run(() => onRestoreToWorkspace?.(closedIndex, workspace.id))}
            >
              Restore to {workspace.name}
            </SidebarMenuItem>
          ))}
          {onRestoreToNewWorkspace && (
            <SidebarMenuItem icon={FiPlus} role="menuitem" onClick={() => run(() => onRestoreToNewWorkspace(closedIndex))}>
              Restore to New Space
            </SidebarMenuItem>
          )}
        </>
      )}
      <SidebarMenuItem icon={FiLink} role="menuitem" onClick={() => run(() => onCopyText(tab.url))}>Copy URL</SidebarMenuItem>
      <SidebarMenuItem icon={FiType} role="menuitem" onClick={() => run(() => onCopyText(title))}>Copy title</SidebarMenuItem>
    </SidebarMenuSurface>
  );
}
