export type TabOrganizationTargetKeyboardIntent = "activate" | "cancel";

export function getTabOrganizationTargetKeyboardIntent(key: string): TabOrganizationTargetKeyboardIntent | null {
  if (key === "Enter" || key === " ") return "activate";
  if (key === "Escape") return "cancel";
  return null;
}
