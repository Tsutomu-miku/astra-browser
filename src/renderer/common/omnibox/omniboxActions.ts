import type { OmniboxSuggestion } from "./omniboxSuggestions";

export interface OmniboxActionHint {
  id: "preview" | "split";
  label: string;
  modifier: string;
}

export type OmniboxAction =
  | { type: "navigateActiveTab"; value: string }
  | { type: "openGlance"; title?: string; url: string }
  | { type: "openTabInSplit"; tabId: string }
  | { type: "openUrlInActiveWorkspace"; title?: string; url: string }
  | { type: "openUrlInSplit"; title?: string; url: string }
  | { type: "selectTab"; tabId: string };

export interface OmniboxActionModifiers {
  altKey?: boolean;
  shiftKey?: boolean;
}

export function getOmniboxActionHints(suggestion: OmniboxSuggestion | undefined): OmniboxActionHint[] {
  if (!suggestion) return [];
  return [
    { id: "preview", modifier: "Alt", label: "Preview" },
    { id: "split", modifier: "Shift", label: "Split" }
  ];
}

export function getOmniboxAction(
  suggestion: OmniboxSuggestion | undefined,
  addressValue: string,
  modifiers: OmniboxActionModifiers = {}
): OmniboxAction {
  const openPreview = Boolean(modifiers.altKey);
  const openInSplit = Boolean(modifiers.shiftKey) && !openPreview;

  switch (suggestion?.type) {
    case "tab":
      if (openPreview && suggestion.url) return { type: "openGlance", title: suggestion.title, url: suggestion.url };
      return openInSplit
        ? { type: "openTabInSplit", tabId: suggestion.tabId }
        : { type: "selectTab", tabId: suggestion.tabId };
    case "essential":
    case "history":
      if (openPreview) return { type: "openGlance", title: suggestion.title, url: suggestion.url };
      return openInSplit
        ? { type: "openUrlInSplit", title: suggestion.title, url: suggestion.url }
        : { type: "navigateActiveTab", value: suggestion.url };
    case "favorite":
      if (openPreview) return { type: "openGlance", title: suggestion.title, url: suggestion.url };
      if (openInSplit) {
        return suggestion.tabId
          ? { type: "openTabInSplit", tabId: suggestion.tabId }
          : { type: "openUrlInSplit", title: suggestion.title, url: suggestion.url };
      }
      return suggestion.tabId
        ? { type: "selectTab", tabId: suggestion.tabId }
        : { type: "openUrlInActiveWorkspace", title: suggestion.title, url: suggestion.url };
    case "navigate":
      if (openPreview) return { type: "openGlance", title: suggestion.title, url: suggestion.value };
      return openInSplit
        ? { type: "openUrlInSplit", title: suggestion.title, url: suggestion.value }
        : { type: "navigateActiveTab", value: suggestion.value };
    default:
      if (openPreview) return { type: "openGlance", url: addressValue };
      return openInSplit
        ? { type: "openUrlInSplit", url: addressValue }
        : { type: "navigateActiveTab", value: addressValue };
  }
}
