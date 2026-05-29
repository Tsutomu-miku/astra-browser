import type { BrowserTab } from "../../../domain/browser";

export type SidebarTabDropIntent =
  | { type: "pinToPinnedPosition" }
  | { type: "reorder" }
  | { type: "unpinToRegularEnd" }
  | { type: "unpinToRegularPosition" };

export function getSidebarTabDropIntent(
  draggedTab: BrowserTab | undefined,
  targetTab: BrowserTab | undefined
): SidebarTabDropIntent {
  if (draggedTab?.isPinned && targetTab && !targetTab.isPinned) {
    return { type: "unpinToRegularPosition" };
  }

  if (draggedTab && !draggedTab.isPinned && targetTab?.isPinned) {
    return { type: "pinToPinnedPosition" };
  }

  return { type: "reorder" };
}

export function getSidebarTabsAreaDropIntent(draggedTab: BrowserTab | undefined): SidebarTabDropIntent {
  if (draggedTab?.isPinned) return { type: "unpinToRegularEnd" };

  return { type: "reorder" };
}
