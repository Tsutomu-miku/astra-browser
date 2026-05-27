import {
  clampListIndex,
  getNextListIndex,
  type ListNavigationKey
} from "../navigation/listNavigation";

export type OmniboxNavigationKey = ListNavigationKey;

export const clampOmniboxIndex = clampListIndex;

export function getNextOmniboxIndex(
  currentIndex: number,
  suggestionCount: number,
  key: OmniboxNavigationKey
): number {
  return getNextListIndex(currentIndex, suggestionCount, key);
}
