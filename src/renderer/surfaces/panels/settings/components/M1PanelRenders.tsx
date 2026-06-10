import type { ReactNode } from "react";

import type {
  AutofillDatabase,
  BrowserSettings,
  DownloadEntry,
  ExtensionEntry,
  HistoryEntry,
  PasswordEntry,
  ProfileEntry,
  ReaderSettings,
  SearchEngineKey,
  SitePermissionRule,
  StartupBehavior,
  TranslationSettings
} from "../../../../domain/browser";
import {
  AppearanceSettingsSection,
  AutofillSettingsSection,
  DefaultBrowserSection,
  DownloadsSection,
  ExtensionsSection,
  HistorySection,
  PrintSection,
  PrivacySecuritySection,
  ReaderSettingsSection,
  ResetAndCleanupSection,
  SearchEngineSection,
  SiteSettingsSection,
  StartupSection,
  SystemSection,
  TranslationSettingsSection,
  YouAndAstraSection
} from "./M1Panels";
import type { SettingsSectionId } from "../model/settingsSections";

export interface M1PanelProps {
  /* autofill */
  autofill: AutofillDatabase;
  onAddPassword: () => void;
  onEditPassword: (entry: PasswordEntry) => void;
  onRevealPassword: (id: string) => Promise<string | null>;
  onRemovePassword: (id: string) => void;
  passwordVaultUnlocked: boolean;
  onUnlockVault: (passphrase?: string) => Promise<void>;
  onLockVault: () => void;
  passwordSearchQuery: string;
  setPasswordSearchQuery: (q: string) => void;
  onAddAddress: () => void;
  onRemoveAddress: (id: string) => void;
  onAddPaymentMethod: () => void;
  onRemovePaymentMethod: (id: string) => void;

  /* appearance */
  appearance: {
    settings: BrowserSettings;
    onChange: (patch: Partial<BrowserSettings>) => void;
    onClearPerOriginZoom: (origin: string) => void;
    onResetPerOriginZoom: () => void;
    onUpsertPerOriginZoom: (origin: string, zoom: number) => void;
  };

  /* privacy + site settings */
  onClearBrowsingData: () => void;
  onClearProfile: (workspaceId: string) => void;
  permissions: SitePermissionRule[];
  onForgetPermission: (rule: SitePermissionRule) => void;
  workspaces: Array<{ id: string; name: string }>;
  historyCount: number;
  downloadCount: number;
  onResetPermissionByKind: (kind: string) => void;

  /* search + startup */
  searchEngine: SearchEngineKey;
  onUpdateSearchEngine: (next: SearchEngineKey) => void;
  startup: StartupBehavior;
  onUpdateStartup: (next: StartupBehavior) => void;

  /* downloads */
  downloads: DownloadEntry[];
  onOpenDownload: (path: string) => void;
  onOpenPath: (path: string) => void;
  onCancelDownload: (id: string) => void;

  /* history (performance section MVP) */
  history: HistoryEntry[];
  historyQuery: string;
  setHistoryQuery: (q: string) => void;
  onOpenHistory: (url: string, title: string) => void;
  onClearHistoryEntry: (id: string) => void;
  clearHistory: () => void;

  /* reader (accessibility section MVP) */
  reader: ReaderSettings;
  onUpdateReader: (patch: Partial<ReaderSettings>) => void;

  /* translation (languages section MVP) */
  translation: TranslationSettings;
  activeUrl: string | null;
  onUpdateTranslation: (patch: Partial<TranslationSettings>) => void;
  onTranslateNow: (targetLang: string) => void;

  /* ===== M2.1 Profiles / Extensions ===== */
  profiles: ProfileEntry[];
  activeProfileId: string;
  onSwitchProfile: (id: string) => void;
  onAddProfile: (name: string, color: string) => void;
  onDeleteProfile: (id: string) => void;
  extensions: ExtensionEntry[];
  onToggleExtension: (id: string, enabled: boolean) => void;
  onUninstallExtension: (id: string) => void;
  onInstallExtensionFromFile?: () => void;
  onOpenExtensionStore?: () => void;

  /* ===== M2.1 Print / System ===== */
  settings: BrowserSettings;
  onChangeSettings: (patch: Partial<BrowserSettings>) => void;
  onPrintActiveTab: (options?: Record<string, unknown>) => void | Promise<unknown>;
  onOpenFolder: (kind: "userData" | "profile") => void;
  onRestartBrowser: () => void;
  autoUpdateStatus: string;

  /* ===== M2.1 Reset-and-cleanup ===== */
  onResetSettings: () => void;
  onClearAllBrowsingData: () => void;
  onClearHistory: () => void;
  onClearDownloads: () => void;
  browsingDataCount: { history: number; downloads: number; permissions: number; autofill: number };
}

