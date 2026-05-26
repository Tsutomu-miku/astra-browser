export type OmniboxNavigationKey = "ArrowDown" | "ArrowUp" | "End" | "Home";

export function clampOmniboxIndex(index: number, suggestionCount: number): number {
  if (suggestionCount <= 0) return 0;
  if (!Number.isFinite(index)) return 0;
  return Math.min(suggestionCount - 1, Math.max(0, index));
}

export function getNextOmniboxIndex(
  currentIndex: number,
  suggestionCount: number,
  key: OmniboxNavigationKey
): number {
  if (suggestionCount <= 0) return 0;
  const current = clampOmniboxIndex(currentIndex, suggestionCount);

  switch (key) {
    case "ArrowDown":
      return (current + 1) % suggestionCount;
    case "ArrowUp":
      return (current - 1 + suggestionCount) % suggestionCount;
    case "End":
      return suggestionCount - 1;
    case "Home":
      return 0;
  }
}
