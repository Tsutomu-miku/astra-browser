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
