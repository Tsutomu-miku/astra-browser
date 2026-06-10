import type { SitePermissionRule } from "../../../../../domain/browser";
import {
  DangerButton,
  Empty,
  GroupHeader,
  NormalButton,
  Pill,
  Row,
  SectionHeader
} from "../shared/SettingsUIPrimitives";

interface PrivacyPanelProps {
  onClearBrowsingData: () => void;
  onClearProfile: (workspaceId: string) => void;
  permissions: SitePermissionRule[];
  onClearPermission: (rule: SitePermissionRule) => void;
  workspaces: Array<{ id: string; name: string }>;
  historyCount: number;
  downloadCount: number;
}

export function PrivacySecuritySection(props: PrivacyPanelProps) {
  return (
    <section className="settings-pane" aria-label="Privacy and security">
      <SectionHeader
        title="Privacy and security"
        description="浏览数据清理、站点权限总控、下载与历史。"
      />
      <div className="card-row">
        <button className="row-button" type="button" onClick={props.onClearBrowsingData}>
          <strong>Clear browsing data</strong>
          <small>{props.historyCount} history · {props.downloadCount} downloads</small>
        </button>
        {props.workspaces.map((ws) => (
          <button
            key={ws.id}
            className="row-button danger"
            type="button"
            onClick={() => props.onClearProfile(ws.id)}
          >
            <strong>Clear {ws.name} profile data</strong>
            <small>Cookies / cache / storage for this Space</small>
          </button>
        ))}
      </div>
      <div className="field field-group" aria-label="Site permissions">
        <GroupHeader title={`Site permissions (${props.permissions.length})`} />
        {props.permissions.length === 0 ? (
          <Empty text="尚无已记住的站点权限。访问需要摄像头/麦克风等权限的站点时会弹窗询问。" />
        ) : (
          <ul className="autofill-list">
            {props.permissions.map((rule) => (
              <Row
                key={`${rule.origin}-${rule.permission}-${rule.profileId}`}
                primary={rule.origin}
                secondary={rule.permission}
                pill={{ kind: rule.decision === "allow" ? "allow" : "block", text: rule.decision }}
                actions={<DangerButton onClick={() => props.onClearPermission(rule)}>Forget</DangerButton>}
              />
            ))}
          </ul>
        )}
      </div>
      <div className="field field-group" aria-label="Incognito">
        <GroupHeader title="Incognito and guest (M1)" />
        <p className="muted">
          无痕模式入口已在 Global 设置中提供（partition in-memory）。访客模式（一次性会话）将在 M2 交付。
        </p>
      </div>
    </section>
  );
}

const SITE_PERMISSION_KINDS = [
  { id: "media-video", label: "Camera", icon: "📷" },
  { id: "media-audio", label: "Microphone", icon: "🎙️" },
  { id: "geolocation", label: "Location", icon: "📍" },
  { id: "notifications", label: "Notifications", icon: "🔔" },
  { id: "clipboard-read", label: "Clipboard read", icon: "📋" },
  { id: "clipboard-sanitized-write", label: "Clipboard write", icon: "✏️" },
  { id: "midi", label: "MIDI", icon: "🎹" },
  { id: "pointer-lock", label: "Pointer lock", icon: "🖱️" },
  { id: "fullscreen", label: "Fullscreen", icon: "⛶" },
  { id: "openExternal", label: "Protocol handler", icon: "↗" }
];

export function SiteSettingsSection({
  rules,
  onForgetRule,
  onResetByKind
}: {
  rules: SitePermissionRule[];
  onForgetRule: (rule: SitePermissionRule) => void;
  onResetByKind: (kind: string) => void;
}) {
  return (
    <section className="settings-pane" aria-label="Site settings">
      <SectionHeader
        title="Site settings"
        description="Origin 级权限默认值与总览（K-5）。一次性权限与自动重置在 M2 提供。"
      />
      <div className="permission-kind-grid">
        {SITE_PERMISSION_KINDS.map((kind) => {
          const count = rules.filter((r) => r.permission === kind.id).length;
          return (
            <button
              key={kind.id}
              className="permission-kind-tile"
              type="button"
              onClick={() => onResetByKind(kind.id)}
              title={`Clear all "${kind.label}" permissions`}
            >
              <span aria-hidden>{kind.icon}</span>
              <strong>{kind.label}</strong>
              <small>{count} remembered</small>
            </button>
          );
        })}
      </div>
      <div className="field field-group" aria-label="All remembered permissions">
        <GroupHeader
          title={`All remembered permissions (${rules.length})`}
          action={
            rules.length > 0
              ? <NormalButton onClick={() => rules.forEach(onForgetRule)}>Forget all</NormalButton>
              : undefined
          }
        />
        {rules.length === 0 ? (
          <Empty text="尚无已记住的权限。" />
        ) : (
          <ul className="autofill-list">
            {rules.map((r) => (
              <Row
                key={`${r.origin}-${r.permission}-${r.profileId}`}
                primary={r.origin}
                secondary={r.permission}
                pill={{ kind: r.decision === "allow" ? "allow" : "block", text: r.decision }}
                actions={<DangerButton onClick={() => onForgetRule(r)}>Forget</DangerButton>}
              />
            ))}
          </ul>
        )}
      </div>
    </section>
  );
}
