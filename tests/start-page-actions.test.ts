import { createElement } from "react";
import { act } from "react";
import { createRoot } from "react-dom/client";
import { describe, expect, it, vi } from "vitest";

import type { BrowserController } from "../src/renderer/app/controller/types";
import { NEUTRAL_CHROME_ACCENT } from "../src/renderer/common/theme/chromeTheme";
import { createDefaultState, createTab, type BrowserState, type Workspace } from "../src/renderer/domain/browser";
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

  it("opens tab-backed Favorites in split by tab id from tiles and context menus", () => {
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

    clickFavoriteTile(container, favorite.title, { shiftKey: true });
    openFavoriteContextMenu(container, favorite.title);
    clickMenuButton(container, "Open in split view");

    expect(actions.openTabInSplit).toHaveBeenCalledTimes(2);
    expect(actions.openTabInSplit).toHaveBeenCalledWith(favorite.tabId);
    expect(actions.openUrlInSplit).not.toHaveBeenCalled();

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

  it("uses the global chrome accent mode on the new tab surface", () => {
    const state = createDefaultState();
    const activeWorkspace = state.workspaces[0];
    activeWorkspace.accent = "#f0abfc";
    const actions = createActions();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(StartPage, {
        controller: createController(state, activeWorkspace, actions),
        isVisible: true
      }));
    });

    expect(container.querySelector<HTMLElement>(".start-page-shell")?.style.getPropertyValue("--start-accent")).toBe(NEUTRAL_CHROME_ACCENT);

    act(() => {
      state.settings.chromeAccentMode = "space";
      root.render(createElement(StartPage, {
        controller: createController(state, activeWorkspace, actions),
        isVisible: true
      }));
    });

    expect(container.querySelector<HTMLElement>(".start-page-shell")?.style.getPropertyValue("--start-accent")).toBe(activeWorkspace.accent);

    act(() => root.unmount());
  });

  it("renders Start tiles with cached site favicons instead of standalone initials", () => {
    const state = createDefaultState();
    const activeWorkspace = state.workspaces[0];
    const docsTab = createTab("Docs", "https://docs.example/page");
    docsTab.isFavorite = true;
    activeWorkspace.tabs = [docsTab];
    activeWorkspace.favoriteOrder = [docsTab.id];
    state.essentials = [];
    state.history = [];
    state.faviconCache = {
      "https://docs.example": "https://docs.example/favicon.ico"
    };
    const actions = createActions();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(StartPage, {
        controller: createController(state, activeWorkspace, actions),
        isVisible: true
      }));
    });

    const iconImage = container.querySelector<HTMLImageElement>(".start-tile .sidebar-item-icon-image");
    expect(iconImage?.getAttribute("src")).toBe("https://docs.example/favicon.ico");
    expect(container.querySelector(".start-tile-icon")?.getAttribute("data-icon-kind")).toBe("web");

    act(() => root.unmount());
  });
});

function createStateWithDuplicateFavoriteUrl() {
  const state = createDefaultState();
  const activeWorkspace = state.workspaces[0];
  const firstTab = createTab("Docs original", "https://docs.example/");
  const secondTab = createTab("Docs selected", "https://docs.example/");
  secondTab.isFavorite = true;
  const favorite = { id: secondTab.id, tabId: secondTab.id, title: secondTab.title, url: secondTab.url };

  activeWorkspace.tabs = [firstTab, secondTab];
  activeWorkspace.activeTabId = firstTab.id;
  activeWorkspace.favoriteOrder = [secondTab.id];
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

function clickFavoriteTile(container: HTMLElement, title: string, options: MouseEventInit = {}) {
  const tile = Array.from(container.querySelectorAll<HTMLButtonElement>(".start-tile"))
    .find((button) => button.textContent?.includes(title));
  act(() => {
    tile?.dispatchEvent(new MouseEvent("click", {
      bubbles: true,
      ...options
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
