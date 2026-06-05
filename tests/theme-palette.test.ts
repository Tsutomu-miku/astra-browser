import { describe, expect, it } from "vitest";

import {
  DEFAULT_THEME,
  THEMES,
  THEME_KEYS,
  getThemeDefinition,
  isThemeKey
} from "../src/renderer/common/theme/themePalette";
import { createDefaultState, normalizeState, type ThemeKey } from "../src/renderer/domain/browser";
import { updateSettings } from "../src/renderer/domain/actions";

describe("theme palette registry", () => {
  it("exposes the default Arc Dark theme", () => {
    expect(DEFAULT_THEME).toBe("arc-dark");
    expect(getThemeDefinition("arc-dark").label).toBe("Arc Dark");
    expect(getThemeDefinition("arc-dark").dark).toBe(true);
  });

  it("registers exactly nine themes and matches the key list", () => {
    expect(THEMES).toHaveLength(9);
    expect(THEME_KEYS).toEqual(THEMES.map((theme) => theme.key));
  });

  it("provides preview swatches for every theme", () => {
    for (const theme of THEMES) {
      expect(theme.swatch).toMatch(/^#[0-9a-f]{6}$/i);
      expect(theme.accent2).toMatch(/^#[0-9a-f]{6}$/i);
      expect(theme.label.length).toBeGreaterThan(0);
      expect(theme.description.length).toBeGreaterThan(0);
    }
  });

  it("guards unknown theme keys", () => {
    expect(isThemeKey("arc-dark")).toBe(true);
    expect(isThemeKey("dracula")).toBe(true);
    expect(isThemeKey("solarized-light")).toBe(true);
    expect(isThemeKey("nordic")).toBe(false);
    expect(isThemeKey(undefined)).toBe(false);
    expect(isThemeKey(null)).toBe(false);
    expect(isThemeKey(42)).toBe(false);
  });

  it("falls back to the default theme for unknown keys", () => {
    expect(getThemeDefinition("bogus" as ThemeKey).key).toBe(DEFAULT_THEME);
  });
});

describe("theme persistence in browser state", () => {
  it("defaults to Arc Dark in a fresh state", () => {
    expect(createDefaultState().settings.theme).toBe("arc-dark");
  });

  it("normalizes unknown themes to the default", () => {
    const state = createDefaultState();
    (state.settings as unknown as { theme: string }).theme = "bogus-theme";
    const normalized = normalizeState(state);
    expect(normalized.settings.theme).toBe("arc-dark");
  });

  it("preserves a valid theme through normalization", () => {
    const state = createDefaultState();
    state.settings.theme = "dracula";
    const normalized = normalizeState(state);
    expect(normalized.settings.theme).toBe("dracula");
  });

  it("updates the theme via updateSettings", () => {
    const state = createDefaultState();
    const dracula = updateSettings(state, { theme: "dracula" });
    expect(dracula.settings.theme).toBe("dracula");

    const nord = updateSettings(dracula, { theme: "nord" });
    expect(nord.settings.theme).toBe("nord");
    expect(dracula.settings.theme).toBe("dracula");
  });
});
