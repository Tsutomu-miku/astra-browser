import { clampSidebarWidth } from "../../common/layout/sidebarSizing";
import {
  normalizeSidebarSectionCollapsedState,
  type SidebarSectionCollapsedState
} from "../../common/sidebar/sidebarSections";

const UI_STORAGE_KEY = "astra-browser-ui-state";

export interface BrowserUiState {
  sidebarSectionCollapsed?: SidebarSectionCollapsedState;
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
  const current = loadBrowserUiState();
  const normalizedPatch = normalizeBrowserUiState(patch);
  const next = {
    ...current,
    ...normalizedPatch
  };

  localStorage.setItem(UI_STORAGE_KEY, JSON.stringify(next));
  return next;
}

function normalizeBrowserUiState(state: BrowserUiState | null): BrowserUiState {
  if (!state || typeof state !== "object") return {};

  const normalized: BrowserUiState = {};

  if (state.sidebarSectionCollapsed) {
    normalized.sidebarSectionCollapsed = normalizeSidebarSectionCollapsedState(state.sidebarSectionCollapsed);
  }

  if (typeof state.sidebarWidth === "number") {
    normalized.sidebarWidth = clampSidebarWidth(state.sidebarWidth);
  }

  return normalized;
}
