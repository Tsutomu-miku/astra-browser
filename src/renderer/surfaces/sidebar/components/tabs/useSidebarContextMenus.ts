import { useEffect, useState, type MouseEvent } from "react";

import type { BrowserTab, Favorite } from "../../../../domain/browser";

export interface TabMenuState {
  left: number;
  tab: BrowserTab;
  top: number;
}

export interface QuickEntryMenuState {
  item: Favorite;
  kind: "essential" | "favorite";
  left: number;
  top: number;
}

export function useSidebarContextMenus() {
  const [tabMenu, setTabMenu] = useState<TabMenuState | null>(null);
  const [quickEntryMenu, setQuickEntryMenu] = useState<QuickEntryMenuState | null>(null);

  function closeMenus() {
    setTabMenu(null);
    setQuickEntryMenu(null);
  }

  function openTabMenu(event: MouseEvent, tab: BrowserTab) {
    event.preventDefault();
    setQuickEntryMenu(null);
    setTabMenu({
      left: Math.min(event.clientX, window.innerWidth - 190),
      tab,
      top: Math.min(event.clientY, window.innerHeight - 260)
    });
  }

  function openQuickEntryMenu(event: MouseEvent, item: Favorite, kind: QuickEntryMenuState["kind"]) {
    event.preventDefault();
    setTabMenu(null);
    setQuickEntryMenu({
      item,
      kind,
      left: Math.min(event.clientX, window.innerWidth - 206),
      top: Math.min(event.clientY, window.innerHeight - 188)
    });
  }

  useEffect(() => {
    if (!tabMenu && !quickEntryMenu) return;

    const closeOnEscape = (event: KeyboardEvent) => {
      if (event.key === "Escape") closeMenus();
    };

    window.addEventListener("click", closeMenus);
    window.addEventListener("blur", closeMenus);
    window.addEventListener("keydown", closeOnEscape);
    window.addEventListener("scroll", closeMenus, true);
    return () => {
      window.removeEventListener("click", closeMenus);
      window.removeEventListener("blur", closeMenus);
      window.removeEventListener("keydown", closeOnEscape);
      window.removeEventListener("scroll", closeMenus, true);
    };
  }, [quickEntryMenu, tabMenu]);

  return {
    closeMenus,
    openQuickEntryMenu,
    openTabMenu,
    quickEntryMenu,
    setQuickEntryMenu,
    tabMenu
  };
}
