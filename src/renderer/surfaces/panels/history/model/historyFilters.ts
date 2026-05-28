import type { HistoryEntry } from "../../../../domain/browser";

export function filterHistory(history: HistoryEntry[], query: string): HistoryEntry[] {
  const normalizedQuery = query.trim().toLowerCase();
  if (!normalizedQuery) return history;

  return history.filter((entry) =>
    entry.title.toLowerCase().includes(normalizedQuery) ||
    entry.url.toLowerCase().includes(normalizedQuery)
  );
}
