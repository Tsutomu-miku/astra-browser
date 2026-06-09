import { DEFAULT_URL, SEARCH_ENGINES } from "./constants";
import type { BrowserState, Favorite, SearchEngineKey, Workspace } from "./types";
import { isInternalNewTabUrl } from "./internalPages";

export function normalizeAddress(value: unknown, searchEngineKey: SearchEngineKey = "google"): string {
  const trimmed = String(value ?? "").trim();
  if (!trimmed) {
    return DEFAULT_URL;
  }

  try {
    const url = new URL(hasAddressScheme(trimmed) ? trimmed : `https://${trimmed}`);
    return url.href;
  } catch {
    const engine = SEARCH_ENGINES[searchEngineKey] ?? SEARCH_ENGINES.google;
    return `${engine.url}${encodeURIComponent(trimmed)}`;
  }
}

function hasAddressScheme(value: string): boolean {
  return value.includes("://") || /^(about|chrome|astra):/i.test(value);
}

export function getHomepageUrl(state?: Pick<BrowserState, "settings">): string {
  return normalizeAddress(state?.settings?.homepage || DEFAULT_URL, state?.settings?.searchEngine);
}

export function getWorkspaceHomepageUrl(
  state: Pick<BrowserState, "settings">,
  workspace: Pick<Workspace, "homepage"> | undefined
): string {
  return normalizeAddress(workspace?.homepage || getHomepageUrl(state), state.settings.searchEngine);
}

export function getSearchUrl(query: string, state?: Pick<BrowserState, "settings">): string {
  const key = state?.settings?.searchEngine;
  const engine = key ? SEARCH_ENGINES[key] : SEARCH_ENGINES.google;
  return `${engine.url}${encodeURIComponent(query)}`;
}

export function getReadableUrlTitle(url: string | undefined): string {
  if (isInternalNewTabUrl(url)) {
    return "New Tab";
  }

  try {
    return new URL(url ?? "").hostname;
  } catch {
    return url || "Untitled";
  }
}

export function getHostInitial(url: string): string {
  try {
    return new URL(url).hostname.replace(/^www\./, "").slice(0, 1).toUpperCase();
  } catch {
    return "?";
  }
}

export function isFavorite(
  workspace: Pick<Workspace, "tabs"> | undefined,
  url: string
): boolean {
  return Boolean(workspace?.tabs?.some((tab) => tab.isFavorite && tab.url === url));
}

export function isTabFavorite(
  _workspace: Pick<Workspace, "tabs"> | undefined,
  tab: Pick<Workspace["tabs"][number], "isFavorite">
): boolean {
  return Boolean(tab?.isFavorite);
}

export function isEssential(state: Pick<BrowserState, "essentials"> | undefined, url: string): boolean {
  return Boolean(state?.essentials?.some((essential: Favorite) => essential.url === url));
}
