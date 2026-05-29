import type { FaviconCache } from "./types";

export function getFaviconCacheKey(pageUrl: string | undefined): string | null {
  try {
    const parsed = new URL(pageUrl ?? "");
    if (parsed.protocol !== "http:" && parsed.protocol !== "https:") return null;
    return parsed.origin;
  } catch {
    return null;
  }
}

export function getCachedFaviconUrl(cache: FaviconCache | undefined, pageUrl: string | undefined): string | undefined {
  const key = getFaviconCacheKey(pageUrl);
  return key ? cache?.[key] : undefined;
}

export function setCachedFaviconUrl(cache: FaviconCache, pageUrl: string | undefined, faviconUrl: string | undefined): void {
  const key = getFaviconCacheKey(pageUrl);
  const normalized = normalizeFaviconUrl(faviconUrl);
  if (key && normalized) {
    cache[key] = normalized;
  }
}

export function normalizeFaviconCache(candidate: unknown): FaviconCache {
  if (!candidate || typeof candidate !== "object" || Array.isArray(candidate)) {
    return {};
  }

  const cache: FaviconCache = {};
  for (const [pageUrl, faviconUrl] of Object.entries(candidate)) {
    setCachedFaviconUrl(cache, pageUrl, typeof faviconUrl === "string" ? faviconUrl : undefined);
  }
  return cache;
}

export function normalizeFaviconUrl(faviconUrl: string | undefined): string | null {
  const value = faviconUrl?.trim();
  if (!value) return null;

  try {
    const parsed = new URL(value);
    if (parsed.protocol === "http:" || parsed.protocol === "https:" || parsed.protocol === "file:" || parsed.protocol === "data:") {
      return value;
    }
  } catch {
    return null;
  }

  return null;
}
