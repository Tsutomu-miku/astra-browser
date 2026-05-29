import type { KeyboardEvent } from "react";

import { handleFocusableListNavigation } from "../../../common/focus/focusableListNavigation";

const WORKSPACE_FOCUSABLE_ITEM_SELECTOR = ".workspace-button";

export function handleWorkspaceFocusNavigation(event: KeyboardEvent<HTMLElement>): boolean {
  return handleFocusableListNavigation(event, WORKSPACE_FOCUSABLE_ITEM_SELECTOR);
}
