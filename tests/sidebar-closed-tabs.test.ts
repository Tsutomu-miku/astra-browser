import { readFileSync } from "node:fs";
import { join } from "node:path";
import { createElement } from "react";
import { act } from "react";
import { createRoot } from "react-dom/client";
import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, it, vi } from "vitest";

import type { ClosedTab } from "../src/renderer/domain/browser";
import { createDefaultState } from "../src/renderer/domain/browser";
import { ClosedTabButton } from "../src/renderer/surfaces/sidebar/components/tabs/ClosedTabButton";
import { SidebarContextMenus } from "../src/renderer/surfaces/sidebar/components/tabs/SidebarContextMenus";
import { ClosedTabContextMenu } from "../src/renderer/surfaces/sidebar/components/tabs/ClosedTabContextMenu";
import type { BrowserController } from "../src/renderer/app/controller/types";

const sidebarCss = readFileSync(join(__dirname, "../src/renderer/styles/sidebar.css"), "utf8");

describe("sidebar recently closed tabs", () => {
  it("renders a compact restore action", () => {
    const html = renderToStaticMarkup(createElement(ClosedTabButton, {
      closedIndex: 2,
      onContextMenu: vi.fn(),
      onOpenInSplit: vi.fn(),
      onPreview: vi.fn(),
      onRestore: vi.fn(),
      tab: closedTab()
    }));

    expect(html).toContain('class="closed-tab-button"');
    expect(html).toContain("Docs");
    expect(html).not.toContain('class="closed-tab-url"');
    expect(html).not.toContain(">https://docs.example/<");
    expect(html).toContain("Restore");
    expect(html).toContain('class="sidebar-item-action-hints"');
    expect(html).toContain('aria-hidden="true"');
    expect(html).toContain('data-action-hint="preview"');
    expect(html).toContain('data-action-hint="split"');
    expect(html).not.toContain("<kbd");
    expect(html).not.toContain(">Preview<");
    expect(html).not.toContain(">Split<");
    expect(html).not.toContain(">Restore<");
    expect(html).toContain('class="closed-tab-action"');
    expect(html).toContain('aria-hidden="true"');
    expect(html).not.toContain('title="Restore Docs"');
    expect(html).toContain('aria-label="Docs, recently closed tab, restore position 3"');
  });

  it("renders a recently closed context menu with restore, split, preview, and copy actions", () => {
    const html = renderToStaticMarkup(createElement(ClosedTabContextMenu, {
      closedIndex: 1,
      left: 10,
      moveWorkspaceTargets: [{ id: "work", name: "Work" }],
      onClose: vi.fn(),
      onCopyText: vi.fn(),
      onOpenInSplit: vi.fn(),
      onPreview: vi.fn(),
      onRestore: vi.fn(),
      onRestoreToNewWorkspace: vi.fn(),
      onRestoreToWorkspace: vi.fn(),
      tab: closedTab(),
      top: 20
    }));

    expect(html).toContain('role="menu"');
    expect(html).toContain("Restore");
    expect(html).toContain("Restore to Work");
    expect(html).toContain("Restore to New Space");
    expect(html).toContain("Preview in Glance");
    expect(html).toContain("Open in split view");
    expect(html).toContain("Copy URL");
    expect(html).toContain("Copy title");
    expect(html).toContain('class="sidebar-menu-item-icon" aria-hidden="true"');
    expect(html).not.toContain('title="Restore');
  });

  it("copies recently closed URL and title", () => {
    const onCopyText = vi.fn();
    const menu = createElement(ClosedTabContextMenu, {
      closedIndex: 1,
      left: 10,
      onClose: vi.fn(),
      onCopyText,
      onOpenInSplit: vi.fn(),
      onPreview: vi.fn(),
      onRestore: vi.fn(),
      tab: closedTab(),
      top: 20
    });

    menu.props.onCopyText(menu.props.tab.url);
    menu.props.onCopyText(menu.props.tab.title || menu.props.tab.url);

    expect(renderToStaticMarkup(menu)).toContain("Copy URL");
    expect(onCopyText).toHaveBeenCalledWith("https://docs.example/");
    expect(onCopyText).toHaveBeenCalledWith("Docs");
  });

  it("restores recently closed tabs to another Space from the context menu", () => {
    const tab = closedTab();
    const onClose = vi.fn();
    const onRestoreToWorkspace = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(ClosedTabContextMenu, {
        closedIndex: 2,
        left: 10,
        moveWorkspaceTargets: [{ id: "work", name: "Work" }],
        onClose,
        onCopyText: vi.fn(),
        onOpenInSplit: vi.fn(),
        onPreview: vi.fn(),
        onRestore: vi.fn(),
        onRestoreToNewWorkspace: vi.fn(),
        onRestoreToWorkspace,
        tab,
        top: 20
      }));
    });

    Array.from(container.querySelectorAll("button"))
      .find((button) => button.textContent === "Restore to Work")
      ?.dispatchEvent(new MouseEvent("click", { bubbles: true }));

    expect(onRestoreToWorkspace).toHaveBeenCalledWith(2, "work");
    expect(onClose).toHaveBeenCalled();

    act(() => root.unmount());
  });

  it("wires recently closed Space restore actions through sidebar context menus", () => {
    const state = createDefaultState();
    const activeWorkspace = state.workspaces[0];
    const tab = closedTab();
    const actions = createActions();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(SidebarContextMenus, {
        actions,
        activeWorkspace,
        closedTabMenu: {
          closedIndex: 1,
          left: 10,
          tab,
          top: 20
        },
        closeMenus: vi.fn(),
        quickEntryMenu: null,
        state,
        tabGroupMenu: null,
        tabMenu: null
      }));
    });

    const restoreButtons = Array.from(container.querySelectorAll(".closed-tab-context-menu button"))
      .filter((button) => button.textContent?.startsWith("Restore to"));
    restoreButtons[0]?.dispatchEvent(new MouseEvent("click", { bubbles: true }));
    restoreButtons.at(-1)?.dispatchEvent(new MouseEvent("click", { bubbles: true }));

    expect(actions.restoreClosedTabToWorkspace).toHaveBeenCalledWith(1, "work");
    expect(actions.restoreClosedTabToNewWorkspace).toHaveBeenCalledWith(1);

    act(() => root.unmount());
  });

  it("opens recently closed context menus from the keyboard", () => {
    const tab = closedTab();
    const onContextMenu = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(ClosedTabButton, {
        closedIndex: 2,
        onContextMenu,
        onOpenInSplit: vi.fn(),
        onPreview: vi.fn(),
        onRestore: vi.fn(),
        tab
      }));
    });

    container.querySelector(".closed-tab-button")?.dispatchEvent(new KeyboardEvent("keydown", {
      bubbles: true,
      key: "ContextMenu"
    }));

    expect(onContextMenu).toHaveBeenCalledWith(expect.objectContaining({ type: "contextmenu" }), tab, 2);

    act(() => root.unmount());
  });

  it("runs recently closed keyboard restore, preview, and split activation", () => {
    const tab = closedTab();
    const onOpenInSplit = vi.fn();
    const onPreview = vi.fn();
    const onRestore = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(ClosedTabButton, {
        closedIndex: 2,
        onContextMenu: vi.fn(),
        onOpenInSplit,
        onPreview,
        onRestore,
        tab
      }));
    });

    const button = container.querySelector(".closed-tab-button");
    button?.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "Enter" }));
    button?.dispatchEvent(new KeyboardEvent("keydown", { altKey: true, bubbles: true, key: "Enter" }));
    button?.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "Enter", shiftKey: true }));

    expect(onRestore).toHaveBeenCalledWith(2);
    expect(onPreview).toHaveBeenCalledWith(tab.url, tab.title);
    expect(onOpenInSplit).toHaveBeenCalledWith(tab.url, tab.title);

    act(() => root.unmount());
  });

  it("marks recently closed rows as draggable restore sources", () => {
    const html = renderToStaticMarkup(createElement(ClosedTabButton, {
      closedIndex: 2,
      draggingClosedTabIndex: 2,
      onContextMenu: vi.fn(),
      onDragEnd: vi.fn(),
      onDragStart: vi.fn(),
      onOpenInSplit: vi.fn(),
      onPreview: vi.fn(),
      onRestore: vi.fn(),
      tab: closedTab()
    }));

    expect(html).toContain('draggable="true"');
    expect(html).toContain('data-dragging="true"');
    expect(html).toContain('aria-label="Docs, recently closed tab, restore position 3, dragging"');
  });

  it("styles the recently closed sidebar section", () => {
    expect(sidebarCss).toContain(".recently-closed-tabs");
    expect(sidebarCss).toContain(".closed-tab-button");
    expect(sidebarCss).toContain('.closed-tab-button[data-dragging="true"]');
    expect(sidebarCss).toContain(".closed-tab-action");
    expect(sidebarCss).not.toContain(".closed-tab-url");
  });
});

function closedTab(): ClosedTab {
  return {
    closedAt: 1,
    title: "Docs",
    url: "https://docs.example/"
  };
}

function createActions() {
  return {
    copyText: vi.fn(),
    openGlance: vi.fn(),
    openUrlInSplit: vi.fn(),
    restoreClosedTab: vi.fn(),
    restoreClosedTabToNewWorkspace: vi.fn(),
    restoreClosedTabToWorkspace: vi.fn()
  } as unknown as BrowserController["actions"];
}
