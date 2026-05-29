import type { BrowserTab, Favorite } from "../../../domain/browser";

export interface TabStatusBadge {
  id: "muted" | "sleeping" | "split";
  label: string;
}

export function isSidebarUrlActive(activeUrl: string, itemUrl: string): boolean {
  return normalizeComparableUrl(activeUrl) === normalizeComparableUrl(itemUrl);
}

export function isSidebarFavoriteActive(
  activeTab: Pick<BrowserTab, "id" | "url">,
  favorite: Pick<Favorite, "tabId" | "url">
): boolean {
  return favorite.tabId ? activeTab.id === favorite.tabId : isSidebarUrlActive(activeTab.url, favorite.url);
}

export function getTabStatusBadges(
  tab: Pick<BrowserTab, "id" | "isMuted" | "isSleeping">,
  splitTabIds: string[]
): TabStatusBadge[] {
  const badges: TabStatusBadge[] = [];

  if (splitTabIds.includes(tab.id)) {
    badges.push({ id: "split", label: "Split" });
  }
  if (tab.isMuted) {
    badges.push({ id: "muted", label: "Muted" });
  }
  if (tab.isSleeping) {
    badges.push({ id: "sleeping", label: "Asleep" });
  }

  return badges;
}

export function getSidebarTabAccessibilityLabel({
  isActive,
  kind,
  statusBadges,
  tab
}: {
  isActive: boolean;
  kind: "favorite tab" | "pinned tab" | "tab";
  statusBadges: TabStatusBadge[];
  tab: Pick<BrowserTab, "title" | "url">;
}): string {
  return [
    tab.title || tab.url,
    isActive ? "active" : null,
    kind,
    ...statusBadges.map((badge) => badge.label)
  ].filter(Boolean).join(", ");
}

function normalizeComparableUrl(url: string): string {
  try {
    const parsed = new URL(url);
    parsed.hash = "";
    if (parsed.pathname === "/") parsed.pathname = "";
    return parsed.href.replace(/\/$/, "");
  } catch {
    return url.trim();
  }
}
