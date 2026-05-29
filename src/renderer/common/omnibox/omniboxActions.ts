import type { OmniboxSuggestion } from "./omniboxSuggestions";

export interface OmniboxActionHint {
  id: "split";
  label: string;
  modifier: string;
}

export type OmniboxAction =
  | { type: "navigateActiveTab"; value: string }
  | { type: "openTabInSplit"; tabId: string }
  | { type: "openUrlInActiveWorkspace"; title?: string; url: string }
  | { type: "openUrlInSplit"; title?: string; url: string }
  | { type: "selectTab"; tabId: string };

export function getOmniboxActionHints(suggestion: OmniboxSuggestion | undefined): OmniboxActionHint[] {
  if (!suggestion) return [];
  return [{ id: "split", modifier: "Alt", label: "Split" }];
}

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
    case "history":
      return openInSplit
        ? { type: "openUrlInSplit", title: suggestion.title, url: suggestion.url }
        : { type: "navigateActiveTab", value: suggestion.url };
    case "favorite":
      if (openInSplit) {
        return suggestion.tabId
          ? { type: "openTabInSplit", tabId: suggestion.tabId }
          : { type: "openUrlInSplit", title: suggestion.title, url: suggestion.url };
      }
      return suggestion.tabId
        ? { type: "selectTab", tabId: suggestion.tabId }
        : { type: "openUrlInActiveWorkspace", title: suggestion.title, url: suggestion.url };
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
