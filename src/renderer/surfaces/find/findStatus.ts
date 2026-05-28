import type { FindResultState } from "../../stores/browserStoreTypes";

export function getFindStatusLabel(query: string, result: FindResultState | null): string {
  if (!query.trim()) return "Ready";
  if (!result) return "Searching";
  if (result.matches === 0 && result.finalUpdate) return "No matches";
  if (result.matches === 0) return "Searching";
  if (result.activeMatchOrdinal > 0) return `${result.activeMatchOrdinal} / ${result.matches}`;
  return `${result.matches} matches`;
}
