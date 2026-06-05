export type ThemeKey =
  | "arc-dark"
  | "arc-light"
  | "dracula"
  | "everforest"
  | "github-dark"
  | "github-light"
  | "monokai"
  | "nord"
  | "solarized-light";

export interface ThemeDefinition {
  key: ThemeKey;
  label: string;
  description: string;
  /** Preview swatch (hex) used in the settings picker. */
  swatch: string;
  /** Preview accent 2 swatch (hex). */
  accent2: string;
  dark: boolean;
}

export const DEFAULT_THEME: ThemeKey = "arc-dark";

export const THEMES: ThemeDefinition[] = [
  {
    key: "arc-dark",
    label: "Arc Dark",
    description: "Deep charcoal with neutral slate accent (default)",
    swatch: "#08090b",
    accent2: "#7dd3fc",
    dark: true
  },
  {
    key: "arc-light",
    label: "Arc Light",
    description: "Warm off-white paper with neutral accent",
    swatch: "#f5f3ee",
    accent2: "#0284c7",
    dark: false
  },
  {
    key: "dracula",
    label: "Dracula",
    description: "Purple-accented dark theme for late nights",
    swatch: "#282a36",
    accent2: "#ff79c6",
    dark: true
  },
  {
    key: "everforest",
    label: "Everforest",
    description: "Green-tinted low-contrast dark theme",
    swatch: "#272e32",
    accent2: "#e67e80",
    dark: true
  },
  {
    key: "github-dark",
    label: "GitHub Dark",
    description: "GitHub's dark dimmed palette",
    swatch: "#0d1117",
    accent2: "#c297ff",
    dark: true
  },
  {
    key: "github-light",
    label: "GitHub Light",
    description: "GitHub's bright, crisp light theme",
    swatch: "#ffffff",
    accent2: "#8250df",
    dark: false
  },
  {
    key: "monokai",
    label: "Monokai",
    description: "Classic neon-green editor palette",
    swatch: "#271d2a",
    accent2: "#f92672",
    dark: true
  },
  {
    key: "nord",
    label: "Nord",
    description: "Arctic blue-grey, cool and calm",
    swatch: "#2e3440",
    accent2: "#b48ead",
    dark: true
  },
  {
    key: "solarized-light",
    label: "Solarized Light",
    description: "Precision light theme by Ethan Schoonover",
    swatch: "#fdf6e3",
    accent2: "#cb4b16",
    dark: false
  }
];

export const THEME_KEYS = THEMES.map((theme) => theme.key);

export function isThemeKey(value: unknown): value is ThemeKey {
  return typeof value === "string" && THEME_KEYS.includes(value as ThemeKey);
}

export function getThemeDefinition(key: ThemeKey): ThemeDefinition {
  return THEMES.find((candidate) => candidate.key === key) ?? THEMES[0];
}
