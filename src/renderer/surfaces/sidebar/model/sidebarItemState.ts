import type { BrowserTab } from "../../../domain/browser";

export interface TabStatusBadge {
  id: "muted" | "sleeping" | "split";
  label: string;
  title: string;
}

export function isSidebarUrlActive(activeUrl: string, itemUrl: string): boolean {
  return normalizeComparableUrl(activeUrl) === normalizeComparableUrl(itemUrl);
}

export function getTabStatusBadges(
  tab: Pick<BrowserTab, "id" | "isMuted" | "isSleeping">,
  splitTabIds: string[]
): TabStatusBadge[] {
  const badges: TabStatusBadge[] = [];

  if (splitTabIds.includes(tab.id)) {
    badges.push({ id: "split", label: "Split", title: "Visible in split view" });
  }
  if (tab.isMuted) {
    badges.push({ id: "muted", label: "Muted", title: "Audio muted" });
  }
  if (tab.isSleeping) {
    badges.push({ id: "sleeping", label: "Asleep", title: "Sleeping tab" });
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
  kind: "pinned tab" | "tab";
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
