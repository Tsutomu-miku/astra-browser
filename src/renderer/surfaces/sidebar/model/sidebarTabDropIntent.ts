import type { BrowserTab } from "../../../domain/browser";

export type SidebarTabDropIntent =
  | { type: "reorder" }
  | { type: "unpinToRegularPosition" };

export function getSidebarTabDropIntent(
  draggedTab: BrowserTab | undefined,
  targetTab: BrowserTab | undefined
): SidebarTabDropIntent {
  if (draggedTab?.isPinned && targetTab && !targetTab.isPinned) {
    return { type: "unpinToRegularPosition" };
  }

  return { type: "reorder" };
}
