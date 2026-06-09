import { describe, expect, it } from "vitest";

import { getOmniboxAction, getOmniboxActionHints } from "../src/renderer/common/omnibox/omniboxActions";
import type { OmniboxSuggestion } from "../src/renderer/common/omnibox/omniboxSuggestions";

describe("getOmniboxAction", () => {
  it("selects or splits tab suggestions", () => {
    const suggestion: OmniboxSuggestion = {
      id: "tab-1",
      subtitle: "Open tab",
      tabId: "tab-1",
      title: "Docs",
      type: "tab",
      url: "https://docs.example"
    };

    expect(getOmniboxAction(suggestion, "")).toEqual({ type: "selectTab", tabId: "tab-1" });
    expect(getOmniboxAction(suggestion, "", { altKey: true })).toEqual({
      title: "Docs",
      type: "openGlance",
      url: "https://docs.example"
    });
    expect(getOmniboxAction(suggestion, "", { shiftKey: true })).toEqual({ type: "openTabInSplit", tabId: "tab-1" });
  });

  it("navigates or splits url-backed suggestions", () => {
    const suggestion: OmniboxSuggestion = {
      id: "essential-1",
      subtitle: "Essential",
      title: "Mail",
      type: "essential",
      url: "https://mail.example"
    };

    expect(getOmniboxAction(suggestion, "")).toEqual({
      type: "navigateActiveTab",
      value: "https://mail.example"
    });
    expect(getOmniboxAction(suggestion, "", { altKey: true })).toEqual({
      title: "Mail",
      type: "openGlance",
      url: "https://mail.example"
    });
    expect(getOmniboxAction(suggestion, "", { shiftKey: true })).toEqual({
      title: "Mail",
      type: "openUrlInSplit",
      url: "https://mail.example"
    });
  });

  it("selects Favorite tab suggestions", () => {
    const favoriteSuggestion: OmniboxSuggestion = {
      id: "favorite-1",
      subtitle: "Favorite tab",
      tabId: "tab-favorite-1",
      title: "Docs",
      type: "favorite",
      url: "https://docs.example"
    };

    expect(getOmniboxAction(favoriteSuggestion, "")).toEqual({
      tabId: "tab-favorite-1",
      type: "selectTab"
    });
    expect(getOmniboxAction(favoriteSuggestion, "", { altKey: true })).toEqual({
      title: "Docs",
      type: "openGlance",
      url: "https://docs.example"
    });
    expect(getOmniboxAction(favoriteSuggestion, "", { shiftKey: true })).toEqual({
      tabId: "tab-favorite-1",
      type: "openTabInSplit"
    });
  });

  it("falls back to the typed address when no suggestion is selected", () => {
    expect(getOmniboxAction(undefined, "example.com")).toEqual({
      type: "navigateActiveTab",
      value: "example.com"
    });
    expect(getOmniboxAction(undefined, "example.com", { altKey: true, shiftKey: true })).toEqual({
      type: "openGlance",
      url: "example.com"
    });
    expect(getOmniboxAction(undefined, "example.com", { shiftKey: true })).toEqual({
      type: "openUrlInSplit",
      url: "example.com"
    });
  });

  it("exposes split action hints for visible suggestions", () => {
    const suggestion: OmniboxSuggestion = {
      id: "history-1",
      subtitle: "History",
      title: "Docs",
      type: "history",
      url: "https://docs.example"
    };

    expect(getOmniboxActionHints(suggestion)).toEqual([
      { id: "preview", modifier: "Alt", label: "Preview" },
      { id: "split", modifier: "Shift", label: "Split" }
    ]);
    expect(getOmniboxActionHints(undefined)).toEqual([]);
  });
});
