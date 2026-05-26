import { describe, expect, it } from "vitest";

import {
  createDefaultState,
  createFavorite
} from "../src/renderer/domain/browser-core";
import {
  openUrlInActiveWorkspace,
  recordHistory
} from "../src/renderer/domain/browser-actions";
import { getActiveTab, getActiveWorkspace } from "../src/renderer/domain/selectors";
import { buildOmniboxSuggestions } from "../src/renderer/hooks/omniboxSuggestions";

describe("buildOmniboxSuggestions", () => {
  it("adds a direct navigation suggestion for typed queries", () => {
    const suggestions = buildOmniboxSuggestions(createDefaultState(), "example.com");

    expect(suggestions[0]).toMatchObject({
      type: "navigate",
      title: "Open example.com",
      subtitle: "Open address",
      value: "example.com"
    });
  });

  it("returns active workspace tabs, favorites, and history matches", () => {
    const opened = openUrlInActiveWorkspace(createDefaultState(), "docs.example", "Docs");
    const active = getActiveTab(getActiveWorkspace(opened));
    const withHistory = recordHistory(opened, active.id, active.url);
    getActiveWorkspace(withHistory).favorites.push(createFavorite("Docs Favorite", "https://docs.example/guide"));
    withHistory.essentials.push(createFavorite("Docs Essential", "https://docs.example/essential"));
    const suggestions = buildOmniboxSuggestions(withHistory, "docs");

    expect(suggestions.some((suggestion) => suggestion.type === "tab" && suggestion.title === "Docs")).toBe(true);
    expect(suggestions.some((suggestion) => suggestion.type === "favorite" && suggestion.title === "Docs Favorite")).toBe(true);
    expect(suggestions.some((suggestion) => suggestion.type === "essential" && suggestion.title === "Docs Essential")).toBe(true);
    expect(suggestions.some((suggestion) => suggestion.type === "history" && suggestion.title === "Docs")).toBe(true);
  });

  it("prioritizes essentials for empty-start suggestions", () => {
    const state = createDefaultState();
    const workspace = getActiveWorkspace(state);
    state.essentials.push(createFavorite("Mail", "https://mail.example"));
    workspace.favorites.push(createFavorite("Docs", "https://docs.example"));

    const suggestionTypes = buildOmniboxSuggestions(state, "").map((suggestion) => suggestion.type);
    const firstFavoriteIndex = suggestionTypes.indexOf("favorite");
    const firstTabIndex = suggestionTypes.indexOf("tab");
    const lastEssentialIndex = suggestionTypes.lastIndexOf("essential");

    expect(firstFavoriteIndex).toBeGreaterThan(lastEssentialIndex);
    expect(firstTabIndex).toBeGreaterThan(lastEssentialIndex);
  });

  it("limits suggestion count for compact topbar rendering", () => {
    const state = createDefaultState();
    for (let index = 0; index < 20; index += 1) {
      getActiveWorkspace(state).favorites.push(createFavorite(`Favorite ${index}`, `https://favorite-${index}.example`));
    }

    expect(buildOmniboxSuggestions(state, "")).toHaveLength(8);
  });
});
