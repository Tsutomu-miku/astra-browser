export type SettingsSectionId = "global" | "space" | "data" | "workspaces";

export interface SettingsSection {
  id: SettingsSectionId;
  label: string;
  summary: string;
}

export const SETTINGS_SECTIONS: SettingsSection[] = [
  { id: "global", label: "Global", summary: "Appearance, homepage, search" },
  { id: "space", label: "Space", summary: "Name, accent, profile" },
  { id: "data", label: "Data", summary: "Storage, backup, cleanup" },
  { id: "workspaces", label: "Spaces", summary: "Create and remove Spaces" }
];
