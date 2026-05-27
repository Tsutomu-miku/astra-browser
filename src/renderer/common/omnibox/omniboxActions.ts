import type { OmniboxSuggestion } from "./omniboxSuggestions";

export type OmniboxAction =
  | { type: "navigateActiveTab"; value: string }
  | { type: "openTabInSplit"; tabId: string }
  | { type: "openUrlInSplit"; title?: string; url: string }
  | { type: "selectTab"; tabId: string };

export function getOmniboxAction(
  suggestion: OmniboxSuggestion | undefined,
  addressValue: string,
  openInSplit = false
): OmniboxAction {
  switch (suggestion?.type) {
    case "tab":
      return openInSplit
        ? { type: "openTabInSplit", tabId: suggestion.tabId }
        : { type: "selectTab", tabId: suggestion.tabId };
    case "essential":
    case "favorite":
    case "history":
      return openInSplit
        ? { type: "openUrlInSplit", title: suggestion.title, url: suggestion.url }
        : { type: "navigateActiveTab", value: suggestion.url };
    case "navigate":
      return openInSplit
        ? { type: "openUrlInSplit", title: suggestion.title, url: suggestion.value }
        : { type: "navigateActiveTab", value: suggestion.value };
    default:
      return openInSplit
        ? { type: "openUrlInSplit", url: addressValue }
        : { type: "navigateActiveTab", value: addressValue };
  }
}
