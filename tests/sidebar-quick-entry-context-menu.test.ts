import { createElement } from "react";
import { act } from "react";
import { createRoot } from "react-dom/client";
import { renderToStaticMarkup } from "react-dom/server";
import { readFileSync } from "node:fs";
import { join } from "node:path";
import { describe, expect, it, vi } from "vitest";

import { createDefaultState, createFavorite } from "../src/renderer/domain/browser";
import type { BrowserController } from "../src/renderer/app/controller/types";
import { QuickEntryContextMenu } from "../src/renderer/surfaces/sidebar/components/tabs/QuickEntryContextMenu";
import { SidebarContextMenus } from "../src/renderer/surfaces/sidebar/components/tabs/SidebarContextMenus";

const contextMenuCss = readFileSync(join(__dirname, "../src/renderer/styles/sidebar-context-menu.css"), "utf8");

describe("sidebar quick entry context menu", () => {
  it("renders Essential actions for open, preview, split, and removal", () => {
    const html = renderToStaticMarkup(createElement(QuickEntryContextMenu, {
      item: createFavorite("Docs", "https://docs.example"),
      kind: "essential",
      left: 10,
      top: 20,
      onClose: vi.fn(),
      onCopyText: vi.fn(),
      onMoveToNewWorkspace: vi.fn(),
      onMoveToWorkspace: vi.fn(),
      onOpen: vi.fn(),
      onOpenInSplit: vi.fn(),
      onPreview: vi.fn(),
      onRemove: vi.fn()
    }));

    expect(html).toContain('role="menu"');
    expect(html).toContain("Open");
    expect(html).toContain("Preview in Glance");
    expect(html).toContain("Open in split view");
    expect(html).toContain("Copy URL");
    expect(html).toContain("Copy title");
    expect(html).toContain("Remove Essential");
    expect(html).not.toContain("Move to New Space");
  });

  it("copies quick entry URL and title", () => {
    const onCopyText = vi.fn();
    const menu = createElement(QuickEntryContextMenu, {
      item: createFavorite("Docs", "https://docs.example"),
      kind: "favorite",
      left: 10,
      top: 20,
      onClose: vi.fn(),
      onCopyText,
      onOpen: vi.fn(),
      onOpenInSplit: vi.fn(),
      onPreview: vi.fn(),
      onRemove: vi.fn()
    });

    menu.props.onCopyText(menu.props.item.url);
    menu.props.onCopyText(menu.props.item.title || menu.props.item.url);

    expect(renderToStaticMarkup(menu)).toContain("Copy URL");
    expect(onCopyText).toHaveBeenCalledWith("https://docs.example");
    expect(onCopyText).toHaveBeenCalledWith("Docs");
  });

  it("renders Favorite removal copy", () => {
    const html = renderToStaticMarkup(createElement(QuickEntryContextMenu, {
      item: createFavorite("Docs", "https://docs.example"),
      kind: "favorite",
      left: 10,
      moveWorkspaceTargets: [{ id: "work", name: "Work" }],
      top: 20,
      onClose: vi.fn(),
      onCopyText: vi.fn(),
      onMoveToNewWorkspace: vi.fn(),
      onMoveToWorkspace: vi.fn(),
      onOpen: vi.fn(),
      onOpenInSplit: vi.fn(),
      onPreview: vi.fn(),
      onRemove: vi.fn()
    }));

    expect(html).toContain("Remove Favorite");
    expect(html).toContain("Move to Work");
    expect(html).toContain("Move to New Space");
  });

  it("moves favorites to another Space from the context menu", () => {
    const favorite = createFavorite("Docs", "https://docs.example");
    const onClose = vi.fn();
    const onMoveToWorkspace = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(QuickEntryContextMenu, {
        item: favorite,
        kind: "favorite",
        left: 10,
        moveWorkspaceTargets: [{ id: "work", name: "Work" }],
        top: 20,
        onClose,
        onCopyText: vi.fn(),
        onMoveToNewWorkspace: vi.fn(),
        onMoveToWorkspace,
        onOpen: vi.fn(),
        onOpenInSplit: vi.fn(),
        onPreview: vi.fn(),
        onRemove: vi.fn()
      }));
    });

    Array.from(container.querySelectorAll("button"))
      .find((button) => button.textContent === "Move to Work")
      ?.dispatchEvent(new MouseEvent("click", { bubbles: true }));

    expect(onMoveToWorkspace).toHaveBeenCalledWith(favorite.id, "work");
    expect(onClose).toHaveBeenCalled();

    act(() => root.unmount());
  });

  it("removes favorites by item id from the context menu", () => {
    const favorite = createFavorite("Docs", "https://docs.example");
    const onClose = vi.fn();
    const onRemove = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(QuickEntryContextMenu, {
        item: favorite,
        kind: "favorite",
        left: 10,
        top: 20,
        onClose,
        onCopyText: vi.fn(),
        onOpen: vi.fn(),
        onOpenInSplit: vi.fn(),
        onPreview: vi.fn(),
        onRemove
      }));
    });

    Array.from(container.querySelectorAll("button"))
      .find((button) => button.textContent === "Remove Favorite")
      ?.dispatchEvent(new MouseEvent("click", { bubbles: true }));

    expect(onRemove).toHaveBeenCalledWith(favorite.id);
    expect(onClose).toHaveBeenCalled();

    act(() => root.unmount());
  });

  it("removes Essentials by URL from the context menu", () => {
    const essential = createFavorite("Docs", "https://docs.example");
    const onRemove = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(QuickEntryContextMenu, {
        item: essential,
        kind: "essential",
        left: 10,
        top: 20,
        onClose: vi.fn(),
        onCopyText: vi.fn(),
        onOpen: vi.fn(),
        onOpenInSplit: vi.fn(),
        onPreview: vi.fn(),
        onRemove
      }));
    });

    Array.from(container.querySelectorAll("button"))
      .find((button) => button.textContent === "Remove Essential")
      ?.dispatchEvent(new MouseEvent("click", { bubbles: true }));

    expect(onRemove).toHaveBeenCalledWith(essential.url);

    act(() => root.unmount());
  });

  it("focuses and navigates quick entry menu actions from the keyboard", () => {
    const container = document.createElement("div");
    document.body.append(container);
    const root = createRoot(container);

    act(() => {
      root.render(createElement(QuickEntryContextMenu, {
        item: createFavorite("Docs", "https://docs.example"),
        kind: "favorite",
        left: 10,
        moveWorkspaceTargets: [{ id: "work", name: "Work" }],
        top: 20,
        onClose: vi.fn(),
        onCopyText: vi.fn(),
        onMoveToNewWorkspace: vi.fn(),
        onMoveToWorkspace: vi.fn(),
        onOpen: vi.fn(),
        onOpenInSplit: vi.fn(),
        onPreview: vi.fn(),
        onRemove: vi.fn()
      }));
    });

    expect(document.activeElement?.textContent).toBe("Open");

    act(() => {
      document.activeElement?.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "ArrowDown" }));
    });
    expect(document.activeElement?.textContent).toBe("Preview in Glance");

    act(() => {
      document.activeElement?.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "End" }));
    });
    expect(document.activeElement?.textContent).toBe("Remove Favorite");

    act(() => {
      document.activeElement?.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "Home" }));
    });
    expect(document.activeElement?.textContent).toBe("Open");

    act(() => root.unmount());
    container.remove();
  });

  it("selects matching tabs from the sidebar quick entry context menu", () => {
    const state = createDefaultState();
    const activeWorkspace = state.workspaces[0];
    const tab = activeWorkspace.tabs[0];
    tab.title = "Docs";
    tab.url = "https://docs.example/";
    const item = createFavorite("Docs", tab.url, tab.id);
    const actions = createActions();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(SidebarContextMenus, {
        actions,
        activeWorkspace,
        closedTabMenu: null,
        closeMenus: vi.fn(),
        quickEntryMenu: {
          item,
          kind: "favorite",
          left: 10,
          top: 20
        },
        state,
        tabGroupMenu: null,
        tabMenu: null
      }));
    });

    container.querySelector(".quick-entry-context-menu button")?.dispatchEvent(new MouseEvent("click", { bubbles: true }));

    expect(actions.selectTab).toHaveBeenCalledWith(tab.id);
    expect(actions.navigateActiveTab).not.toHaveBeenCalled();
    expect(actions.openUrlInActiveWorkspace).not.toHaveBeenCalled();

    act(() => root.unmount());
  });

  it("selects Favorite tabs by tab id before URL fallback from the sidebar context menu", () => {
    const state = createDefaultState();
    const activeWorkspace = state.workspaces[0];
    const firstTab = activeWorkspace.tabs[0];
    firstTab.title = "Docs original";
    firstTab.url = "https://docs.example/";
    const secondTab = {
      ...firstTab,
      id: "docs-second",
      title: "Docs selected"
    };
    activeWorkspace.tabs.push(secondTab);
    const item = createFavorite("Docs", secondTab.url, secondTab.id);
    const actions = createActions();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(SidebarContextMenus, {
        actions,
        activeWorkspace,
        closedTabMenu: null,
        closeMenus: vi.fn(),
        quickEntryMenu: {
          item,
          kind: "favorite",
          left: 10,
          top: 20
        },
        state,
        tabGroupMenu: null,
        tabMenu: null
      }));
    });

    container.querySelector(".quick-entry-context-menu button")?.dispatchEvent(new MouseEvent("click", { bubbles: true }));

    expect(actions.selectTab).toHaveBeenCalledWith(secondTab.id);
    expect(actions.selectTab).not.toHaveBeenCalledWith(firstTab.id);

    act(() => root.unmount());
  });

  it("wires favorite Space move actions through sidebar context menus", () => {
    const state = createDefaultState();
    const activeWorkspace = state.workspaces[0];
    const item = createFavorite("Docs", "https://docs.example");
    const actions = createActions();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(SidebarContextMenus, {
        actions,
        activeWorkspace,
        closedTabMenu: null,
        closeMenus: vi.fn(),
        quickEntryMenu: {
          item,
          kind: "favorite",
          left: 10,
          top: 20
        },
        state,
        tabGroupMenu: null,
        tabMenu: null
      }));
    });

    const moveButtons = Array.from(container.querySelectorAll(".quick-entry-context-menu button"))
      .filter((button) => button.textContent?.startsWith("Move to"));
    moveButtons[0]?.dispatchEvent(new MouseEvent("click", { bubbles: true }));
    moveButtons.at(-1)?.dispatchEvent(new MouseEvent("click", { bubbles: true }));

    expect(actions.moveWorkspaceFavoriteToWorkspace).toHaveBeenCalledWith(item.id, "work");
    expect(actions.moveWorkspaceFavoriteToNewWorkspace).toHaveBeenCalledWith(item.id);

    act(() => root.unmount());
  });

  it("styles quick entry menus with the shared sidebar menu surface", () => {
    expect(contextMenuCss).toContain(".quick-entry-context-menu");
    expect(contextMenuCss).toContain(".tab-context-menu button.danger");
  });
});

function createActions() {
  return {
    copyText: vi.fn(),
    moveWorkspaceFavoriteToNewWorkspace: vi.fn(),
    moveWorkspaceFavoriteToWorkspace: vi.fn(),
    navigateActiveTab: vi.fn(),
    openGlance: vi.fn(),
    openUrlInActiveWorkspace: vi.fn(),
    openUrlInSplit: vi.fn(),
    removeEssential: vi.fn(),
    removeWorkspaceFavorite: vi.fn(),
    selectTab: vi.fn()
  } as unknown as BrowserController["actions"];
}
