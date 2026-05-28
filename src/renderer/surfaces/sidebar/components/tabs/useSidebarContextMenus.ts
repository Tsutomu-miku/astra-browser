import { useEffect, useState, type MouseEvent } from "react";

import type { BrowserTab, ClosedTab, Favorite, TabGroup } from "../../../../domain/browser";

const MENU_EDGE_GAP = 12;

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

export interface TabGroupMenuState {
  groupId: string;
  left: number;
  top: number;
}

export function useSidebarContextMenus() {
  const [tabMenu, setTabMenu] = useState<TabMenuState | null>(null);
  const [quickEntryMenu, setQuickEntryMenu] = useState<QuickEntryMenuState | null>(null);
  const [closedTabMenu, setClosedTabMenu] = useState<ClosedTabMenuState | null>(null);
  const [tabGroupMenu, setTabGroupMenu] = useState<TabGroupMenuState | null>(null);

  function closeMenus() {
    setClosedTabMenu(null);
    setTabGroupMenu(null);
    setTabMenu(null);
    setQuickEntryMenu(null);
  }

  function openTabMenu(event: MouseEvent, tab: BrowserTab) {
    event.preventDefault();
    setClosedTabMenu(null);
    setTabGroupMenu(null);
    setQuickEntryMenu(null);
    setTabMenu({
      ...getMenuPosition(event, 190, 260),
      tab,
    });
  }

  function openQuickEntryMenu(event: MouseEvent, item: Favorite, kind: QuickEntryMenuState["kind"]) {
    event.preventDefault();
    setClosedTabMenu(null);
    setTabGroupMenu(null);
    setTabMenu(null);
    setQuickEntryMenu({
      item,
      kind,
      ...getMenuPosition(event, 206, 188)
    });
  }

  function openClosedTabMenu(event: MouseEvent, tab: ClosedTab, closedIndex: number) {
    event.preventDefault();
    setQuickEntryMenu(null);
    setTabGroupMenu(null);
    setTabMenu(null);
    setClosedTabMenu({
      closedIndex,
      tab,
      ...getMenuPosition(event, 206, 170)
    });
  }

  function openTabGroupMenu(event: MouseEvent, group: TabGroup) {
    event.preventDefault();
    setClosedTabMenu(null);
    setQuickEntryMenu(null);
    setTabMenu(null);
    setTabGroupMenu({
      groupId: group.id,
      ...getMenuPosition(event, 224, 430)
    });
  }

  useEffect(() => {
    if (!closedTabMenu && !tabGroupMenu && !tabMenu && !quickEntryMenu) return;

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
  }, [closedTabMenu, quickEntryMenu, tabGroupMenu, tabMenu]);

  return {
    closedTabMenu,
    closeMenus,
    openClosedTabMenu,
    openQuickEntryMenu,
    openTabGroupMenu,
    openTabMenu,
    quickEntryMenu,
    tabGroupMenu,
    tabMenu
  };
}

function getMenuPosition(event: MouseEvent, width: number, height: number) {
  return {
    left: Math.max(MENU_EDGE_GAP, Math.min(event.clientX, window.innerWidth - width - MENU_EDGE_GAP)),
    top: Math.max(MENU_EDGE_GAP, Math.min(event.clientY, window.innerHeight - height - MENU_EDGE_GAP))
  };
}
