export type DisclosureKeyboardToggleIntent = "collapse" | "expand";

export function getDisclosureKeyboardToggleIntent(
  key: string,
  isCollapsed: boolean
): DisclosureKeyboardToggleIntent | null {
  if (key === "ArrowLeft" && !isCollapsed) return "collapse";
  if (key === "ArrowRight" && isCollapsed) return "expand";
  return null;
}
