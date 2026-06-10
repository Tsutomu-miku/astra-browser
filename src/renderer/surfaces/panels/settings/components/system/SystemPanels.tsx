import { FiMonitor, FiPower, FiRefreshCcw, FiTrash2 } from "react-icons/fi";

import type {
  BrowserState,
  BrowserSettings
} from "../../../../../domain/browser";
import {
  DangerButton,
  Field,
  GroupHeader,
  NormalButton,
  SectionHeader
} from "../shared/SettingsUIPrimitives";

/**
 * M2 设置：Print + System + Reset-and-cleanup 三面板。
 * 每一块都是 MVP 可用级（非占位）：
 *   - Print：暴露 W-7 的主要打印选项，调用 Astra main 进程 print-webview IPC
 *   - System：后台运行 / 硬件加速 / 打开文件 / 自动更新占位（W-10 在独立模块接）
 *   - Reset-and-cleanup：恢复默认 settings + 一次性数据清理 + 垃圾扩展清除
 */

export function PrintSection({
  settings,
  onChange,
  onPrintActiveTab
}: {
  settings: BrowserSettings;
  onChange: (patch: Partial<BrowserSettings>) => void;
  onPrintActiveTab: () => void;
}) {
  return (
    <section className="settings-pane" aria-label="Print">
      <SectionHeader
        title="Print"
        description="系统对话框打印，另存 PDF 走 Electron PDFium（W-7）。页眉页脚、背景图形、缩放按 Chrome 基线。"
      />
      <Field label="Header and footer">
        <input
          type="checkbox"
          checked={settings.printHeaders ?? true}
          onChange={(e) => onChange({ printHeaders: e.target.checked })}
        />
      </Field>
      <Field label="Background graphics">
        <input
          type="checkbox"
          checked={settings.printBackgrounds ?? false}
          onChange={(e) => onChange({ printBackgrounds: e.target.checked })}
        />
      </Field>
      <Field label="Default paper size">
        <select
          value={settings.printPaperSize ?? "A4"}
          onChange={(e) => onChange({ printPaperSize: e.target.value })}
        >
          <option value="A4">A4 (210 × 297 mm)</option>
          <option value="Letter">Letter (8.5 × 11 in)</option>
          <option value="Legal">Legal (8.5 × 14 in)</option>
          <option value="Tabloid">Tabloid (11 × 17 in)</option>
        </select>
      </Field>
      <Field label={`Scale: ${Math.round((settings.printScale ?? 1) * 100)}%`}>
        <input
          type="range"
          min={0.5}
          max={2}
          step={0.05}
          value={settings.printScale ?? 1}
          onChange={(e) => onChange({ printScale: Number(e.target.value) })}
        />
      </Field>
      <div className="button-cluster" aria-label="Print actions">
        <NormalButton onClick={onPrintActiveTab}>
          <FiPrinterInline /> Print active tab
        </NormalButton>
      </div>
      <p className="muted">
        PDF 保存：系统打印对话框中选择 "Save as PDF" 即可。
        PDF 表单填写：依赖 PDFium（已在 main 进程 webPreferences.plugins 启用）。
      </p>
    </section>
  );
}

