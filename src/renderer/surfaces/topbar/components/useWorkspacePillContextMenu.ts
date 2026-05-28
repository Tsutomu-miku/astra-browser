import type { MouseEvent } from "react";

import { useAnchoredContextMenu } from "../../../common/context-menu/useAnchoredContextMenu";

export interface WorkspacePillContextMenuItem {
  id: string;
  name: string;
}

export function useWorkspacePillContextMenu() {
  const { closeMenu, menu, openMenu } = useAnchoredContextMenu<WorkspacePillContextMenuItem>({ menuHeight: 152 });

  function openWorkspacePillMenu(event: MouseEvent, item: WorkspacePillContextMenuItem) {
    openMenu(event, item);
  }

  return { closeMenu, menu, openWorkspacePillMenu };
}
