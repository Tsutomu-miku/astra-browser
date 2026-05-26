import type { BrowserState } from "../domain/browser-core";
import { getActiveWorkspace } from "../domain/selectors";

export type OmniboxSuggestion =
  | { type: "navigate"; id: string; title: string; subtitle: string; value: string }
  | { type: "tab"; id: string; title: string; subtitle: string; tabId: string }
  | { type: "essential" | "favorite" | "history"; id: string; title: string; subtitle: string; url: string };

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
    ...workspace.tabs.map((tab) => ({
      type: "tab" as const,
      id: `tab-${tab.id}`,
      title: tab.title || tab.url,
      subtitle: `Open tab · ${tab.url}`,
      tabId: tab.id
    })),
    ...workspace.favorites.map((favorite) => ({
      type: "favorite" as const,
      id: `favorite-${favorite.id}`,
      title: favorite.title,
      subtitle: `Favorite · ${favorite.url}`,
      url: favorite.url
    })),
    ...state.essentials.map((essential) => ({
      type: "essential" as const,
      id: `essential-${essential.id}`,
      title: essential.title,
      subtitle: `Essential · ${essential.url}`,
      url: essential.url
    })),
    ...state.history.slice(0, 20).map((entry) => ({
      type: "history" as const,
      id: `history-${entry.id}`,
      title: entry.title,
      subtitle: `History · ${entry.url}`,
      url: entry.url
    }))
  ];

  const matches = normalizedQuery
    ? entries.filter((entry) => suggestionMatches(entry, normalizedQuery))
    : entries;

  return [...suggestions, ...matches].slice(0, 8);
}

function suggestionMatches(suggestion: OmniboxSuggestion, query: string): boolean {
  return `${suggestion.title} ${suggestion.subtitle}`.toLowerCase().includes(query);
}

function isLikelyUrl(query: string): boolean {
  return query.includes("://") || /^[^\s]+\.[^\s]+$/.test(query);
}
