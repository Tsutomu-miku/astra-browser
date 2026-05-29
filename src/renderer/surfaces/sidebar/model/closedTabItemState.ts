import type { ClosedTab } from "../../../domain/browser";

export function getClosedTabAccessibilityLabel({
  closedIndex,
  isDragging,
  tab
}: {
  closedIndex: number;
  isDragging: boolean;
  tab: ClosedTab;
}): string {
  return [
    tab.title || tab.url,
    "recently closed tab",
    `restore position ${closedIndex + 1}`,
    isDragging ? "dragging" : null
  ].filter(Boolean).join(", ");
}
