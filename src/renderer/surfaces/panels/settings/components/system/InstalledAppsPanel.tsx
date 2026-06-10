import { FiDownload, FiExternalLink, FiTrash2 } from "react-icons/fi";

import type { InstalledPwaApp } from "../../../../../domain/browser";
import {
  GroupHeader,
  NormalButton,
  SectionHeader
} from "../shared/SettingsUIPrimitives";

/**
 * M2.4 W-3: Installed apps settings panel.
 * Lists installed PWAs with Launch/Uninstall actions and mirrors the
 * origin-keyed registry kept in the main process. Installs themselves are
 * triggered from the topbar install affordance (beforeinstallprompt).
 */
export function InstalledAppsSection({
  installedApps,
  onLaunchApp,
  onUninstallApp,
  onRefresh
}: {
  installedApps: InstalledPwaApp[];
  onLaunchApp: (origin: string) => Promise<{ ok: boolean; reason?: string }>;
  onUninstallApp: (origin: string) => Promise<{ ok: boolean; reason?: string }>;
  onRefresh: () => Promise<void> | void;
}) {
  return (
    <section className="settings-pane" aria-label="Installed apps">
      <SectionHeader
        title="Installed apps"
        description="PWA / installed web apps. 在地址栏右侧出现 Install 按钮时点击可安装为独立窗口。"
      />
      <div className="field field-group">
        <GroupHeader
          title={`${installedApps.length} app${installedApps.length === 1 ? "" : "s"} installed`}
          action={
            <NormalButton onClick={() => { void onRefresh(); }}>
              <FiDownload aria-hidden /> Refresh list
            </NormalButton>
          }
        >
          {installedApps.length === 0 ? (
            <p className="muted">
              目前没有安装的 Web 应用。打开支持的站点（Gmail、Notion、YouTube
              等）后，地址栏右侧会出现安装按钮。
            </p>
          ) : (
            <div className="card-row" aria-label="Installed PWA list">
              {installedApps.map((app) => (
                <button
                  key={app.id}
                  className="row-button"
                  type="button"
                  onClick={() => { void onLaunchApp(app.origin); }}
                >
                  <strong>
                    {app.icon ? (
                      <img
                        alt=""
                        aria-hidden
                        src={app.icon}
                        style={{ width: 18, height: 18, borderRadius: 4, marginRight: 6, verticalAlign: "middle" }}
                      />
                    ) : null}
                    {app.name}
                  </strong>
                  <small>{app.startUrl}</small>
                  <div className="row-button-actions" style={{ marginTop: 8 }}>
                    <span
                      role="button"
                      tabIndex={0}
                      onKeyDown={(e) => { if (e.key === "Enter") { void onLaunchApp(app.origin); } }}
                      onClick={(e) => { e.stopPropagation(); void onLaunchApp(app.origin); }}
                      style={{
                        display: "inline-flex",
                        alignItems: "center",
                        gap: 6,
                        padding: "6px 10px",
                        borderRadius: 8,
                        background: "var(--control)",
                        color: "var(--text)",
                        border: 0,
                        cursor: "pointer"
                      }}
                    >
                      <FiExternalLink aria-hidden /> Launch
                    </span>
                    <span
                      role="button"
                      tabIndex={0}
                      onKeyDown={(e) => { if (e.key === "Enter") { void onUninstallApp(app.origin); } }}
                      onClick={(e) => {
                        e.stopPropagation();
                        if (window.confirm(`Uninstall ${app.name}?`)) {
                          void onUninstallApp(app.origin);
                        }
                      }}
                      style={{
                        display: "inline-flex",
                        alignItems: "center",
                        gap: 6,
                        padding: "6px 10px",
                        borderRadius: 8,
                        background: "color-mix(in srgb, var(--theme-danger) 15%, transparent)",
                        color: "var(--theme-danger)",
                        border: 0,
                        cursor: "pointer"
                      }}
                    >
                      <FiTrash2 aria-hidden /> Uninstall
                    </span>
                  </div>
                </button>
              ))}
            </div>
          )}
        </GroupHeader>
      </div>
      <p className="muted">
        每个已安装应用使用独立的 persist:astra-pwa-[origin] session
        分区，与常规浏览隔离；离线数据、Service Worker、徽章、通知在该分区内保留。
      </p>
    </section>
  );
}
