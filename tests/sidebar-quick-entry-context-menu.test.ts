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

  it("opens quick entries from the sidebar context menu in the active tab", () => {
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

    container.querySelector(".quick-entry-context-menu button")?.dispatchEvent(new MouseEvent("click", { bubbles: true }));

    expect(actions.navigateActiveTab).toHaveBeenCalledWith(item.url, item.title);
    expect(actions.openUrlInActiveWorkspace).not.toHaveBeenCalled();

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
    removeWorkspaceFavorite: vi.fn()
  } as unknown as BrowserController["actions"];
}
