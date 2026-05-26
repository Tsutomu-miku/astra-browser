export type ListNavigationKey = "ArrowDown" | "ArrowUp" | "End" | "Home";

export function isListNavigationKey(key: string): key is ListNavigationKey {
  return key === "ArrowDown" || key === "ArrowUp" || key === "End" || key === "Home";
}

export function clampListIndex(index: number, itemCount: number): number {
  if (itemCount <= 0) return 0;
  if (!Number.isFinite(index)) return 0;
  return Math.min(itemCount - 1, Math.max(0, index));
}

export function getNextListIndex(
  currentIndex: number,
  itemCount: number,
  key: ListNavigationKey
): number {
  if (itemCount <= 0) return 0;
  const current = clampListIndex(currentIndex, itemCount);

  switch (key) {
    case "ArrowDown":
      return (current + 1) % itemCount;
    case "ArrowUp":
      return (current - 1 + itemCount) % itemCount;
    case "End":
      return itemCount - 1;
    case "Home":
      return 0;
  }
}
