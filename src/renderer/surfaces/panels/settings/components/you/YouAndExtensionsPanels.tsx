import { FiPlus, FiDownload, FiTrash2, FiUsers, FiLogIn, FiSettings } from "react-icons/fi";

import type { ExtensionEntry, ProfileEntry } from "../../../../../domain/browser";
import {
  DangerButton,
  Empty,
  GroupHeader,
  NormalButton,
  Pill,
  Row,
  SectionHeader
} from "../shared/SettingsUIPrimitives";

/**
 * M2 设置：§3.12 Profile（C-1 / E-11） + §3.10 Extensions（E-1~E-3）。
 *
 * Profile：本地多人切换 MVP（不依赖登录），Profile 独立 cookies partition。
 * Extensions：E-1/E-2/E-3 MVP — 目前为 list + 安装入口占位。
 *   真正的 Chrome Web Store MV3 兼容将在 M2 中后期通过独立的 Electron 扩展兼容层接入。
 */

export function YouAndAstraSection({
  profiles,
  activeProfileId,
  onSwitchProfile,
  onAddProfile,
  onDeleteProfile,
  onOpenSyncSettings
}: {
  profiles: ProfileEntry[];
  activeProfileId: string;
  onSwitchProfile: (id: string) => void;
  onAddProfile: (name: string, color: string) => void;
  onDeleteProfile: (id: string) => void;
  onOpenSyncSettings?: () => void;
}) {
  const onAdd = () => {
    const n = profiles.length + 1;
    const palette = ["#60a5fa", "#f472b6", "#34d399", "#fbbf24", "#a78bfa", "#fb923c"];
    onAddProfile(`Profile ${n}`, palette[(n - 1) % palette.length]);
  };
  return (
    <section className="settings-pane" aria-label="You and Astra">
      <SectionHeader
        title="You and Astra"
        description="本地多用户 Profile（C-1 / E-11）。每人独立 cookies / 密码 / 扩展 / 设置。跨设备同步留到 M4。"
      />

      <div className="field field-group">
        <GroupHeader
          title={`People (${profiles.length})`}
          action={
            <NormalButton onClick={onAdd}>
              <FiPlus aria-hidden /> Add profile
            </NormalButton>
          }
        />
        {profiles.length === 0 ? (
          <Empty text="尚无 Profile。默认 workspace 对应 Default profile。" />
        ) : (
          <ul className="autofill-list">
            {profiles.map((profile) => {
              const isActive = profile.id === activeProfileId;
              return (
                <li key={profile.id}>
                  <Row
                    avatarColor={profile.color}
                    primary={profile.name}
                    secondary={isActive ? "Signed in · current" : "Inactive profile"}
                    pill={isActive ? { kind: "allow", text: "current" } : undefined}
                    actions={
                      <div className="row-actions">
                        {!isActive && (
                          <NormalButton onClick={() => onSwitchProfile(profile.id)}>
                            Switch
                          </NormalButton>
                        )}
                        <DangerButton onClick={() => onDeleteProfile(profile.id)}>
                          <FiTrash2 aria-hidden />
                        </DangerButton>
                      </div>
                    }
                  />
                </li>
              );
            })}
          </ul>
        )}
      </div>

      <div className="card-row" aria-label="Account actions">
        <button className="row-button" type="button" onClick={onOpenSyncSettings}>
          <strong><FiLogIn aria-hidden /> Account & sync</strong>
          <small>默认关闭。登录后可同步 bookmarks / passwords / history。</small>
        </button>
        <button className="row-button" type="button">
          <strong><FiUsers aria-hidden /> Guest / Incognito</strong>
          <small>应用菜单：New Incognito Window · New Guest Session</small>
        </button>
      </div>
    </section>
  );
}

export function ExtensionsSection({
  extensions,
  onOpenStore,
  onInstallFromFile,
  onToggleEnabled,
  onUninstall,
  onManagePermissions
}: {
  extensions: ExtensionEntry[];
  onOpenStore?: () => void;
  onInstallFromFile?: () => void;
  onToggleEnabled: (id: string, enabled: boolean) => void;
  onUninstall: (id: string) => void;
  onManagePermissions?: (id: string) => void;
}) {
  return (
    <section className="settings-pane" aria-label="Extensions">
      <SectionHeader
        title="Extensions"
        description="Chrome Web Store MV3 兼容（E-1~E-3 PoC）：storage/runtime/tabs/DNR 4 个命名空间 + content_scripts + SW host。通过 astra://flags.mv3-extensions 启用。"
      />

      <div className="button-cluster" aria-label="Install extensions">
        <NormalButton onClick={onOpenStore}>
          <FiDownload aria-hidden /> Open Chrome Web Store
        </NormalButton>
        <NormalButton onClick={onInstallFromFile} aria-label="选择已解压扩展目录（包含 manifest.json）">
          <FiPlus aria-hidden /> Load unpacked
        </NormalButton>
      </div>

      <div className="field field-group">
        <GroupHeader title={`Installed (${extensions.length})`}>
          {extensions.length === 0 ? (
            <Empty text="未安装扩展。Chrome Web Store 官方扩展选择后下载 CRX，或点击 Load unpacked 选择解压目录。" />
          ) : (
            <ul className="autofill-list">
              {extensions.map((ext) => (
                <li key={ext.id}>
                  <Row
                    primary={ext.name}
                    secondary={ext.version + (ext.description ? ` · ${ext.description}` : "")}
                    pill={<Pill kind={ext.enabled ? "allow" : "block"} text={ext.enabled ? "On" : "Off"} />}
                    actions={
                      <div className="row-actions">
                        <label className="field inline-toggle">
                          <input
                            type="checkbox"
                            checked={ext.enabled}
                            onChange={(e) => onToggleEnabled(ext.id, e.target.checked)}
                          />
                          Enabled
                        </label>
                        {onManagePermissions && (
                          <NormalButton onClick={() => onManagePermissions(ext.id)}>
                            <FiSettings aria-hidden /> Permissions
                          </NormalButton>
                        )}
                        <DangerButton onClick={() => onUninstall(ext.id)}>
                          <FiTrash2 aria-hidden /> Remove
                        </DangerButton>
                      </div>
                    }
                  />
                </li>
              ))}
            </ul>
          )}
        </GroupHeader>
      </div>

      <p className="muted">
        已启用 PoC：content_scripts 主世界注入 + session 级 DNR 规则 +
        chrome.storage.local/sync/tabs/runtime SW host。MV3 全部 chrome.* 命名空间、
        Side Panel API、CWS 自动更新在后续版本接入。
      </p>
    </section>
  );
}
