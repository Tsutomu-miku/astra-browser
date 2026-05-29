import type { KeyboardEvent } from "react";

import { handleFocusableListNavigation } from "../../../common/focus/focusableListNavigation";

const SIDEBAR_FOOTER_FOCUSABLE_ITEM_SELECTOR = ".sidebar-footer button";

export function handleSidebarFooterFocusNavigation(event: KeyboardEvent<HTMLElement>): boolean {
  return handleFocusableListNavigation(event, SIDEBAR_FOOTER_FOCUSABLE_ITEM_SELECTOR, "horizontal");
}
