import { clampSidebarWidth } from "../../common/layout/sidebarSizing";

const UI_STORAGE_KEY = "astra-browser-ui-state";

export interface BrowserUiState {
  sidebarWidth?: number;
}

export function loadBrowserUiState(): BrowserUiState {
  try {
    const stored = localStorage.getItem(UI_STORAGE_KEY);
    if (!stored) return {};

    const parsed = JSON.parse(stored) as BrowserUiState | null;
    return normalizeBrowserUiState(parsed);
  } catch {
    return {};
  }
}

export function saveBrowserUiState(patch: BrowserUiState): BrowserUiState {
  const next = {
    ...loadBrowserUiState(),
    ...normalizeBrowserUiState(patch)
  };

  localStorage.setItem(UI_STORAGE_KEY, JSON.stringify(next));
  return next;
}

function normalizeBrowserUiState(state: BrowserUiState | null): BrowserUiState {
  if (!state || typeof state !== "object") return {};

  return {
    sidebarWidth: typeof state.sidebarWidth === "number"
      ? clampSidebarWidth(state.sidebarWidth)
      : undefined
  };
}
