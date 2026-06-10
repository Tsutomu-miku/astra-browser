import {
  formatZoomPercent,
  MAX_ZOOM_FACTOR,
  MIN_ZOOM_FACTOR,
  normalizeAddress,
  SEARCH_ENGINES,
  type BrowserSettings,
  type ChromeAccentMode,
  type IncognitoSessionMode,
  type SearchEngineKey,
  type StartupBehavior,
  type ThemeKey
} from "../../../../domain/browser";
import { THEMES, getThemeDefinition } from "../../../../common/theme/themePalette";

export function GlobalSettingsSection({
  onClearPerOriginZoom,
  onResetPerOriginZoom,
  onUpdateSettings,
  onUpsertPerOriginZoom,
  settings
}: {
  onClearPerOriginZoom?: (origin: string) => void;
  onResetPerOriginZoom?: () => void;
  onUpdateSettings: (settings: Partial<BrowserSettings>) => void;
  onUpsertPerOriginZoom?: (origin: string, zoom: number) => void;
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
      <label className="field">
        <span>Default page zoom (U-7)</span>
        <input
          max={MAX_ZOOM_FACTOR}
          min={MIN_ZOOM_FACTOR}
          step={0.1}
          type="range"
          value={settings.defaultZoomFactor}
          onChange={(event) => onUpdateSettings({ defaultZoomFactor: Number(event.target.value) })}
        />
        <small>
          {formatZoomPercent(settings.defaultZoomFactor)} — applies to new tabs
          without a per-origin override.
        </small>
      </label>
      <PerOriginZoomList
        entries={settings.perOriginZoom ?? []}
        onClear={onClearPerOriginZoom ?? noop}
        onResetAll={onResetPerOriginZoom ?? noop}
        onUpsert={onUpsertPerOriginZoom ?? noop}
      />
      <label className="field">
        <span>Incognito (K-12 MVP)</span>
        <select
          value={settings.incognito}
          onChange={(event) => onUpdateSettings({ incognito: event.target.value as IncognitoSessionMode })}
        >
          <option value="disabled">Disabled — all sessions persist on disk</option>
          <option value="in-memory">
            Enable "Incognito window": in-memory partitions, no history written
          </option>
        </select>
        <small>
          Use <kbd>⌃/⌘</kbd>+<kbd>⇧</kbd>+<kbd>N</kbd> to open an incognito
          window when enabled.
        </small>
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

function PerOriginZoomList({
  entries,
  onUpsert,
  onClear,
  onResetAll
}: {
  entries: Array<{ origin: string; zoomFactor: number; updatedAt: number }>;
  onUpsert: (origin: string, zoom: number) => void;
  onClear: (origin: string) => void;
  onResetAll: () => void;
}) {
  return (
    <div className="field field-group" aria-label="Per-origin zoom exceptions">
      <div className="field-group-header">
        <span>Per-origin zoom exceptions</span>
        <button type="button" onClick={onResetAll}>Reset all</button>
      </div>
      {entries.length === 0 && (
        <p className="muted">No per-site overrides yet. Use Ctrl/Cmd +/- to zoom a page — zoom level persists on revisits.</p>
      )}
      <ul className="per-origin-zoom-list">
        {entries.map((entry) => (
          <li key={entry.origin}>
            <code>{entry.origin}</code>
            <input
              max={MAX_ZOOM_FACTOR}
              min={MIN_ZOOM_FACTOR}
              step={0.1}
              type="range"
              value={entry.zoomFactor}
              onChange={(event) => onUpsert(entry.origin, Number(event.target.value))}
            />
            <span>{formatZoomPercent(entry.zoomFactor)}</span>
            <button type="button" onClick={() => onClear(entry.origin)} aria-label={`Remove zoom for ${entry.origin}`}>×</button>
          </li>
        ))}
      </ul>
    </div>
  );
}

function noop(): void {
  /* intentional no-op — setters may not be wired by older call sites */
}
