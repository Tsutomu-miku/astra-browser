import { useEffect, useState, type MouseEvent } from "react";

import type { BrowserTab, ClosedTab, Favorite } from "../../../../domain/browser";

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

export interface ClosedTabMenuState {
  closedIndex: number;
  left: number;
  tab: ClosedTab;
  top: number;
}

export function useSidebarContextMenus() {
  const [tabMenu, setTabMenu] = useState<TabMenuState | null>(null);
  const [quickEntryMenu, setQuickEntryMenu] = useState<QuickEntryMenuState | null>(null);
  const [closedTabMenu, setClosedTabMenu] = useState<ClosedTabMenuState | null>(null);

  function closeMenus() {
    setClosedTabMenu(null);
    setTabMenu(null);
    setQuickEntryMenu(null);
  }

  function openTabMenu(event: MouseEvent, tab: BrowserTab) {
    event.preventDefault();
    setClosedTabMenu(null);
    setQuickEntryMenu(null);
    setTabMenu({
      left: Math.min(event.clientX, window.innerWidth - 190),
      tab,
      top: Math.min(event.clientY, window.innerHeight - 260)
    });
  }

  function openQuickEntryMenu(event: MouseEvent, item: Favorite, kind: QuickEntryMenuState["kind"]) {
    event.preventDefault();
    setClosedTabMenu(null);
    setTabMenu(null);
    setQuickEntryMenu({
      item,
      kind,
      left: Math.min(event.clientX, window.innerWidth - 206),
      top: Math.min(event.clientY, window.innerHeight - 188)
    });
  }

  function openClosedTabMenu(event: MouseEvent, tab: ClosedTab, closedIndex: number) {
    event.preventDefault();
    setQuickEntryMenu(null);
    setTabMenu(null);
    setClosedTabMenu({
      closedIndex,
      left: Math.min(event.clientX, window.innerWidth - 206),
      tab,
      top: Math.min(event.clientY, window.innerHeight - 170)
    });
  }

  useEffect(() => {
    if (!closedTabMenu && !tabMenu && !quickEntryMenu) return;

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
  }, [closedTabMenu, quickEntryMenu, tabMenu]);

  return {
    closedTabMenu,
    closeMenus,
    openClosedTabMenu,
    openQuickEntryMenu,
    openTabMenu,
    quickEntryMenu,
    tabMenu
  };
}
