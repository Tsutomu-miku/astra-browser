import type { BrowserState } from "./types";
import { normalizeState } from "./stateNormalization";

export function updateBrowserState(
  state: BrowserState,
  updater: (draft: BrowserState) => void
): BrowserState {
  const next = structuredClone(state);
  updater(next);
  return normalizeState(next);
}
