import { resolveFavoriteTab, type BrowserState } from "../../domain/browser";
import { getActiveWorkspace } from "../../domain/browser/selectors";

type OmniboxSuggestionBase = {
  id: string;
  title: string;
  subtitle: string;
  completion?: string;
};

export type OmniboxSuggestion =
  | (OmniboxSuggestionBase & { type: "navigate"; value: string })
  | (OmniboxSuggestionBase & { type: "tab"; tabId: string; url?: string })
  | (OmniboxSuggestionBase & { type: "essential" | "history"; url: string })
  | (OmniboxSuggestionBase & { type: "favorite"; tabId?: string; url: string });

export interface OmniboxInlineCompletion {
  suggestionId: string;
  suffix: string;
  value: string;
}

interface RankedSuggestion {
  rank: number;
  suggestion: OmniboxSuggestion;
}

const SUGGESTION_LIMIT = 8;

export function buildOmniboxSuggestions(state: BrowserState, query: string): OmniboxSuggestion[] {
  const workspace = getActiveWorkspace(state);
  const trimmed = query.trim();
  const normalizedQuery = trimmed.toLowerCase();
  const suggestions: OmniboxSuggestion[] = [];

  if (trimmed) {
    suggestions.push({
      type: "navigate",
      id: `navigate-${trimmed}`,
      title: isLikelyUrl(trimmed) ? `Open ${trimmed}` : `Search ${trimmed}`,
      subtitle: isLikelyUrl(trimmed) ? "Open address" : "Search with selected engine",
      value: trimmed
    });
  }

  const entries: OmniboxSuggestion[] = [
    ...state.essentials.map((essential) => ({
      type: "essential" as const,
      completion: getDisplayUrl(essential.url),
      id: `essential-${essential.id}`,
      title: essential.title,
      subtitle: `Essential · ${essential.url}`,
      url: essential.url
    })),
    ...workspace.favorites.map((favorite) => ({
      type: "favorite" as const,
      completion: getDisplayUrl(favorite.url),
      id: `favorite-${favorite.id}`,
      tabId: resolveFavoriteTab(workspace, favorite)?.id,
      title: favorite.title,
      subtitle: `Favorite · ${favorite.url}`,
      url: favorite.url
    })),
    ...workspace.tabs.map((tab) => ({
      type: "tab" as const,
      completion: getDisplayUrl(tab.url),
      id: `tab-${tab.id}`,
      title: tab.title || tab.url,
      subtitle: `Open tab · ${tab.url}`,
      tabId: tab.id,
      url: tab.url
    })),
    ...state.history.slice(0, 20).map((entry) => ({
      type: "history" as const,
      completion: getDisplayUrl(entry.url),
      id: `history-${entry.id}`,
      title: entry.title,
      subtitle: `History · ${entry.url}`,
      url: entry.url
    }))
  ];

  const matches = normalizedQuery
    ? entries
      .map((entry, index) => rankSuggestion(entry, normalizedQuery, index))
      .filter((entry): entry is RankedSuggestion => entry !== null)
      .sort((first, second) => second.rank - first.rank)
      .map(({ suggestion }) => suggestion)
    : entries;

  return [...suggestions, ...matches].slice(0, SUGGESTION_LIMIT);
}

export function getOmniboxInlineCompletion(
  suggestions: OmniboxSuggestion[],
  query: string
): OmniboxInlineCompletion | null {
  const trimmed = query.trim();
  if (!trimmed || trimmed !== query || /\s/.test(trimmed)) return null;

  for (const suggestion of suggestions) {
    if (suggestion.type === "navigate") continue;
    const candidates = getCompletionCandidates(suggestion);
    const completion = candidates.find((candidate) => startsWithIgnoreCase(candidate, trimmed));
    if (completion && completion.length > trimmed.length) {
      return {
        suggestionId: suggestion.id,
        suffix: completion.slice(trimmed.length),
        value: completion
      };
    }
  }

  return null;
}

function rankSuggestion(suggestion: OmniboxSuggestion, query: string, index: number): RankedSuggestion | null {
  const sourceRank = getSourceRank(suggestion);
  const fields = getSearchFields(suggestion);
  let best = -1;

  for (const field of fields) {
    const normalizedField = field.toLowerCase();
    if (normalizedField.startsWith(query)) {
      best = Math.max(best, 1200);
    } else if (normalizedField.includes(` ${query}`) || normalizedField.includes(`.${query}`) || normalizedField.includes(`/${query}`)) {
      best = Math.max(best, 950);
    } else if (normalizedField.includes(query)) {
      best = Math.max(best, 700);
    }
  }

  if (best < 0) return null;

  return {
    rank: best + sourceRank - index,
    suggestion
  };
}

function getSourceRank(suggestion: OmniboxSuggestion): number {
  switch (suggestion.type) {
    case "essential":
      return 80;
    case "favorite":
      return 70;
    case "tab":
      return 60;
    case "history":
      return 40;
    case "navigate":
      return 0;
  }
}

function getSearchFields(suggestion: OmniboxSuggestion): string[] {
  const url = getSuggestionUrl(suggestion);
  return [
    suggestion.title,
    suggestion.subtitle,
    suggestion.completion ?? "",
    url,
    getHost(url)
  ].filter(Boolean);
}

function getCompletionCandidates(suggestion: OmniboxSuggestion): string[] {
  const url = getSuggestionUrl(suggestion);
  const displayUrl = getDisplayUrl(url);
  return uniqueNonEmpty([
    suggestion.completion,
    displayUrl,
    trimTrailingSlash(url)
  ]);
}

function getSuggestionUrl(suggestion: OmniboxSuggestion): string {
  if (suggestion.type === "navigate") return suggestion.value;
  if (suggestion.type === "tab") return suggestion.url ?? "";
  return suggestion.url;
}

function getDisplayUrl(url: string): string {
  try {
    const parsed = new URL(url);
    if (parsed.protocol !== "http:" && parsed.protocol !== "https:") return url;
    return trimTrailingSlash(`${parsed.host}${parsed.pathname}${parsed.search}${parsed.hash}`);
  } catch {
    return url;
  }
}

function getHost(url: string): string {
  try {
    return new URL(url).host;
  } catch {
    return "";
  }
}

function uniqueNonEmpty(values: Array<string | undefined>): string[] {
  return Array.from(new Set(values.filter((value): value is string => Boolean(value))));
}

function startsWithIgnoreCase(value: string, prefix: string): boolean {
  return value.toLowerCase().startsWith(prefix.toLowerCase());
}

function trimTrailingSlash(value: string): string {
  return value.endsWith("/") ? value.slice(0, -1) : value;
}

function isLikelyUrl(query: string): boolean {
  return query.includes("://") || /^[^\s]+\.[^\s]+$/.test(query);
}
