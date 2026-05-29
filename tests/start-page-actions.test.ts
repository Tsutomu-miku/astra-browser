import { createElement } from "react";
import { act } from "react";
import { createRoot } from "react-dom/client";
import { describe, expect, it, vi } from "vitest";

import type { BrowserController } from "../src/renderer/app/controller/types";
import { createDefaultState, createFavorite, createTab, type BrowserState, type Workspace } from "../src/renderer/domain/browser";
import { StartPage } from "../src/renderer/surfaces/start/StartPage";

describe("StartPage actions", () => {
  it("opens Favorite context-menu items by tab id before URL fallback", () => {
    const { activeWorkspace, favorite, state } = createStateWithDuplicateFavoriteUrl();
    const actions = createActions();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(StartPage, {
        controller: createController(state, activeWorkspace, actions),
        isVisible: false
      }));
    });

    openFavoriteContextMenu(container, favorite.title);
    clickMenuButton(container, "Open");

    expect(actions.selectTab).toHaveBeenCalledWith(favorite.tabId);
    expect(actions.selectTab).not.toHaveBeenCalledWith(activeWorkspace.tabs[0].id);
    expect(actions.openUrlInActiveWorkspace).not.toHaveBeenCalled();

    act(() => root.unmount());
  });

  it("removes Favorite context-menu items by id", () => {
    const { activeWorkspace, favorite, state } = createStateWithDuplicateFavoriteUrl();
    const actions = createActions();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(StartPage, {
        controller: createController(state, activeWorkspace, actions),
        isVisible: false
      }));
    });

    openFavoriteContextMenu(container, favorite.title);
    clickMenuButton(container, "Remove Favorite");

    expect(actions.removeWorkspaceFavorite).toHaveBeenCalledWith(favorite.id);
    expect(actions.removeWorkspaceFavorite).not.toHaveBeenCalledWith(favorite.url);

    act(() => root.unmount());
  });
});

function createStateWithDuplicateFavoriteUrl() {
  const state = createDefaultState();
  const activeWorkspace = state.workspaces[0];
  const firstTab = createTab("Docs original", "https://docs.example/");
  const secondTab = createTab("Docs selected", "https://docs.example/");
  const favorite = createFavorite("Docs selected", secondTab.url, secondTab.id);

  activeWorkspace.tabs = [firstTab, secondTab];
  activeWorkspace.activeTabId = firstTab.id;
  activeWorkspace.favorites = [favorite];
  state.essentials = [];
  state.history = [];

  return { activeWorkspace, favorite, state };
}

function createActions() {
  return {
    navigateActiveTab: vi.fn(),
    openGlance: vi.fn(),
    openTabInSplit: vi.fn(),
    openUrlInActiveWorkspace: vi.fn(),
    openUrlInSplit: vi.fn(),
    removeEssential: vi.fn(),
    removeHistoryEntry: vi.fn(),
    removeWorkspaceFavorite: vi.fn(),
    selectTab: vi.fn()
  };
}

function createController(
  state: BrowserState,
  activeWorkspace: Workspace,
  actions: ReturnType<typeof createActions>
): BrowserController {
  return {
    actions,
    activeWorkspace,
    state
  } as unknown as BrowserController;
}

function openFavoriteContextMenu(container: HTMLElement, title: string) {
  const tile = Array.from(container.querySelectorAll<HTMLButtonElement>(".start-tile"))
    .find((button) => button.textContent?.includes(title));
  act(() => {
    tile?.dispatchEvent(new MouseEvent("contextmenu", {
      bubbles: true,
      button: 2,
      clientX: 16,
      clientY: 24
    }));
  });
}

function clickMenuButton(container: HTMLElement, label: string) {
  const button = Array.from(container.querySelectorAll<HTMLButtonElement>(".start-context-menu button"))
    .find((button) => button.textContent === label);
  act(() => {
    button?.dispatchEvent(new MouseEvent("click", { bubbles: true }));
  });
}
