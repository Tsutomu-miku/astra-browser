import type { ReactNode } from "react";

import type {
  AutofillDatabase,
  BrowserSettings,
  DownloadEntry,
  HistoryEntry,
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
  HistorySection,
  PrivacySecuritySection,
  ReaderSettingsSection,
  SearchEngineSection,
  SiteSettingsSection,
  StartupSection,
  TranslationSettingsSection
} from "./M1Panels";
import type { SettingsSectionId } from "../model/settingsSections";

export interface M1PanelProps {
  /* autofill */
  autofill: AutofillDatabase;
  onAddPassword: () => void;
  onEditPassword: (entry: { id: string }) => void;
  onRemovePassword: (id: string) => void;
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
          onRemovePassword={p.onRemovePassword}
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
    default:
      return null;
  }
}
