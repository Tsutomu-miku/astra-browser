import type { KeyboardEvent } from "react";

import { getNextListIndex, type ListNavigationKey } from "../navigation/listNavigation";

export type FocusableListOrientation = "horizontal" | "vertical";

export function handleFocusableListNavigation(
  event: KeyboardEvent<HTMLElement>,
  selector: string,
  orientation: FocusableListOrientation = "vertical"
): boolean {
  const navigationKey = getNavigationKey(event.key, orientation);
  if (
    event.altKey ||
    event.ctrlKey ||
    event.metaKey ||
    !navigationKey ||
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
  const nextItem = items[getNextListIndex(activeIndex, items.length, navigationKey)];
  nextItem?.focus({ preventScroll: true });
  nextItem?.scrollIntoView?.({ block: "nearest", inline: "nearest" });
  return true;
}

function getNavigationKey(key: string, orientation: FocusableListOrientation): ListNavigationKey | null {
  if (key === "Home" || key === "End") return key;
  if (orientation === "vertical") {
    if (key === "ArrowDown" || key === "ArrowUp") return key;
    return null;
  }

  if (key === "ArrowRight") return "ArrowDown";
  if (key === "ArrowLeft") return "ArrowUp";
  return null;
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
