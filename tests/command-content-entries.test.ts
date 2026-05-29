import { describe, expect, it, vi } from "vitest";

import { createDefaultState, createFavorite } from "../src/renderer/domain/browser";
import { buildContentCommands } from "../src/renderer/surfaces/command/model/commandContentEntries";
import type { CommandActions } from "../src/renderer/surfaces/command/model/commandTypes";

describe("command content entries", () => {
  it("navigates the current tab for Essentials and Favorites", () => {
    const state = createDefaultState();
    const essential = createFavorite("Docs", "https://docs.example");
    const favorite = createFavorite("Mail", "https://mail.example");
    const workspace = {
      ...state.workspaces[0],
      favorites: [favorite]
    };
    state.essentials = [essential];
    const actions = createActions();

    const commands = buildContentCommands(state, workspace, actions);
    commands.find((command) => command.subtitle.startsWith("Essential"))?.run();
    commands.find((command) => command.subtitle.startsWith("Favorite"))?.run();

    expect(actions.navigateActiveTab).toHaveBeenCalledWith(essential.url);
    expect(actions.navigateActiveTab).toHaveBeenCalledWith(favorite.url);
    expect(actions.openUrlInActiveWorkspace).not.toHaveBeenCalled();
  });
});

function createActions() {
  return {
    navigateActiveTab: vi.fn(),
    openGlance: vi.fn(),
    openUrlInActiveWorkspace: vi.fn(),
    openUrlInSplit: vi.fn()
  } as unknown as CommandActions & {
    navigateActiveTab: ReturnType<typeof vi.fn>;
    openUrlInActiveWorkspace: ReturnType<typeof vi.fn>;
  };
}
