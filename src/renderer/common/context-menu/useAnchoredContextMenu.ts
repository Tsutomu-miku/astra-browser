import { useState, type MouseEvent } from "react";

import { useContextMenuDismissal, type ContextMenuCloseOptions } from "./menuDismissal";
import { getAnchoredContextMenuPosition } from "./menuPosition";

export interface AnchoredContextMenuState<TItem> {
  item: TItem;
  left: number;
  top: number;
}

export function useAnchoredContextMenu<TItem>({
  menuHeight = 188,
  menuWidth = 206
}: {
  menuHeight?: number;
  menuWidth?: number;
} = {}) {
  const [menu, setMenu] = useState<AnchoredContextMenuState<TItem> | null>(null);

  function closeMenu(_options?: ContextMenuCloseOptions) {
    setMenu(null);
  }

  function openMenu(event: MouseEvent, item: TItem) {
    event.preventDefault();
    setMenu({
      item,
      ...getAnchoredContextMenuPosition(event, {
        height: menuHeight,
        width: menuWidth
      })
    });
  }

  useContextMenuDismissal({ isOpen: Boolean(menu), onClose: closeMenu });

  return { closeMenu, menu, openMenu };
}
