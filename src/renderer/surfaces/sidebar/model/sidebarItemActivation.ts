import type { KeyboardEvent } from "react";

export type SidebarItemActivation = "primary" | "preview" | "split";

export function getSidebarItemKeyboardActivation(
  event: KeyboardEvent<HTMLElement>
): SidebarItemActivation | null {
  if (event.key !== "Enter" || event.ctrlKey || event.metaKey) return null;
  if (event.altKey) return "preview";
  if (event.shiftKey) return "split";
  return "primary";
}

export function runSidebarItemKeyboardActivation(
  event: KeyboardEvent<HTMLElement>,
  handlers: Record<SidebarItemActivation, () => void>
): boolean {
  const activation = getSidebarItemKeyboardActivation(event);
  if (!activation) return false;

  event.preventDefault();
  event.stopPropagation();
  handlers[activation]();
  return true;
}