export function SystemSection({
  settings,
  onChange,
  onOpenFolder,
  onRestartBrowser,
  autoUpdateStatus
}: {
  settings: BrowserSettings;
  onChange: (patch: Partial<BrowserSettings>) => void;
  onOpenFolder: (kind: "userData" | "profile") => void;
  onRestartBrowser: () => void;
  autoUpdateStatus: string;
}) {
  return (
    <section className="settings-pane" aria-label="System">
      <SectionHeader
        title="System"
        description="硬件加速 / 后台运行 / 用户数据目录 / 自动更新（W-10 + W-14）。"
      />
      <Field label="Continue running background apps when Astra is closed">
        <input
          type="checkbox"
          checked={settings.backgroundAppMode ?? true}
          onChange={(e) => onChange({ backgroundAppMode: e.target.checked })}
        />
      </Field>
      <Field label="Hardware acceleration">
        <input
          type="checkbox"
          checked={settings.hardwareAcceleration ?? true}
          onChange={(e) => onChange({ hardwareAcceleration: e.target.checked })}
        />
        <small>关闭后需重启浏览器生效。Electron GPU 进程会被禁用。</small>
      </Field>
      <Field label="Low power mode (W-14)">
        <input
          type="checkbox"
          checked={settings.lowPowerMode ?? false}
          onChange={(e) => onChange({ lowPowerMode: e.target.checked })}
        />
        <small>笔记本电池或电量≤20% 时降帧率、减少 webview 预创建。</small>
      </Field>

      <div className="field field-group">
        <GroupHeader
          title="Folders"
          action={
            <NormalButton onClick={() => onOpenFolder("userData")}>
              <FiMonitor aria-hidden /> Open user data
            </NormalButton>
          }
        >
          <p className="muted">
            用户数据目录包含：workspaces、书签、密码、扩展、下载、设置、缓存。
          </p>
        </GroupHeader>
      </div>

      <div className="field field-group">
        <GroupHeader title={`Auto-update — ${autoUpdateStatus}`}>
          <p className="muted">
            自动更新走 Squirrel / Sparkle，签名包安装后生效。CI 的 notarization/EV
            证书就绪后 M2 末打开自动下载。
          </p>
        </GroupHeader>
      </div>

      <div className="card-row" aria-label="Danger zone">
        <button className="row-button danger" type="button" onClick={onRestartBrowser}>
          <strong><FiPower aria-hidden /> Restart browser</strong>
          <small>关闭所有窗口后重新启动（更新、硬件加速切换场景）</small>
        </button>
      </div>
    </section>
  );
}

export function ResetAndCleanupSection({
  onResetSettings,
  onClearAllBrowsingData,
  onClearHistory,
  onClearDownloads,
  browsingDataCount
}: {
  onResetSettings: () => void;
  onClearAllBrowsingData: () => void;
  onClearHistory: () => void;
  onClearDownloads: () => void;
  browsingDataCount: { history: number; downloads: number; permissions: number; autofill: number };
}) {
  return (
    <section className="settings-pane" aria-label="Reset and cleanup">
      <SectionHeader
        title="Reset and cleanup"
        description="恢复默认设置、批量清理扩展与浏览数据。"
      />
      <div className="card-row">
        <button className="row-button danger" type="button" onClick={onResetSettings}>
          <strong><FiRefreshCcw aria-hidden /> Restore settings to their defaults</strong>
          <small>保留 bookmarks / 密码 / 历史，仅重置 settings 块到 factory 默认</small>
        </button>
        <button className="row-button danger" type="button" onClick={onClearAllBrowsingData}>
          <strong><FiTrash2 aria-hidden /> Clear all browsing data</strong>
          <small>
            {browsingDataCount.history} history · {browsingDataCount.downloads} downloads ·{" "}
            {browsingDataCount.permissions} permissions
          </small>
        </button>
      </div>

      <div className="field field-group">
        <GroupHeader title="Selective cleanup">
          <div className="settings-bullets">
            <li>
              <strong>History only</strong>
              <DangerButton onClick={onClearHistory}>
                Delete {browsingDataCount.history} history entries
              </DangerButton>
            </li>
            <li>
              <strong>Download records</strong>
              <DangerButton onClick={onClearDownloads}>
                Delete {browsingDataCount.downloads} download records
              </DangerButton>
            </li>
            <li>
              <strong>Autofill (passwords + addresses + payment methods)</strong>
              <span className="muted">
                {browsingDataCount.autofill} entries — 在 Autofill 面板按条删除。
              </span>
            </li>
          </div>
        </GroupHeader>
      </div>

      <p className="muted">
        扩展清理依赖 §M2 Extensions（E-1/E-3） 合入后，在此面板暴露 "Junk extension cleanup"
        一键清除权限过宽/从未使用的扩展。
      </p>
    </section>
  );
}

/* 内联 FiPrinter 组件 — 避免在 print 面板单独引入 react-icons。 */
function FiPrinterInline() {
  return (
    <svg
      aria-hidden
      height="1em"
      viewBox="0 0 24 24"
      width="1em"
      xmlns="http://www.w3.org/2000/svg"
    >
      <g fill="none" stroke="currentColor" strokeLinecap="round" strokeLinejoin="round" strokeWidth="2">
        <polyline points="6 9 6 2 18 2 18 9" />
        <path d="M6 18H4a2 2 0 0 1-2-2v-5a2 2 0 0 1 2-2h16a2 2 0 0 1 2 2v5a2 2 0 0 1-2 2h-2" />
        <rect height="8" width="12" x="6" y="14" />
      </g>
    </svg>
  );
}
