import { describe, expect, it, vi } from "vitest";

import { createDefaultState, createFavorite, createTab } from "../src/renderer/domain/browser";
import { buildContentCommands } from "../src/renderer/surfaces/command/model/commandContentEntries";
import type { CommandActions } from "../src/renderer/surfaces/command/model/commandTypes";

describe("command content entries", () => {
  it("navigates Essentials in the active tab and selects existing Favorite tabs", () => {
    const state = createDefaultState();
    const docsTab = createTab("Docs", "https://docs.example");
    const mailTab = createTab("Mail", "https://mail.example");
    const essential = createFavorite("Docs", docsTab.url, docsTab.id);
    const favorite = createFavorite("Mail", mailTab.url, mailTab.id);
    const workspace = {
      ...state.workspaces[0],
      favorites: [favorite],
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
    const favorite = createFavorite("Mail", mailTab.url, mailTab.id);
    const workspace = {
      ...state.workspaces[0],
      favorites: [favorite],
      tabs: [mailTab]
    };
    const actions = createActions();

    buildContentCommands(state, workspace, actions)
      .find((command) => command.subtitle.startsWith("Favorite"))
      ?.runInSplit?.();

    expect(actions.openTabInSplit).toHaveBeenCalledWith(mailTab.id);
    expect(actions.openUrlInSplit).not.toHaveBeenCalled();
  });

  it("falls back by URL when a Favorite tab id is stale", () => {
    const state = createDefaultState();
    const docsTab = createTab("Docs", "https://docs.example");
    const favorite = createFavorite("Docs", docsTab.url, "missing-tab");
    const workspace = {
      ...state.workspaces[0],
      favorites: [favorite],
      tabs: [docsTab]
    };
    const actions = createActions();

    buildContentCommands(state, workspace, actions)
      .find((command) => command.subtitle.startsWith("Favorite"))
      ?.run();

    expect(actions.selectTab).toHaveBeenCalledWith(docsTab.id);
    expect(actions.openUrlInActiveWorkspace).not.toHaveBeenCalled();
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
    openTabInSplit: ReturnType<typeof vi.fn>;
    openUrlInActiveWorkspace: ReturnType<typeof vi.fn>;
    openUrlInSplit: ReturnType<typeof vi.fn>;
    selectTab: ReturnType<typeof vi.fn>;
  };
}
