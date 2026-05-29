import type { KeyboardEvent } from "react";

import { getNextListIndex, isListNavigationKey } from "../navigation/listNavigation";

export function handleFocusableListNavigation(
  event: KeyboardEvent<HTMLElement>,
  selector: string
): boolean {
  if (
    event.altKey ||
    event.ctrlKey ||
    event.metaKey ||
    !isListNavigationKey(event.key) ||
    isEditableTarget(event.target)
  ) {
    return false;
  }

  const items = getFocusableItems(event.currentTarget, selector);
  if (items.length === 0) return false;

  event.preventDefault();
  event.stopPropagation();

  const activeItem = findActiveFocusableItem(items, document.activeElement);
  const activeIndex = Math.max(0, activeItem ? items.indexOf(activeItem) : -1);
  items[getNextListIndex(activeIndex, items.length, event.key)]?.focus();
  return true;
}

function getFocusableItems(root: HTMLElement, selector: string): HTMLElement[] {
  return Array.from(root.querySelectorAll<HTMLElement>(selector))
    .filter((item) => !item.hasAttribute("hidden") && !isDisabled(item));
}

function findActiveFocusableItem(items: HTMLElement[], activeElement: Element | null): HTMLElement | undefined {
  if (!activeElement) return undefined;
  return items.find((item) => item === activeElement || item.contains(activeElement));
}

function isDisabled(item: HTMLElement): boolean {
  return item instanceof HTMLButtonElement && item.disabled;
}

function isEditableTarget(target: EventTarget | null): boolean {
  return target instanceof HTMLInputElement ||
    target instanceof HTMLTextAreaElement ||
    target instanceof HTMLSelectElement ||
    (target instanceof HTMLElement && target.isContentEditable);
}