export function renderM1Panels(
  activeSection: SettingsSectionId,
  p: M1PanelProps
): ReactNode {
  switch (activeSection) {
    case "autofill":
      return (
        <AutofillSettingsSection
          autofill={p.autofill}
          onAddPassword={p.onAddPassword}
          onEditPassword={p.onEditPassword}
          onRevealPassword={p.onRevealPassword}
          onRemovePassword={p.onRemovePassword}
          passwordVaultUnlocked={p.passwordVaultUnlocked}
          onUnlockVault={p.onUnlockVault}
          onLockVault={p.onLockVault}
          passwordSearchQuery={p.passwordSearchQuery}
          setPasswordSearchQuery={p.setPasswordSearchQuery}
          onAddAddress={p.onAddAddress}
          onRemoveAddress={p.onRemoveAddress}
          onAddPaymentMethod={p.onAddPaymentMethod}
          onRemovePaymentMethod={p.onRemovePaymentMethod}
        />
      );
    case "privacy-and-security":
      return (
        <PrivacySecuritySection
          onClearBrowsingData={p.onClearBrowsingData}
          onClearProfile={p.onClearProfile}
          permissions={p.permissions}
          onClearPermission={p.onForgetPermission}
          workspaces={p.workspaces}
          historyCount={p.historyCount}
          downloadCount={p.downloadCount}
        />
      );
    case "appearance":
      return <AppearanceSettingsSection {...p.appearance} />;
    case "search-engine":
      return (
        <SearchEngineSection
          value={p.searchEngine}
          onChange={p.onUpdateSearchEngine}
        />
      );
    case "default-browser":
      return <DefaultBrowserSection />;
    case "startup":
      return <StartupSection value={p.startup} onChange={p.onUpdateStartup} />;
    case "site-settings":
      return (
        <SiteSettingsSection
          rules={p.permissions}
          onForgetRule={p.onForgetPermission}
          onResetByKind={p.onResetPermissionByKind}
        />
      );
    case "downloads":
      return (
        <DownloadsSection
          entries={p.downloads}
          onOpen={(entry) => p.onOpenDownload(entry.savePath)}
          onOpenPath={(entry) => p.onOpenPath(entry.savePath)}
          onCancel={p.onCancelDownload}
        />
      );
    case "performance":
      return (
        <HistorySection
          entries={p.history}
          query={p.historyQuery}
          setQuery={p.setHistoryQuery}
          onOpen={(e) => p.onOpenHistory(e.url, e.title)}
          onRemove={p.onClearHistoryEntry}
          onClearAll={p.clearHistory}
        />
      );
    case "accessibility":
      return (
        <ReaderSettingsSection
          settings={p.reader}
          onChange={p.onUpdateReader}
        />
      );
    case "languages":
      return (
        <TranslationSettingsSection
          translation={p.translation}
          activeUrl={p.activeUrl}
          onChange={p.onUpdateTranslation}
          onTranslateNow={p.onTranslateNow}
        />
      );
    case "you-and-astra":
      return (
        <YouAndAstraSection
          profiles={p.profiles}
          activeProfileId={p.activeProfileId}
          onSwitchProfile={p.onSwitchProfile}
          onAddProfile={p.onAddProfile}
          onDeleteProfile={p.onDeleteProfile}
        />
      );
    case "extensions":
      return (
        <ExtensionsSection
          extensions={p.extensions}
          onToggleEnabled={p.onToggleExtension}
          onUninstall={p.onUninstallExtension}
          onInstallFromFile={p.onInstallExtensionFromFile}
          onOpenStore={p.onOpenExtensionStore}
        />
      );
    case "print":
      return (
        <PrintSection
          settings={p.settings}
          onChange={p.onChangeSettings}
          onPrintActiveTab={p.onPrintActiveTab}
        />
      );
    case "system":
      return (
        <SystemSection
          settings={p.settings}
          onChange={p.onChangeSettings}
          onOpenFolder={p.onOpenFolder}
          onRestartBrowser={p.onRestartBrowser}
          autoUpdateStatus={p.autoUpdateStatus}
        />
      );
    case "reset-and-cleanup":
      return (
        <ResetAndCleanupSection
          onResetSettings={p.onResetSettings}
          onClearAllBrowsingData={p.onClearAllBrowsingData}
          onClearHistory={p.onClearHistory}
          onClearDownloads={p.onClearDownloads}
          browsingDataCount={p.browsingDataCount}
        />
      );
    default:
      return null;
  }
}
