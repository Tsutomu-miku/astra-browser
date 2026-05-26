export type CommandPaletteNavigationKey = "ArrowDown" | "ArrowUp" | "End" | "Home";

export function clampCommandIndex(index: number, commandCount: number): number {
  if (commandCount <= 0) return 0;
  if (!Number.isFinite(index)) return 0;
  return Math.min(commandCount - 1, Math.max(0, index));
}

export function getNextCommandIndex(
  currentIndex: number,
  commandCount: number,
  key: CommandPaletteNavigationKey
): number {
  if (commandCount <= 0) return 0;
  const current = clampCommandIndex(currentIndex, commandCount);

  switch (key) {
    case "ArrowDown":
      return (current + 1) % commandCount;
    case "ArrowUp":
      return (current - 1 + commandCount) % commandCount;
    case "End":
      return commandCount - 1;
    case "Home":
      return 0;
  }
}
