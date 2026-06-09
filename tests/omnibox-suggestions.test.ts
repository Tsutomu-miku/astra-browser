import { describe, expect, it } from "vitest";

import {
  createDefaultState,
  createFavorite,
  createTab
} from "../src/renderer/domain/browser";
import {
  openUrlInActiveWorkspace,
  recordHistory
} from "../src/renderer/domain/actions";
import { getActiveTab, getActiveWorkspace } from "../src/renderer/domain/browser/selectors";
import {
  buildOmniboxSuggestions,
  getOmniboxInlineCompletion
} from "../src/renderer/common/omnibox/omniboxSuggestions";

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
    const favoriteTab = getActiveTab(getActiveWorkspace(withHistory));
    favoriteTab.isFavorite = true;
    getActiveWorkspace(withHistory).favoriteOrder.unshift(favoriteTab.id);
    withHistory.essentials.push(createFavorite("Docs Essential", "https://docs.example/essential"));
    const suggestions = buildOmniboxSuggestions(withHistory, "docs");

    expect(suggestions.some((suggestion) => suggestion.type === "tab" && suggestion.title === "Docs")).toBe(true);
    expect(suggestions.some((suggestion) => (
      suggestion.type === "favorite" &&
      suggestion.title === "Docs" &&
      suggestion.subtitle === `Favorite tab · ${favoriteTab.url}` &&
      suggestion.tabId === favoriteTab.id
    ))).toBe(true);
    expect(suggestions.some((suggestion) => suggestion.type === "essential" && suggestion.title === "Docs Essential")).toBe(true);
    expect(suggestions.some((suggestion) => suggestion.type === "history" && suggestion.title === "Docs")).toBe(true);
  });

  it("ignores favoriteOrder entries that do not resolve to existing favorite tabs", () => {
    const opened = openUrlInActiveWorkspace(createDefaultState(), "docs.example", "Docs");
    const workspace = getActiveWorkspace(opened);
    // The "Docs" tab is not marked as favorite; add a non-existent id to favoriteOrder
    workspace.favoriteOrder.unshift("missing-tab");

    const favoriteSuggestions = buildOmniboxSuggestions(opened, "docs")
      .filter((suggestion) => suggestion.type === "favorite");

    // Invalid entries are skipped and non-favorite tabs are not surfaced as favorites
    expect(favoriteSuggestions).toHaveLength(0);
  });

  it("matches tab-backed Favorites by current backing tab title and URL", () => {
    const opened = openUrlInActiveWorkspace(createDefaultState(), "docs.example/current", "Current Project Brief");
    const tab = getActiveTab(getActiveWorkspace(opened));
    tab.isFavorite = true;
    getActiveWorkspace(opened).favoriteOrder.push(tab.id);

    const suggestion = buildOmniboxSuggestions(opened, "brief")
      .find((candidate) => candidate.type === "favorite");

    expect(suggestion).toMatchObject({
      completion: "docs.example/current",
      subtitle: `Favorite tab · ${tab.url}`,
      tabId: tab.id,
      title: "Current Project Brief",
      type: "favorite",
      url: tab.url
    });
  });

  it("prioritizes essentials for empty-start suggestions", () => {
    const state = createDefaultState();
    const workspace = getActiveWorkspace(state);
    state.essentials.push(createFavorite("Mail", "https://mail.example"));
    const docsTab = createTab("Docs", "https://docs.example");
    docsTab.isFavorite = true;
    workspace.tabs.push(docsTab);
    workspace.favoriteOrder.push(docsTab.id);

    const suggestionTypes = buildOmniboxSuggestions(state, "").map((suggestion) => suggestion.type);
    const firstFavoriteIndex = suggestionTypes.indexOf("favorite");
    const firstTabIndex = suggestionTypes.indexOf("tab");
    const lastEssentialIndex = suggestionTypes.lastIndexOf("essential");

    expect(firstFavoriteIndex).toBeGreaterThan(lastEssentialIndex);
    expect(firstTabIndex).toBeGreaterThan(lastEssentialIndex);
  });

  it("limits suggestion count for compact topbar rendering", () => {
    const state = createDefaultState();
    const workspace = getActiveWorkspace(state);
    for (let index = 0; index < 20; index += 1) {
      const tab = createTab(`Favorite ${index}`, `https://favorite-${index}.example`);
      tab.isFavorite = true;
      workspace.tabs.push(tab);
      workspace.favoriteOrder.push(tab.id);
    }

    expect(buildOmniboxSuggestions(state, "")).toHaveLength(8);
  });

  it("ranks host and title prefix matches ahead of weaker history matches", () => {
    const state = createDefaultState();
    state.history.unshift({
      id: "history-weak",
      title: "Old example",
      url: "https://example.test/articles/github",
      visitedAt: Date.now(),
      workspaceId: "personal"
    });

    const suggestions = buildOmniboxSuggestions(state, "git");

    expect(suggestions[1]).toMatchObject({
      completion: "github.com",
      title: "GitHub",
      type: "essential"
    });
  });

  it("returns inline completion for URL-backed suggestions", () => {
    const suggestions = buildOmniboxSuggestions(createDefaultState(), "git");

    expect(getOmniboxInlineCompletion(suggestions, "git")).toEqual({
      suggestionId: suggestions[1].id,
      suffix: "hub.com",
      value: "github.com"
    });
    expect(getOmniboxInlineCompletion(suggestions, "git hub")).toBeNull();
  });

  it("returns inline completion for title prefixes when the URL does not start with the query", () => {
    const state = createDefaultState();
    const linearTab = createTab("Linear Planning", "https://docs.example/linear");
    linearTab.isFavorite = true;
    const workspace = getActiveWorkspace(state);
    workspace.tabs.unshift(linearTab);
    workspace.favoriteOrder.unshift(linearTab.id);
    const suggestions = buildOmniboxSuggestions(state, "lin");

    const favorite = suggestions.find((suggestion) => suggestion.type === "favorite" && suggestion.title === "Linear Planning")!;
    expect(getOmniboxInlineCompletion(suggestions, "lin")).toEqual({
      suggestionId: favorite.id,
      suffix: "ear Planning",
      value: "Linear Planning"
    });
  });

  it("matches title acronyms for compact browser-style lookups", () => {
    const state = createDefaultState();
    const issuesTab = createTab("GitHub Issues", "https://issues.example");
    issuesTab.isFavorite = true;
    const workspace = getActiveWorkspace(state);
    workspace.tabs.unshift(issuesTab);
    workspace.favoriteOrder.unshift(issuesTab.id);
    const suggestions = buildOmniboxSuggestions(state, "ghi");

    expect(suggestions[1]).toMatchObject({
      title: "GitHub Issues",
      type: "favorite"
    });
  });
});
