import {
  formatZoomPercent,
  MAX_ZOOM_FACTOR,
  MIN_ZOOM_FACTOR,
  type BrowserSettings,
  type ChromeAccentMode,
  type ThemeKey
} from "../../../../../domain/browser";
import { THEMES, getThemeDefinition } from "../../../../../common/theme/themePalette";
import {
  Empty,
  Field,
  GroupHeader,
  SectionHeader
} from "../shared/SettingsUIPrimitives";

interface AppearanceProps {
  settings: BrowserSettings;
  onChange: (patch: Partial<BrowserSettings>) => void;
  onClearPerOriginZoom: (origin: string) => void;
  onResetPerOriginZoom: () => void;
  onUpsertPerOriginZoom: (origin: string, zoom: number) => void;
}

export function AppearanceSettingsSection(props: AppearanceProps) {
  const { settings, onChange, onClearPerOriginZoom, onResetPerOriginZoom, onUpsertPerOriginZoom } = props;
  return (
    <section className="settings-pane" aria-label="Appearance">
      <SectionHeader title="Appearance" description="主题、色板、侧栏/书签栏、页面缩放。" />
      <Field label="Chrome color" hint="所有 Space 共用的 Chrome 色板，或跟随当前 Space 的 accent。">
        <select
          value={settings.chromeAccentMode}
          onChange={(e) => onChange({ chromeAccentMode: e.target.value as ChromeAccentMode })}
        >
          <option value="neutral">Neutral</option>
          <option value="space">Match current Space</option>
        </select>
      </Field>
      <Field label="Theme" hint="Light / Dark 跟随系统将在 M3 提供（U-12）。">
        <select value={settings.theme} onChange={(e) => onChange({ theme: e.target.value as ThemeKey })}>
          {THEMES.map((candidate) => (
            <option key={candidate.key} value={candidate.key}>{candidate.label}</option>
          ))}
        </select>
        <ThemePreview theme={settings.theme} />
      </Field>
      <Field
        label="Default page zoom (U-7)"
        hint={`${formatZoomPercent(settings.defaultZoomFactor)} — 新标签默认缩放，per-origin 例外优先级更高。`}
      >
        <input
          max={MAX_ZOOM_FACTOR}
          min={MIN_ZOOM_FACTOR}
          step={0.1}
          type="range"
          value={settings.defaultZoomFactor}
          onChange={(e) => onChange({ defaultZoomFactor: Number(e.target.value) })}
        />
      </Field>
      <div className="field field-group" aria-label="Per-origin zoom exceptions">
        <GroupHeader
          title={`Per-origin zoom exceptions (${(settings.perOriginZoom ?? []).length})`}
          action={<button type="button" onClick={onResetPerOriginZoom}>Reset all</button>}
        />
        {(settings.perOriginZoom ?? []).length === 0 ? (
          <Empty text="No per-site overrides yet. Ctrl/Cmd +/- 或页面菜单缩放会自动记忆。" />
        ) : (
          <ul className="per-origin-zoom-list">
            {(settings.perOriginZoom ?? []).map((entry) => (
              <li key={entry.origin}>
                <code>{entry.origin}</code>
                <input
                  max={MAX_ZOOM_FACTOR}
                  min={MIN_ZOOM_FACTOR}
                  step={0.1}
                  type="range"
                  value={entry.zoomFactor}
                  onChange={(e) => onUpsertPerOriginZoom(entry.origin, Number(e.target.value))}
                />
                <span>{formatZoomPercent(entry.zoomFactor)}</span>
                <button
                  aria-label={`Remove zoom for ${entry.origin}`}
                  className="danger"
                  type="button"
                  onClick={() => onClearPerOriginZoom(entry.origin)}
                >×</button>
              </li>
            ))}
          </ul>
        )}
      </div>
    </section>
  );
}

function ThemePreview({ theme }: { theme: ThemeKey }) {
  const def = getThemeDefinition(theme);
  return (
    <div className="theme-preview" aria-hidden="true">
      <span
        className="theme-preview-swatch"
        style={{ background: `linear-gradient(135deg, ${def.swatch} 0%, ${def.accent2} 100%)` }}
      />
      <span className="theme-preview-label">{def.description}</span>
    </div>
  );
}
