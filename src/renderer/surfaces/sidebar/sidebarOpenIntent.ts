import type { SidebarSearchTarget } from "./sidebarFiltering";

export type SidebarOpenIntent =
  | { type: "openUrl"; title?: string; url: string }
  | { type: "navigateUrl"; title?: string; url: string }
  | { type: "preview"; title?: string; url: string }
  | { type: "selectTab"; tabId: string }
  | { type: "splitTab"; tabId: string }
  | { type: "splitUrl"; title?: string; url: string };

export interface SidebarOpenModifiers {
  altKey: boolean;
  shiftKey: boolean;
}

export function getSidebarSearchOpenIntent(
  target: SidebarSearchTarget,
  modifiers: SidebarOpenModifiers
): SidebarOpenIntent {
  if (modifiers.altKey) {
    return { type: "preview", title: target.title, url: target.url };
  }

  if (modifiers.shiftKey) {
    if (target.type === "tab" || target.type === "favorite") {
      return { type: "splitTab", tabId: target.type === "favorite" ? target.tabId : target.id };
    }
    return { type: "splitUrl", title: target.title, url: target.url };
  }

  if (target.type === "tab") return { type: "selectTab", tabId: target.id };
  if (target.type === "favorite") return { type: "selectTab", tabId: target.tabId };
  return { type: "navigateUrl", title: target.title, url: target.url };
}
