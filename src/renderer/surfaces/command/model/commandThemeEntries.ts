import { THEMES, getThemeDefinition } from "../../../common/theme/themePalette";
import type { BrowserState, ThemeKey, Workspace } from "../../../domain/browser";
import type { Command, CommandActions } from "./commandTypes";

export function buildThemeCommands(
  state: BrowserState,
  _workspace: Workspace,
  actions: CommandActions
): Command[] {
  const currentTheme = state.settings.theme;
  const currentDef = getThemeDefinition(currentTheme);

  return [
    {
      title: `Current theme: ${currentDef.label}`,
      subtitle: currentDef.description,
      run: () => undefined
    },
    ...THEMES.filter((candidate) => candidate.key !== currentTheme).map<Command>((candidate) => ({
      title: `Switch to ${candidate.label}`,
      subtitle: candidate.description,
      run: () => {
        actions.updateSettings({ theme: candidate.key as ThemeKey });
      }
    }))
  ];
}
