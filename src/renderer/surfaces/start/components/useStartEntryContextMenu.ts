import { useEffect, useState, type MouseEvent } from "react";

import type { Favorite, HistoryEntry } from "../../../domain/browser";

export type StartEntryContextMenuKind = "essential" | "favorite" | "history";
export type StartEntryContextMenuItem = Pick<Favorite | HistoryEntry, "id" | "title" | "url">;

export interface StartEntryContextMenuState {
  item: StartEntryContextMenuItem;
  kind: StartEntryContextMenuKind;
  left: number;
  top: number;
}

export function useStartEntryContextMenu() {
  const [menu, setMenu] = useState<StartEntryContextMenuState | null>(null);

  function closeMenu() {
    setMenu(null);
  }

  function openMenu(event: MouseEvent, item: StartEntryContextMenuItem, kind: StartEntryContextMenuKind) {
    event.preventDefault();
    setMenu({
      item,
      kind,
      left: Math.min(event.clientX, window.innerWidth - 206),
      top: Math.min(event.clientY, window.innerHeight - 188)
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
