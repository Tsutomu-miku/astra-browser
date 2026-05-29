import { createElement, useEffect } from "react";
import { act } from "react";
import { createRoot } from "react-dom/client";
import { describe, expect, it, vi } from "vitest";

import { useBrowserShortcuts } from "../src/renderer/app/controller/useBrowserShortcuts";
import type { BrowserActions } from "../src/renderer/app/controller/useBrowserActions";
import type { ShortcutIntent } from "../src/renderer/common/shortcuts/keyboardShortcuts";
import { createDefaultState, createFavorite, createTab } from "../src/renderer/domain/browser";
import type { BrowserStore } from "../src/renderer/stores/browserStoreTypes";

describe("browser shortcuts", () => {
  it("navigates the active tab when an Alt number shortcut targets an Essential", () => {
    const essential = createFavorite("Mail", "https://mail.example");
    const state = createDefaultState();
    state.essentials = [essential];
    const activeWorkspace = state.workspaces[0];
    const actions = createActions();
    let runShortcut: ((intent: ShortcutIntent) => void) | undefined;

    const { container, root } = renderShortcutHarness({
      actions,
      activeWorkspace,
      onReady: (runner) => {
        runShortcut = runner;
      },
      store: createStore(state)
    });

    act(() => {
      runShortcut?.({ type: "selectTabIndex", index: 0 });
    });

    expect(actions.navigateActiveTab).toHaveBeenCalledWith(essential.url);
    expect(actions.openUrlInActiveWorkspace).not.toHaveBeenCalled();

    act(() => root.unmount());
    container.remove();
  });

  it("selects the last tab using sidebar visual order for Alt 9", () => {
    const state = createDefaultState();
    const favoriteTab = createTab("Favorite", "https://favorite.example");
    const firstRegular = createTab("Docs", "https://docs.example");
    const hiddenGrouped = { ...createTab("Hidden", "https://hidden.example"), groupId: "group" };
    const lastRegular = createTab("News", "https://news.example");
    const trailingPinned = { ...createTab("Pinned", "https://pinned.example"), isPinned: true };
    const activeWorkspace = {
      ...state.workspaces[0],
      tabGroups: [{
        color: "#7dd3fc",
        id: "group",
        isCollapsed: true,
        name: "Collapsed"
      }],
      favorites: [createFavorite("Favorite", favoriteTab.url, favoriteTab.id)],
      tabs: [firstRegular, hiddenGrouped, favoriteTab, lastRegular, trailingPinned]
    };
    const actions = createActions();
    let runShortcut: ((intent: ShortcutIntent) => void) | undefined;

    const { container, root } = renderShortcutHarness({
      actions,
      activeWorkspace,
      onReady: (runner) => {
        runShortcut = runner;
      },
      store: createStore({ ...state, workspaces: [activeWorkspace, ...state.workspaces.slice(1)] })
    });

    act(() => {
      runShortcut?.({ type: "selectLastTab" });
    });

    expect(actions.selectTab).toHaveBeenCalledWith(lastRegular.id);

    act(() => root.unmount());
    container.remove();
  });
});

function ShortcutHarness({
  actions,
  activeWorkspace,
  onReady,
  store
}: {
  actions: BrowserActions;
  activeWorkspace: BrowserStore["state"]["workspaces"][number];
  onReady: (runShortcut: (intent: ShortcutIntent) => void) => void;
  store: BrowserStore;
}) {
  const runShortcut = useBrowserShortcuts({
    actions,
    activeWebview: undefined,
    activeWorkspace,
    store
  });

  useEffect(() => {
    onReady(runShortcut);
  }, [onReady, runShortcut]);

  return null;
}

function renderShortcutHarness(props: Parameters<typeof ShortcutHarness>[0]) {
  const container = document.createElement("div");
  document.body.append(container);
  const root = createRoot(container);

  act(() => {
    root.render(createElement(ShortcutHarness, props));
  });

  return { container, root };
}

function createActions(): BrowserActions {
  return {
    navigateActiveTab: vi.fn(),
    openUrlInActiveWorkspace: vi.fn(),
    selectTab: vi.fn()
  } as unknown as BrowserActions;
}

function createStore(state: BrowserStore["state"]): BrowserStore {
  return {
    findQuery: "",
    state
  } as unknown as BrowserStore;
}
