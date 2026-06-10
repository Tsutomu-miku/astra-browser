import {
  buildTranslateUrl,
  detectLanguage,
  getTranslationPanelStatus,
  normalizeAddress,
  type ReaderSettings,
  type TranslationProvider,
  type TranslationSettings
} from "../../../../../domain/browser";
import {
  DangerButton,
  Empty,
  Field,
  GroupHeader,
  SectionHeader
} from "../shared/SettingsUIPrimitives";

export function ReaderSettingsSection({
  settings,
  onChange
}: {
  settings: ReaderSettings;
  onChange: (patch: Partial<ReaderSettings>) => void;
}) {
  return (
    <section className="settings-pane" aria-label="Reader settings">
      <SectionHeader
        title="Reader mode"
        description="在正文为主的长文章页面自动进入阅读模式（V-3）。"
      />
      <Field label="Enter reader mode automatically">
        <input
          type="checkbox"
          checked={settings.enabled}
          onChange={(e) => onChange({ enabled: e.target.checked })}
        />
      </Field>
      <Field label="Font">
        <select value={settings.fontFamily} onChange={(e) => onChange({ fontFamily: e.target.value })}>
          <option value="serif">Serif</option>
          <option value="sans">Sans-serif</option>
          <option value="mono">Monospace</option>
        </select>
      </Field>
      <Field label={`Font size: ${settings.fontSize}px`}>
        <input
          type="range" min={10} max={36}
          value={settings.fontSize}
          onChange={(e) => onChange({ fontSize: Number(e.target.value) })}
        />
      </Field>
      <Field label={`Line height: ${settings.lineHeight.toFixed(2)}`}>
        <input
          type="range" min={1} max={2.4} step={0.05}
          value={settings.lineHeight}
          onChange={(e) => onChange({ lineHeight: Number(e.target.value) })}
        />
      </Field>
      <Field label={`Content width: ${settings.contentWidth}ch`}>
        <input
          type="range" min={30} max={120}
          value={settings.contentWidth}
          onChange={(e) => onChange({ contentWidth: Number(e.target.value) })}
        />
      </Field>
      <Field label="Theme">
        <select
          value={settings.theme}
          onChange={(e) => onChange({ theme: e.target.value as ReaderSettings["theme"] })}
        >
          <option value="light">Light</option>
          <option value="sepia">Sepia</option>
          <option value="dark">Dark</option>
        </select>
      </Field>
    </section>
  );
}

export interface TranslationPanelProps {
  translation: TranslationSettings;
  activeUrl: string | null;
  pageText?: string;
  onChange: (patch: Partial<TranslationSettings>) => void;
  onTranslateNow: (targetLang: string) => void;
}

export function TranslationSettingsSection(props: TranslationPanelProps) {
  const status = getTranslationPanelStatus(
    { settings: { translation: props.translation } } as never,
    props.activeUrl ?? "",
    props.pageText ?? ""
  );
  const detected = detectLanguage(props.pageText ?? "");
  return (
    <section className="settings-pane" aria-label="Translation settings">
      <SectionHeader
        title="Translation"
        description="页面级翻译 MVP（V-12）。默认走 Google Translate 重写结果页。"
      />
      <Field label="Provider">
        <select
          value={props.translation.provider}
          onChange={(e) => props.onChange({ provider: e.target.value as TranslationProvider })}
        >
          <option value="google">Google Translate (default)</option>
          <option value="libretranslate">LibreTranslate (self-host, M2)</option>
          <option value="disabled">Disabled</option>
        </select>
      </Field>
      <Field label="Preferred target language" hint="e.g. zh-CN, en, ja, ko. BCP 47 code.">
        <input
          value={props.translation.preferredTarget}
          onChange={(e) => props.onChange({ preferredTarget: e.target.value })}
        />
      </Field>
      <Field label="Auto-translate on page load">
        <input
          type="checkbox"
          checked={props.translation.autoTranslate}
          onChange={(e) => props.onChange({ autoTranslate: e.target.checked })}
        />
      </Field>
      {status.suggestedUrl ? (
        <div className="translation-preview card-row">
          <div>
            <strong>Translate now</strong>
            <small>
              Detected <code>{detected}</code> →{" "}
              <code>{props.translation.preferredTarget}</code>
            </small>
          </div>
          <button type="button" onClick={() => props.onTranslateNow(props.translation.preferredTarget)}>
            Open translated page
          </button>
        </div>
      ) : null}
      <div className="field field-group" aria-label="Skip translation for these origins">
        <GroupHeader
          title={`Skip list (${props.translation.skipOrigins.length})`}
          action={
            <form
              onSubmit={(e) => {
                e.preventDefault();
                const fd = new FormData(e.currentTarget);
                const raw = String(fd.get("skip-origin") || "");
                const origin = normalizeAddress(raw).replace(/\/$/, "");
                props.onChange({
                  skipOrigins: [...props.translation.skipOrigins, origin]
                });
              }}
            >
              <input name="skip-origin" placeholder="https://example.com" />
              <button type="submit">Add</button>
            </form>
          }
        />
        {props.translation.skipOrigins.length === 0 ? (
          <Empty text="No exceptions added yet." />
        ) : (
          <ul className="autofill-list">
            {props.translation.skipOrigins.map((origin) => (
              <li key={origin}>
                <code>{origin}</code>
                <DangerButton
                  onClick={() => props.onChange({
                    skipOrigins: props.translation.skipOrigins.filter((x) => x !== origin)
                  })}
                >Remove</DangerButton>
              </li>
            ))}
          </ul>
        )}
      </div>
    </section>
  );
}

export { buildTranslateUrl };
