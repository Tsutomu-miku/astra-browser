import {
  clampListIndex,
  getNextListIndex,
  type ListNavigationKey
} from "../../../common/navigation/listNavigation";

export type CommandPaletteNavigationKey = ListNavigationKey;

export const clampCommandIndex = clampListIndex;

export function getNextCommandIndex(
  currentIndex: number,
  commandCount: number,
  key: CommandPaletteNavigationKey
): number {
  return getNextListIndex(currentIndex, commandCount, key);
}
