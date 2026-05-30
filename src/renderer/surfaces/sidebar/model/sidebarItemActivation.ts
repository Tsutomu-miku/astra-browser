import type { KeyboardEvent, MouseEvent } from "react";

export type SidebarItemActivation = "primary" | "preview" | "split";
export type SidebarItemActivationHandlers = Record<SidebarItemActivation, () => void>;

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
  handlers: SidebarItemActivationHandlers
): boolean {
  const activation = getSidebarItemKeyboardActivation(event);
  if (!activation) return false;

  event.preventDefault();
  event.stopPropagation();
  handlers[activation]();
  return true;
}

export function getSidebarItemPointerActivation(
  event: Pick<MouseEvent<HTMLElement>, "altKey" | "shiftKey">
): SidebarItemActivation {
  if (event.altKey) return "preview";
  if (event.shiftKey) return "split";
  return "primary";
}

export function runSidebarItemPointerActivation(
  event: MouseEvent<HTMLElement>,
  handlers: SidebarItemActivationHandlers
) {
  handlers[getSidebarItemPointerActivation(event)]();
}
