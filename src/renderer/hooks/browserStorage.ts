import { applyStartupBehavior, type BrowserState, normalizeState } from "../domain/browser-core";

const STORAGE_KEY = "astra-browser-state";
const LEGACY_STORAGE_KEY = "zen-style-browser-state";

export function loadBrowserState(): BrowserState {
  try {
    const stored = localStorage.getItem(STORAGE_KEY) ?? localStorage.getItem(LEGACY_STORAGE_KEY);
    return applyStartupBehavior(normalizeState(JSON.parse(stored || "null")));
  } catch {
    return applyStartupBehavior(normalizeState(null));
  }
}

export function saveBrowserState(state: BrowserState): void {
  localStorage.setItem(STORAGE_KEY, JSON.stringify(state));
}
