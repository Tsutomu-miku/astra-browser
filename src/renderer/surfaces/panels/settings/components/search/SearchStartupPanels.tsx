import {
  SEARCH_ENGINES,
  type SearchEngineKey,
  type StartupBehavior
} from "../../../../../domain/browser";
import { Empty, Field, SectionHeader } from "../shared/SettingsUIPrimitives";

export function SearchEngineSection({
  value,
  onChange
}: {
  value: SearchEngineKey;
  onChange: (next: SearchEngineKey) => void;
}) {
  return (
    <section className="settings-pane" aria-label="Search engine">
      <SectionHeader
        title="Search engine"
        description="地址栏输入非 URL 时走的默认引擎。Tab-to-Search（N-2）在 M2 提供。"
      />
      <Field label="Default search engine">
        <select value={value} onChange={(e) => onChange(e.target.value as SearchEngineKey)}>
          {Object.entries(SEARCH_ENGINES).map(([key, engine]) => (
            <option key={key} value={key}>{engine.name}</option>
          ))}
        </select>
      </Field>
      <ul className="settings-bullets">
        {Object.entries(SEARCH_ENGINES).map(([key, engine]) => (
          <li key={key}>
            <strong>{engine.name}</strong> — <code>{engine.url.replace(/^https?:\/\//, "").slice(0, 40)}…</code>
            {value === key ? <span className="pill allow">active</span> : null}
          </li>
        ))}
      </ul>
    </section>
  );
}

export function DefaultBrowserSection() {
  return (
    <section className="settings-pane" aria-label="Default browser">
      <SectionHeader
        title="Default browser"
        description="需要系统级注册（Windows SetAppUserModelId / macOS LSHandlers）。在 M2 交付真实入口。"
      />
      <Empty text="当前 Astra 尚未注册为系统浏览器，按钮将在 M2 中启用。" />
    </section>
  );
}

export function StartupSection({
  value,
  onChange
}: {
  value: StartupBehavior;
  onChange: (next: StartupBehavior) => void;
}) {
  return (
    <section className="settings-pane" aria-label="On startup">
      <SectionHeader title="On startup" description="启动时恢复会话，还是每个 Space 打开首页。" />
      <Field label="Startup behavior">
        <select value={value} onChange={(e) => onChange(e.target.value as StartupBehavior)}>
          <option value="restore">Restore previous session</option>
          <option value="homepage">Open homepage in each Space</option>
        </select>
      </Field>
    </section>
  );
}
