import { useEffect, type KeyboardEvent, type RefObject } from "react";

import { getNextListIndex, isListNavigationKey } from "../navigation/listNavigation";

const MENU_FOCUSABLE_SELECTOR = [
  "button:not(:disabled)",
  "input:not(:disabled)",
  "select:not(:disabled)",
  "textarea:not(:disabled)",
  "[tabindex]:not([tabindex='-1'])"
].join(",");

export function useMenuInitialFocus(menuRef: RefObject<HTMLElement | null>) {
  useEffect(() => {
    getMenuFocusableItems(menuRef.current)[0]?.focus();
  }, [menuRef]);
}

export function handleMenuKeyboardNavigation(event: KeyboardEvent<HTMLElement>): boolean {
  if (!isListNavigationKey(event.key) || isEditableTarget(event.target)) {
    return false;
  }

  const items = getMenuFocusableItems(event.currentTarget);
  if (items.length === 0) return false;

  event.preventDefault();
  event.stopPropagation();

  const activeIndex = Math.max(0, items.indexOf(document.activeElement as HTMLElement));
  items[getNextListIndex(activeIndex, items.length, event.key)]?.focus();
  return true;
}

function getMenuFocusableItems(menu: HTMLElement | null): HTMLElement[] {
  if (!menu) return [];
  return Array.from(menu.querySelectorAll<HTMLElement>(MENU_FOCUSABLE_SELECTOR))
    .filter((item) => !item.hasAttribute("hidden"));
}

function isEditableTarget(target: EventTarget | null): boolean {
  return target instanceof HTMLInputElement ||
    target instanceof HTMLTextAreaElement ||
    target instanceof HTMLSelectElement;
}
