import type { BrowserSettings, Workspace } from "../../domain/browser";

export const NEUTRAL_CHROME_ACCENT = "#a8b0bd";

export function getChromeAccent(
  settings: Pick<BrowserSettings, "chromeAccentMode">,
  workspace: Pick<Workspace, "accent">
): string {
  return settings.chromeAccentMode === "space"
    ? workspace.accent
    : NEUTRAL_CHROME_ACCENT;
}
