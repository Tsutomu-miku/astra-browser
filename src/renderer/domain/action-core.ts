import { BrowserState, normalizeState } from "./browser-core";

export function updateBrowserState(
  state: BrowserState,
  updater: (draft: BrowserState) => void
): BrowserState {
  const next = structuredClone(state);
  updater(next);
  return normalizeState(next);
}
