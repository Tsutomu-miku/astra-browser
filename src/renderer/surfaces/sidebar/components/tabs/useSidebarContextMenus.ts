import { useEffect, useRef, useState, type MouseEvent } from "react";

import { getAnchoredContextMenuPosition } from "../../../../common/context-menu/menuPosition";
import type { BrowserTab, ClosedTab, Favorite, TabGroup } from "../../../../domain/browser";

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
  const triggerRef = useRef<HTMLElement | null>(null);

  function closeMenus({ restoreFocus = true }: { restoreFocus?: boolean } = {}) {
    setClosedTabMenu(null);
    setTabGroupMenu(null);
    setTabMenu(null);
    setQuickEntryMenu(null);
    if (restoreFocus && triggerRef.current?.isConnected) {
      triggerRef.current.focus();
    }
    triggerRef.current = null;
  }

  function openTabMenu(event: MouseEvent, tab: BrowserTab) {
    event.preventDefault();
    triggerRef.current = event.currentTarget instanceof HTMLElement ? event.currentTarget : null;
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
    triggerRef.current = event.currentTarget instanceof HTMLElement ? event.currentTarget : null;
    setClosedTabMenu(null);
    setTabGroupMenu(null);
    setTabMenu(null);
    setQuickEntryMenu({
      item,
      kind,
      ...getMenuPosition(event, 206, kind === "favorite" ? 272 : 188)
    });
  }

  function openClosedTabMenu(event: MouseEvent, tab: ClosedTab, closedIndex: number) {
    event.preventDefault();
    triggerRef.current = event.currentTarget instanceof HTMLElement ? event.currentTarget : null;
    setQuickEntryMenu(null);
    setTabGroupMenu(null);
    setTabMenu(null);
    setClosedTabMenu({
      closedIndex,
      tab,
      ...getMenuPosition(event, 206, 252)
    });
  }

  function openTabGroupMenu(event: MouseEvent, group: TabGroup) {
    event.preventDefault();
    triggerRef.current = event.currentTarget instanceof HTMLElement ? event.currentTarget : null;
    setClosedTabMenu(null);
    setQuickEntryMenu(null);
    setTabMenu(null);
    setTabGroupMenu({
      groupId: group.id,
      ...getMenuPosition(event, 224, 462)
    });
  }

  useEffect(() => {
    if (!closedTabMenu && !tabGroupMenu && !tabMenu && !quickEntryMenu) return;

    const closeOnEscape = (event: KeyboardEvent) => {
      if (event.key === "Escape") closeMenus();
    };
    const closeWithoutFocusRestore = () => closeMenus({ restoreFocus: false });

    window.addEventListener("click", closeWithoutFocusRestore);
    window.addEventListener("blur", closeWithoutFocusRestore);
    window.addEventListener("keydown", closeOnEscape);
    window.addEventListener("scroll", closeWithoutFocusRestore, true);
    return () => {
      window.removeEventListener("click", closeWithoutFocusRestore);
      window.removeEventListener("blur", closeWithoutFocusRestore);
      window.removeEventListener("keydown", closeOnEscape);
      window.removeEventListener("scroll", closeWithoutFocusRestore, true);
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
  return getAnchoredContextMenuPosition(event, { height, width });
}
