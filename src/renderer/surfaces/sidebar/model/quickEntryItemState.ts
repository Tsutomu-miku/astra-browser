import type { Favorite } from "../../../domain/browser";

export type QuickEntryKind = "essential" | "favorite";

export function getQuickEntryAccessibilityLabel({
  entry,
  isActive,
  isDragging,
  isDropTarget,
  isSearchSelected,
  kind
}: {
  entry: Favorite;
  isActive: boolean;
  isDragging: boolean;
  isDropTarget: boolean;
  isSearchSelected: boolean;
  kind: QuickEntryKind;
}): string {
  return [
    entry.title || entry.url,
    kind === "essential" ? "Essential" : "Favorite",
    isActive ? "current page" : null,
    isSearchSelected ? "selected search result" : null,
    isDragging ? "dragging" : null,
    isDropTarget ? "drop target" : null
  ].filter(Boolean).join(", ");
}
