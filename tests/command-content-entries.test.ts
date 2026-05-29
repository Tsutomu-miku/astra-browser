import { describe, expect, it, vi } from "vitest";

import { createDefaultState, createFavorite, createTab } from "../src/renderer/domain/browser";
import { buildContentCommands } from "../src/renderer/surfaces/command/model/commandContentEntries";
import type { CommandActions } from "../src/renderer/surfaces/command/model/commandTypes";

describe("command content entries", () => {
  it("selects existing tabs for Essentials and Favorites", () => {
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

    expect(actions.selectTab).toHaveBeenCalledWith(docsTab.id);
    expect(actions.selectTab).toHaveBeenCalledWith(mailTab.id);
    expect(actions.openUrlInActiveWorkspace).not.toHaveBeenCalled();
  });
});

function createActions() {
  return {
    openGlance: vi.fn(),
    openUrlInActiveWorkspace: vi.fn(),
    openUrlInSplit: vi.fn(),
    selectTab: vi.fn()
  } as unknown as CommandActions & {
    openUrlInActiveWorkspace: ReturnType<typeof vi.fn>;
    selectTab: ReturnType<typeof vi.fn>;
  };
}
