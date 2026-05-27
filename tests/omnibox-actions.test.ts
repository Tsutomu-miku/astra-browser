import { describe, expect, it } from "vitest";

import { getOmniboxAction } from "../src/renderer/common/omnibox/omniboxActions";
import type { OmniboxSuggestion } from "../src/renderer/common/omnibox/omniboxSuggestions";

describe("getOmniboxAction", () => {
  it("selects or splits tab suggestions", () => {
    const suggestion: OmniboxSuggestion = {
      id: "tab-1",
      subtitle: "Open tab",
      tabId: "tab-1",
      title: "Docs",
      type: "tab"
    };

    expect(getOmniboxAction(suggestion, "")).toEqual({ type: "selectTab", tabId: "tab-1" });
    expect(getOmniboxAction(suggestion, "", true)).toEqual({ type: "openTabInSplit", tabId: "tab-1" });
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
    expect(getOmniboxAction(suggestion, "", true)).toEqual({
      title: "Mail",
      type: "openUrlInSplit",
      url: "https://mail.example"
    });
  });

  it("falls back to the typed address when no suggestion is selected", () => {
    expect(getOmniboxAction(undefined, "example.com")).toEqual({
      type: "navigateActiveTab",
      value: "example.com"
    });
    expect(getOmniboxAction(undefined, "example.com", true)).toEqual({
      type: "openUrlInSplit",
      url: "example.com"
    });
  });
});
