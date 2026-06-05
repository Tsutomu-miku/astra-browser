import {
  normalizeAddress,
  SEARCH_ENGINES,
  type BrowserSettings,
  type ChromeAccentMode,
  type SearchEngineKey,
  type StartupBehavior,
  type ThemeKey
} from "../../../../domain/browser";
import { THEMES, getThemeDefinition } from "../../../../common/theme/themePalette";

export function GlobalSettingsSection({
  onUpdateSettings,
  settings
}: {
  onUpdateSettings: (settings: Partial<BrowserSettings>) => void;
  settings: BrowserSettings;
}) {
  return (
    <section className="settings-pane" aria-label="Global settings">
      <label className="field">
        <span>Homepage</span>
        <input
          autoComplete="off"
          spellCheck={false}
          value={settings.homepage}
          onChange={(event) => onUpdateSettings({ homepage: event.target.value })}
          onBlur={(event) => onUpdateSettings({ homepage: normalizeAddress(event.target.value, settings.searchEngine) })}
        />
      </label>
      <label className="field">
        <span>Chrome color</span>
        <select
          value={settings.chromeAccentMode}
          onChange={(event) => onUpdateSettings({ chromeAccentMode: event.target.value as ChromeAccentMode })}
        >
          <option value="neutral">Neutral</option>
          <option value="space">Match current Space</option>
        </select>
      </label>
      <label className="field">
        <span>Theme</span>
        <select
          value={settings.theme}
          onChange={(event) => onUpdateSettings({ theme: event.target.value as ThemeKey })}
        >
          {THEMES.map((candidate) => (
            <option key={candidate.key} value={candidate.key}>{candidate.label}</option>
          ))}
        </select>
        <ThemePreview theme={settings.theme} />
      </label>
      <label className="field">
        <span>Search engine</span>
        <select
          value={settings.searchEngine}
          onChange={(event) => onUpdateSettings({ searchEngine: event.target.value as SearchEngineKey })}
        >
          {Object.entries(SEARCH_ENGINES).map(([key, engine]) => (
            <option key={key} value={key}>{engine.name}</option>
          ))}
        </select>
      </label>
      <label className="field">
        <span>Startup</span>
        <select
          value={settings.startupBehavior}
          onChange={(event) => onUpdateSettings({ startupBehavior: event.target.value as StartupBehavior })}
        >
          <option value="restore">Restore previous session</option>
          <option value="homepage">Open homepage in each Space</option>
        </select>
      </label>
    </section>
  );
}

function ThemePreview({ theme }: { theme: ThemeKey }) {
  const def = getThemeDefinition(theme);
  return (
    <div className="theme-preview" aria-hidden="true">
      <span
        className="theme-preview-swatch"
        style={{
          background: `linear-gradient(135deg, ${def.swatch} 0%, ${def.accent2} 100%)`
        }}
      />
      <span className="theme-preview-label">{def.description}</span>
    </div>
  );
}
