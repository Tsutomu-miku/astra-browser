/**
 * Barrel export for the 11 M1 interactive settings panels.
 * Each sub-panel lives in its own small file to avoid max-lines violations.
 */
export {
  AutofillSettingsSection
} from "./autofill/AutofillSettingsSection";
export type { AutofillPanelProps } from "./autofill/AutofillSettingsSection";
export {
  AppearanceSettingsSection
} from "./appearance/AppearanceSettingsSection";
export {
  PrivacySecuritySection,
  SiteSettingsSection
} from "./privacy/PrivacySiteSettings";
export {
  DefaultBrowserSection,
  SearchEngineSection,
  StartupSection
} from "./search/SearchStartupPanels";
export {
  DownloadsSection,
  HistorySection
} from "./system/DownloadHistoryPanels";
export {
  ReaderSettingsSection,
  TranslationSettingsSection
} from "./content/ReaderTranslationPanels";
export type { TranslationPanelProps } from "./content/ReaderTranslationPanels";
export {
  DangerButton,
  Empty,
  Field,
  GroupHeader,
  NormalButton,
  Pill,
  Row,
  SectionHeader
} from "./shared/SettingsUIPrimitives";
export {
  PrintSection,
  ResetAndCleanupSection,
  SystemSection
} from "./system/SystemPanels";
export {
  InstalledAppsSection
} from "./system/InstalledAppsPanel";
export {
  ExtensionsSection,
  YouAndAstraSection
} from "./you/YouAndExtensionsPanels";
