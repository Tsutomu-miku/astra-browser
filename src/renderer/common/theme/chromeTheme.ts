import type { BrowserSettings, Workspace } from "../../domain/browser";

export const NEUTRAL_CHROME_ACCENT = "#86d39d";

export function getChromeAccent(
  settings: Pick<BrowserSettings, "chromeAccentMode">,
  workspace: Pick<Workspace, "accent">
): string {
  return settings.chromeAccentMode === "space"
    ? workspace.accent
    : NEUTRAL_CHROME_ACCENT;
}
