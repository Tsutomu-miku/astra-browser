import { describe, expect, it, vi } from "vitest";

import { createDefaultState, createFavorite, createTab } from "../src/renderer/domain/browser";
import { buildContentCommands } from "../src/renderer/surfaces/command/model/commandContentEntries";
import type { CommandActions } from "../src/renderer/surfaces/command/model/commandTypes";

describe("command content entries", () => {
  it("navigates Essentials in the active tab and selects existing Favorite tabs", () => {
    const state = createDefaultState();
    const docsTab = createTab("Docs", "https://docs.example");
    const mailTab = createTab("Mail", "https://mail.example");
    mailTab.isFavorite = true;
    const essential = createFavorite("Docs", docsTab.url, docsTab.id);
    const workspace = {
      ...state.workspaces[0],
      favoriteOrder: [mailTab.id],
      tabs: [docsTab, mailTab]
    };
    state.essentials = [essential];
    const actions = createActions();

    const commands = buildContentCommands(state, workspace, actions);
    commands.find((command) => command.subtitle.startsWith("Essential"))?.run();
    commands.find((command) => command.subtitle.startsWith("Favorite"))?.run();

    expect(actions.navigateActiveTab).toHaveBeenCalledWith(docsTab.url);
    expect(actions.selectTab).toHaveBeenCalledWith(mailTab.id);
    expect(actions.openUrlInActiveWorkspace).not.toHaveBeenCalled();
  });

  it("opens tab-backed Favorites in split by tab identity", () => {
    const state = createDefaultState();
    const mailTab = createTab("Mail", "https://mail.example");
    mailTab.isFavorite = true;
    const workspace = {
      ...state.workspaces[0],
      favoriteOrder: [mailTab.id],
      tabs: [mailTab]
    };
    const actions = createActions();

    buildContentCommands(state, workspace, actions)
      .find((command) => command.subtitle.startsWith("Favorite"))
      ?.runInSplit?.();

    expect(actions.openTabInSplit).toHaveBeenCalledWith(mailTab.id);
    expect(actions.openUrlInSplit).not.toHaveBeenCalled();
  });

  it("keeps stale tab-backed Favorites from claiming unrelated matching tabs", () => {
    const state = createDefaultState();
    const docsTab = createTab("Docs", "https://docs.example");
    const workspace = {
      ...state.workspaces[0],
      favoriteOrder: ["missing-tab"],
      tabs: [docsTab]
    };
    const actions = createActions();

    const favoriteCommands = buildContentCommands(state, workspace, actions)
      .filter((command) => command.subtitle.startsWith("Favorite"));

    // Stale favoriteOrder entries that do not resolve to valid favorite tabs
    // produce no favorite commands and never claim unrelated tabs.
    expect(favoriteCommands).toHaveLength(0);
    expect(actions.selectTab).not.toHaveBeenCalled();
  });

  it("uses backing tab data for Favorite command display and preview", () => {
    const state = createDefaultState();
    const tab = createTab("Current Docs", "https://docs.example/current");
    tab.isFavorite = true;
    const workspace = {
      ...state.workspaces[0],
      favoriteOrder: [tab.id],
      tabs: [tab]
    };
    const actions = createActions();

    const command = buildContentCommands(state, workspace, actions)
      .find((candidate) => candidate.subtitle.startsWith("Favorite tab"));

    expect(command).toMatchObject({
      subtitle: `Favorite tab · ${tab.url}`,
      title: tab.title
    });

    command?.runPreview?.();

    expect(actions.openGlance).toHaveBeenCalledWith(tab.url, tab.title);
  });
});

function createActions() {
  return {
    navigateActiveTab: vi.fn(),
    openGlance: vi.fn(),
    openTabInSplit: vi.fn(),
    openUrlInActiveWorkspace: vi.fn(),
    openUrlInSplit: vi.fn(),
    selectTab: vi.fn()
  } as unknown as CommandActions & {
    navigateActiveTab: ReturnType<typeof vi.fn>;
    openGlance: ReturnType<typeof vi.fn>;
    openTabInSplit: ReturnType<typeof vi.fn>;
    openUrlInActiveWorkspace: ReturnType<typeof vi.fn>;
    openUrlInSplit: ReturnType<typeof vi.fn>;
    selectTab: ReturnType<typeof vi.fn>;
  };
}
