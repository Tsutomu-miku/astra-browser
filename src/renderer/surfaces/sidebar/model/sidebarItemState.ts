export function isSidebarUrlActive(activeUrl: string, itemUrl: string): boolean {
  return normalizeComparableUrl(activeUrl) === normalizeComparableUrl(itemUrl);
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
