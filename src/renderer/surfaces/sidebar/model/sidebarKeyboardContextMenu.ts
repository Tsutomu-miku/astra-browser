import type { KeyboardEvent } from "react";

export function isSidebarContextMenuKey(key: string, shiftKey = false): boolean {
  return key === "ContextMenu" || (key === "F10" && shiftKey);
}

export function openSidebarKeyboardContextMenu(event: KeyboardEvent<HTMLElement>): boolean {
  if (!isSidebarContextMenuKey(event.key, event.shiftKey)) return false;

  event.preventDefault();
  event.stopPropagation();

  const rect = event.currentTarget.getBoundingClientRect();
  event.currentTarget.dispatchEvent(new MouseEvent("contextmenu", {
    bubbles: true,
    clientX: rect.left + rect.width / 2,
    clientY: rect.top + rect.height / 2
  }));
  return true;
}
