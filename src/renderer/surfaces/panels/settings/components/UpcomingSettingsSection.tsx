import type { SettingsSection } from "../model/settingsSections";

/**
 * UpcomingSettingsSection — PRD §5 M0 skeleton 占位面板。
 *
 * PRD 要求：设置页 16 主区块导航 + 跳转入口（M0 不要求全部实现内容）。
 * 未实现内容的 section 就显示这块：
 *   • 交付时间分带 + PRD 关联章节；
 *   • 预计覆盖的条目列表；
 *   • 一个 "Check PRD §X" 的外链（但在 M0 先显示为文本）。
 */
export function UpcomingSettingsSection({ section }: { section: SettingsSection }) {
  return (
    <section
      className="upcoming-settings-section"
      aria-label={`${section.label} settings (upcoming)`}
    >
      <header className="upcoming-settings-header">
        <h3>{section.label}</h3>
        <p className="upcoming-settings-summary">{section.summary}</p>
        <div className="upcoming-settings-badges">
          <span className={`upcoming-settings-badge priority-${section.priority}`}>
            {section.priority.toUpperCase()}
          </span>
          <span className="upcoming-settings-badge milestone">{section.milestone}</span>
          <span className="upcoming-settings-badge prd-ref">{section.prdRef}</span>
        </div>
      </header>

      <div className="upcoming-settings-body">
        <h4>In scope (scheduled)</h4>
        <ul>
          {section.scope.map((item) => (
            <li key={item}>{item}</li>
          ))}
        </ul>
        <p className="upcoming-settings-note">
          {isCorePanel(section)
            ? "这是 P0 级浏览器底线能力，M1/M2 必须交付才能达到 §4.3 切回 Chrome 临界点 20/20。"
            : "这是 P1/P2 级体验完善，先挂骨架避免用户无从找入口。"}
        </p>
      </div>
    </section>
  );
}

function isCorePanel(section: SettingsSection): boolean {
  return section.priority === "core";
}
