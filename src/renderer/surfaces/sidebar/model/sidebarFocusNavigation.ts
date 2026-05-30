import type { KeyboardEvent } from "react";

import { handleFocusableListNavigation } from "../../../common/focus/focusableListNavigation";

const SIDEBAR_FOCUSABLE_ITEM_SELECTOR = [
  ".sidebar-section-header-button",
  ".tab-group-toggle",
  ".favorite-button",
  ".pinned-tab-button",
  ".closed-tab-button",
  ".tab-button"
].join(",");

export function handleSidebarFocusNavigation(event: KeyboardEvent<HTMLElement>): boolean {
  return handleFocusableListNavigation(event, SIDEBAR_FOCUSABLE_ITEM_SELECTOR);
}

export function focusCurrentOrFirstSidebarItem(root: HTMLElement | null): boolean {
  if (!root) return false;

  const target = root.querySelector<HTMLElement>(
    [
      '.tab-row[aria-current="true"] .tab-button',
      '.favorite-button[aria-current="true"]',
      '.pinned-tab-button[aria-current="true"]'
    ].join(",")
  ) ?? root.querySelector<HTMLElement>(SIDEBAR_FOCUSABLE_ITEM_SELECTOR);

  if (!target) return false;

  target.focus({ preventScroll: true });
  target.scrollIntoView?.({ block: "nearest", inline: "nearest" });
  return true;
}
