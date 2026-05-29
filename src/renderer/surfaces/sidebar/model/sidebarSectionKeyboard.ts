export type SidebarSectionKeyboardToggleIntent = "collapse" | "expand";

export function getSidebarSectionKeyboardToggleIntent(
  key: string,
  isCollapsed: boolean
): SidebarSectionKeyboardToggleIntent | null {
  if (key === "ArrowLeft" && !isCollapsed) return "collapse";
  if (key === "ArrowRight" && isCollapsed) return "expand";
  return null;
}
