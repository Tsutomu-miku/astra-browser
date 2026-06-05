import { useRef, useState, type MouseEvent } from "react";

import { useContextMenuDismissal, type ContextMenuCloseOptions } from "../../../../common/context-menu/menuDismissal";
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

  function closeMenus({ restoreFocus = true }: ContextMenuCloseOptions = {}) {
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
      ...getMenuPosition(event, 224, 494)
    });
  }

  useContextMenuDismissal({
    isOpen: Boolean(closedTabMenu || quickEntryMenu || tabGroupMenu || tabMenu),
    onClose: closeMenus
  });

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
