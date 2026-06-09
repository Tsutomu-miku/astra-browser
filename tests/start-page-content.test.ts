import { describe, expect, it } from "vitest";

import { createDefaultState, createFavorite, createTab } from "../src/renderer/domain/browser";
import { getActiveWorkspace } from "../src/renderer/domain/browser/selectors";
import { getStartPageContent } from "../src/renderer/surfaces/start/startPageContent";

describe("getStartPageContent", () => {
  it("returns global Essentials plus active Space favorites and history", () => {
    const state = createDefaultState();
    const workspace = getActiveWorkspace(state);
    state.essentials.push(createFavorite("Mail", "https://mail.example"));
    const docsTab = createTab("Docs", "https://docs.example");
    docsTab.isFavorite = true;
    workspace.tabs.push(docsTab);
    workspace.favoriteOrder.push(docsTab.id);
    state.history.push(
      {
        id: "active-history",
        title: "Active history",
        url: "https://active.example",
        visitedAt: 2,
        workspaceId: workspace.id
      },
      {
        id: "other-history",
        title: "Other history",
        url: "https://other.example",
        visitedAt: 1,
        workspaceId: "other"
      }
    );

    const content = getStartPageContent(state, workspace);

    expect(content.essentials.map((item) => item.title)).toContain("Mail");
    expect(content.favorites.map((item) => item.title)).toContain("Docs");
    expect(content.recentHistory.map((entry) => entry.title)).toEqual(["Active history"]);
  });

  it("uses backing tab data for Space favorite tiles", () => {
    const state = createDefaultState();
    const workspace = getActiveWorkspace(state);
    const tab = createTab("Current Docs", "https://docs.example/current");
    tab.isFavorite = true;
    workspace.tabs = [tab];
    workspace.favoriteOrder = [tab.id];

    const content = getStartPageContent(state, workspace);

    expect(content.favorites[0]).toMatchObject({
      tabId: tab.id,
      title: tab.title,
      url: tab.url
    });
  });
});
