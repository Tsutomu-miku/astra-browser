import type { MouseEvent } from "react";

import { useAnchoredContextMenu } from "../../../common/context-menu/useAnchoredContextMenu";
import type { Favorite, HistoryEntry } from "../../../domain/browser";

export type StartEntryContextMenuKind = "essential" | "favorite" | "history";
export type StartEntryContextMenuItem = Pick<Favorite | HistoryEntry, "id" | "title" | "url"> & {
  tabId?: string;
};

export interface StartEntryContextMenuState {
  item: StartEntryContextMenuItem;
  kind: StartEntryContextMenuKind;
}

export function useStartEntryContextMenu() {
  const contextMenu = useAnchoredContextMenu<StartEntryContextMenuState>();

  function openMenu(event: MouseEvent, item: StartEntryContextMenuItem, kind: StartEntryContextMenuKind) {
    contextMenu.openMenu(event, { item, kind });
  }

  const menu = contextMenu.menu && {
    item: contextMenu.menu.item.item,
    kind: contextMenu.menu.item.kind,
    left: contextMenu.menu.left,
    top: contextMenu.menu.top
  };

  return { closeMenu: contextMenu.closeMenu, menu, openMenu };
}
