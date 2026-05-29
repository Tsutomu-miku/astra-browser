import { createElement } from "react";
import { act } from "react";
import { createRoot } from "react-dom/client";
import { describe, expect, it, vi } from "vitest";

import type { BrowserController } from "../src/renderer/app/controller/types";
import { createDefaultState } from "../src/renderer/domain/browser";
import { SidebarContextMenus } from "../src/renderer/surfaces/sidebar/components/tabs/SidebarContextMenus";
import { useSidebarContextMenus } from "../src/renderer/surfaces/sidebar/components/tabs/useSidebarContextMenus";

describe("sidebar context menu focus", () => {
  it("restores focus to the trigger when a sidebar context menu closes with Escape", () => {
    const container = document.createElement("div");
    document.body.append(container);
    const root = createRoot(container);

    act(() => {
      root.render(createElement(ContextMenuHarness));
    });

    const trigger = container.querySelector<HTMLButtonElement>(".context-trigger")!;
    trigger.focus();

    act(() => {
      trigger.dispatchEvent(new MouseEvent("contextmenu", {
        bubbles: true,
        clientX: 10,
        clientY: 10
      }));
    });

    expect(container.querySelector(".tab-context-menu")).not.toBeNull();

    act(() => {
      window.dispatchEvent(new KeyboardEvent("keydown", { key: "Escape" }));
    });

    expect(container.querySelector(".tab-context-menu")).toBeNull();
    expect(document.activeElement).toBe(trigger);

    act(() => root.unmount());
    container.remove();
  });

  it("restores focus to the trigger after running a sidebar context menu action", () => {
    const container = document.createElement("div");
    document.body.append(container);
    const root = createRoot(container);

    act(() => {
      root.render(createElement(ContextMenuHarness));
    });

    const trigger = container.querySelector<HTMLButtonElement>(".context-trigger")!;
    trigger.focus();

    act(() => {
      trigger.dispatchEvent(new MouseEvent("contextmenu", {
        bubbles: true,
        clientX: 10,
        clientY: 10
      }));
    });

    act(() => {
      container.querySelector<HTMLButtonElement>(".tab-context-menu button")?.dispatchEvent(new MouseEvent("click", {
        bubbles: true
      }));
    });

    expect(container.querySelector(".tab-context-menu")).toBeNull();
    expect(document.activeElement).toBe(trigger);

    act(() => root.unmount());
    container.remove();
  });
});

function ContextMenuHarness() {
  const state = createDefaultState();
  const activeWorkspace = state.workspaces[0];
  const tab = activeWorkspace.tabs[0];
  const menus = useSidebarContextMenus();

  return createElement("div", null,
    createElement("button", {
      className: "context-trigger",
      type: "button",
      onContextMenu: (event) => menus.openTabMenu(event, tab)
    }, "Open menu"),
    createElement(SidebarContextMenus, {
      actions: createActions(),
      activeWorkspace,
      closedTabMenu: menus.closedTabMenu,
      closeMenus: menus.closeMenus,
      quickEntryMenu: menus.quickEntryMenu,
      state,
      tabGroupMenu: menus.tabGroupMenu,
      tabMenu: menus.tabMenu
    })
  );
}

function createActions() {
  return {
    assignTabToGroup: vi.fn(),
    closeOtherTabs: vi.fn(),
    closeTab: vi.fn(),
    closeTabsToLeft: vi.fn(),
    closeTabsToRight: vi.fn(),
    copyText: vi.fn(),
    duplicateTab: vi.fn(),
    groupTab: vi.fn(),
    moveTabToWorkspace: vi.fn(),
    openGlance: vi.fn(),
    openTabInSplit: vi.fn(),
    selectTab: vi.fn(),
    sleepTab: vi.fn(),
    toggleTabEssential: vi.fn(),
    toggleTabFavorite: vi.fn(),
    toggleTabMuted: vi.fn(),
    toggleTabPinned: vi.fn(),
    ungroupTab: vi.fn()
  } as unknown as BrowserController["actions"];
}
