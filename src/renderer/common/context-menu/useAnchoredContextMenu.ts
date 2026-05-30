import { useEffect, useState, type MouseEvent } from "react";

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

  function closeMenu() {
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

  useEffect(() => {
    if (!menu) return;

    const closeOnEscape = (event: KeyboardEvent) => {
      if (event.key === "Escape") closeMenu();
    };

    window.addEventListener("click", closeMenu);
    window.addEventListener("blur", closeMenu);
    window.addEventListener("keydown", closeOnEscape);
    window.addEventListener("scroll", closeMenu, true);
    return () => {
      window.removeEventListener("click", closeMenu);
      window.removeEventListener("blur", closeMenu);
      window.removeEventListener("keydown", closeOnEscape);
      window.removeEventListener("scroll", closeMenu, true);
    };
  }, [menu]);

  return { closeMenu, menu, openMenu };
}
