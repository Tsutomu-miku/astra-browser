import { beforeEach, describe, expect, it } from "vitest";

import {
  loadBrowserUiState,
  saveBrowserUiState
} from "../src/renderer/platform/persistence/browserUiStorage";

describe("browser UI storage", () => {
  beforeEach(() => {
    localStorage.clear();
  });

  it("loads empty UI state when storage is missing or invalid", () => {
    expect(loadBrowserUiState()).toEqual({});

    localStorage.setItem("astra-browser-ui-state", "{bad json");
    expect(loadBrowserUiState()).toEqual({});
  });

  it("persists clamped sidebar width separately from browser state", () => {
    expect(saveBrowserUiState({ sidebarWidth: 999 })).toEqual({ sidebarWidth: 420 });
    expect(loadBrowserUiState()).toEqual({ sidebarWidth: 420 });

    expect(saveBrowserUiState({ sidebarWidth: 100 })).toEqual({ sidebarWidth: 240 });
    expect(loadBrowserUiState()).toEqual({ sidebarWidth: 240 });
    expect(localStorage.getItem("astra-browser-state")).toBeNull();
  });
});
