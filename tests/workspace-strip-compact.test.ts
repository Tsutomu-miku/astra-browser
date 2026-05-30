import { createElement } from "react";
import { act } from "react";
import { createRoot } from "react-dom/client";
import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, it, vi } from "vitest";
import { readFileSync } from "fs";
import { join } from "path";

import { createDefaultState } from "../src/renderer/domain/browser";
import { SIDEBAR_TAB_DRAG_TYPE } from "../src/renderer/common/drag-drop/sidebarDragPayload";
import { getActiveWorkspace } from "../src/renderer/domain/browser/selectors";
import { WorkspaceStrip } from "../src/renderer/surfaces/sidebar/components/workspaces/WorkspaceStrip";

const workspaceCss = readFileSync(join(__dirname, "../src/renderer/styles/sidebar-workspaces.css"), "utf8");
const sidebarMenuCss = readFileSync(join(__dirname, "../src/renderer/styles/sidebar-menu.css"), "utf8");

describe("workspace strip compact controls", () => {
  it("uses the always-visible sidebar toggle as a compact floating-sidebar pin", () => {
    const normal = renderStrip({ compactMode: false, floatingSidebarOpen: false, sidebarCollapsed: false });
    expect(normal).toContain('aria-label="Collapse sidebar"');
    expect(normal).not.toContain('title="Collapse sidebar"');

    const compact = renderStrip({ compactMode: true, floatingSidebarOpen: false, sidebarCollapsed: true });
    expect(compact).toContain('aria-label="Pin floating sidebar"');
    expect(compact).toContain('aria-pressed="false"');
    expect(compact).not.toContain('title="Pin floating sidebar"');

    const pinned = renderStrip({ compactMode: true, floatingSidebarOpen: true, sidebarCollapsed: true });
    expect(pinned).toContain('aria-label="Unpin floating sidebar"');
    expect(pinned).toContain('aria-pressed="true"');
    expect(pinned).not.toContain('title="Unpin floating sidebar"');
  });

  it("avoids native title tooltips on the Space rail controls", () => {
    const html = renderStrip({ compactMode: false, floatingSidebarOpen: false, sidebarCollapsed: false });

    expect(html).toContain('aria-label="Personal, 1 tab, active Space"');
    expect(html).toContain('aria-label="New Space"');
    expect(html).toContain('aria-label="Collapse sidebar"');
    expect(html).not.toContain('title="Personal, 1 tab"');
    expect(html).not.toContain('title="New Space"');
    expect(html).not.toContain('title="Collapse sidebar"');
  });

  it("avoids native title tooltips on Space color swatches", () => {
    const state = createDefaultState();
    const activeWorkspace = getActiveWorkspace(state);
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(WorkspaceStrip, {
        activeWorkspaceId: activeWorkspace.id,
        compactMode: false,
        draggingGroupId: null,
        draggingTabId: null,
        draggingWorkspaceId: null,
        floatingSidebarOpen: false,
        onDragEnd: vi.fn(),
        onDragOver: vi.fn(),
        onDragStart: vi.fn(),
        onDrop: vi.fn(),
        onDeleteWorkspace: vi.fn(),
        onNewWorkspace: vi.fn(),
        onNewWorkspaceDrop: vi.fn(),
        onOpenSettings: vi.fn(),
        onSelect: vi.fn(),
        onToggleSidebar: vi.fn(),
        onUpdateWorkspace: vi.fn(),
        sidebarCollapsed: false,
        workspaces: state.workspaces
      }));
    });

    act(() => {
      container.querySelector(".workspace-button")?.dispatchEvent(new MouseEvent("contextmenu", {
        bubbles: true,
        button: 2,
        clientX: 16,
        clientY: 24
      }));
    });

    const swatch = container.querySelector<HTMLButtonElement>(".workspace-menu-swatch");
    expect(swatch?.getAttribute("aria-label")).toBe("Use #7dd3fc");
    expect(swatch?.hasAttribute("title")).toBe(false);

    act(() => root.unmount());
  });

  it("marks other Spaces as drop targets while dragging a tab group", () => {
    const html = renderStrip({
      compactMode: false,
      draggingGroupId: "group",
      floatingSidebarOpen: false,
      sidebarCollapsed: false
    });

    expect(html).toContain('data-drop-target="true"');
  });

  it("marks Spaces as restore targets while dragging a recently closed tab", () => {
    const html = renderStrip({
      compactMode: false,
      draggingClosedTabIndex: 0,
      floatingSidebarOpen: false,
      sidebarCollapsed: false
    });

    expect(html.match(/data-drop-target="true"/g)).toHaveLength(3);
    expect(html).toContain('aria-label="Personal, 1 tab, active Space, drop target"');
    expect(html).toContain('aria-label="Drop to create New Space" data-drop-target="true"');
  });

  it("marks other Spaces as drop targets while dragging a Space favorite", () => {
    const html = renderStrip({
      compactMode: false,
      draggingFavoriteId: "favorite",
      floatingSidebarOpen: false,
      sidebarCollapsed: false
    });

    expect(html.match(/data-drop-target="true"/g)).toHaveLength(2);
    expect(html).toContain('aria-label="Work, 1 tab, Space, drop target"');
    expect(html).toContain('aria-label="Drop to create New Space" data-drop-target="true"');
  });

  it("marks Space rail drop targets while dragging tabs or groups", () => {
    const tabHtml = renderStrip({
      compactMode: false,
      draggingTabId: "tab",
      floatingSidebarOpen: false,
      sidebarCollapsed: false
    });
    const groupHtml = renderStrip({
      compactMode: false,
      draggingGroupId: "group",
      floatingSidebarOpen: false,
      sidebarCollapsed: false
    });

    expect(tabHtml).toContain('class="workspace-button workspace-new-button"');
    expect(tabHtml).toContain('aria-label="Work, 1 tab, Space, drop target"');
    expect(tabHtml).toContain('aria-label="Drop to create New Space" data-drop-target="true"');
    expect(groupHtml).toContain('aria-label="Drop to create New Space" data-drop-target="true"');
  });

  it("accepts payload-backed tab drags on New Space before React drag state syncs", () => {
    const state = createDefaultState();
    const activeWorkspace = getActiveWorkspace(state);
    const onNewWorkspaceDrop = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(WorkspaceStrip, {
        activeWorkspaceId: activeWorkspace.id,
        compactMode: false,
        draggingGroupId: null,
        draggingTabId: null,
        draggingWorkspaceId: null,
        floatingSidebarOpen: false,
        onDragEnd: vi.fn(),
        onDragOver: vi.fn(),
        onDragStart: vi.fn(),
        onDrop: vi.fn(),
        onDeleteWorkspace: vi.fn(),
        onNewWorkspace: vi.fn(),
        onNewWorkspaceDrop,
        onOpenSettings: vi.fn(),
        onSelect: vi.fn(),
        onToggleSidebar: vi.fn(),
        onUpdateWorkspace: vi.fn(),
        sidebarCollapsed: false,
        workspaces: state.workspaces
      }));
    });

    const newSpace = container.querySelector<HTMLButtonElement>(".workspace-new-button")!;
    const dragOver = createDragEvent("dragover", { [SIDEBAR_TAB_DRAG_TYPE]: "tab" });
    newSpace.dispatchEvent(dragOver);
    newSpace.dispatchEvent(createDragEvent("drop", { [SIDEBAR_TAB_DRAG_TYPE]: "tab" }));

    expect(dragOver.defaultPrevented).toBe(true);
    expect(onNewWorkspaceDrop).toHaveBeenCalledWith(expect.objectContaining({ type: "drop" }));

    act(() => root.unmount());
  });

  it("accepts payload-backed Favorite drags on New Space before React drag state syncs", () => {
    const state = createDefaultState();
    const activeWorkspace = getActiveWorkspace(state);
    const onNewWorkspaceDrop = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(WorkspaceStrip, {
        activeWorkspaceId: activeWorkspace.id,
        compactMode: false,
        draggingFavoriteId: null,
        draggingGroupId: null,
        draggingTabId: null,
        draggingWorkspaceId: null,
        floatingSidebarOpen: false,
        onDragEnd: vi.fn(),
        onDragOver: vi.fn(),
        onDragStart: vi.fn(),
        onDrop: vi.fn(),
        onDeleteWorkspace: vi.fn(),
        onNewWorkspace: vi.fn(),
        onNewWorkspaceDrop,
        onOpenSettings: vi.fn(),
        onSelect: vi.fn(),
        onToggleSidebar: vi.fn(),
        onUpdateWorkspace: vi.fn(),
        sidebarCollapsed: false,
        workspaces: state.workspaces
      }));
    });

    const newSpace = container.querySelector<HTMLButtonElement>(".workspace-new-button")!;
    const dragOver = createDragEvent("dragover", { "text/favorite-id": "favorite" });
    newSpace.dispatchEvent(dragOver);
    newSpace.dispatchEvent(createDragEvent("drop", { "text/favorite-id": "favorite" }));

    expect(dragOver.defaultPrevented).toBe(true);
    expect(onNewWorkspaceDrop).toHaveBeenCalledWith(expect.objectContaining({ type: "drop" }));

    act(() => root.unmount());
  });

  it("marks New Space as a drop target while dragging a recently closed tab", () => {
    const html = renderStrip({
      compactMode: false,
      draggingClosedTabIndex: 0,
      floatingSidebarOpen: false,
      sidebarCollapsed: false
    });

    expect(html).toContain('aria-label="Drop to create New Space" data-drop-target="true"');
  });

  it("opens Space context menus from the keyboard", () => {
    const state = createDefaultState();
    const activeWorkspace = getActiveWorkspace(state);
    const onSelect = vi.fn();
    const container = document.createElement("div");
    document.body.append(container);
    const root = createRoot(container);

    act(() => {
      root.render(createElement(WorkspaceStrip, {
        activeWorkspaceId: activeWorkspace.id,
        compactMode: false,
        draggingGroupId: null,
        draggingTabId: null,
        draggingWorkspaceId: null,
        floatingSidebarOpen: false,
        onDragEnd: vi.fn(),
        onDragOver: vi.fn(),
        onDragStart: vi.fn(),
        onDrop: vi.fn(),
        onDeleteWorkspace: vi.fn(),
        onNewWorkspace: vi.fn(),
        onNewWorkspaceDrop: vi.fn(),
        onOpenSettings: vi.fn(),
        onSelect,
        onToggleSidebar: vi.fn(),
        onUpdateWorkspace: vi.fn(),
        sidebarCollapsed: false,
        workspaces: state.workspaces
      }));
    });

    act(() => {
      container.querySelector(".workspace-button")?.dispatchEvent(new KeyboardEvent("keydown", {
        bubbles: true,
        key: "ContextMenu"
      }));
    });

    expect(container.querySelector(".workspace-context-menu")).not.toBeNull();
    expect(container.textContent).toContain("Space settings");
    expect(container.textContent).toContain("Switch to Space");
    expect(container.textContent).toContain("New Space");
    expect(container.innerHTML).toContain('class="sidebar-menu-item-icon" aria-hidden="true"');
    expect(container.innerHTML).not.toContain('title="Space settings"');
    expect(document.activeElement?.textContent).toBe("Space settings");

    act(() => {
      document.activeElement?.dispatchEvent(new KeyboardEvent("keydown", {
        bubbles: true,
        key: "ArrowDown"
      }));
    });
    expect(document.activeElement?.textContent).toBe("Switch to Space");
    expect(onSelect).not.toHaveBeenCalled();

    act(() => root.unmount());
    container.remove();
  });

  it("restores focus to the Space button when its context menu closes with Escape", () => {
    const state = createDefaultState();
    const activeWorkspace = getActiveWorkspace(state);
    const container = document.createElement("div");
    document.body.append(container);
    const root = createRoot(container);

    act(() => {
      root.render(createElement(WorkspaceStrip, {
        activeWorkspaceId: activeWorkspace.id,
        compactMode: false,
        draggingGroupId: null,
        draggingTabId: null,
        draggingWorkspaceId: null,
        floatingSidebarOpen: false,
        onDragEnd: vi.fn(),
        onDragOver: vi.fn(),
        onDragStart: vi.fn(),
        onDrop: vi.fn(),
        onDeleteWorkspace: vi.fn(),
        onNewWorkspace: vi.fn(),
        onNewWorkspaceDrop: vi.fn(),
        onOpenSettings: vi.fn(),
        onSelect: vi.fn(),
        onToggleSidebar: vi.fn(),
        onUpdateWorkspace: vi.fn(),
        sidebarCollapsed: false,
        workspaces: state.workspaces
      }));
    });

    const trigger = container.querySelector<HTMLButtonElement>(".workspace-button")!;
    trigger.focus();

    act(() => {
      trigger.dispatchEvent(new KeyboardEvent("keydown", {
        bubbles: true,
        key: "ContextMenu"
      }));
    });

    expect(container.querySelector(".workspace-context-menu")).not.toBeNull();

    act(() => {
      window.dispatchEvent(new KeyboardEvent("keydown", { key: "Escape" }));
    });

    expect(container.querySelector(".workspace-context-menu")).toBeNull();
    expect(document.activeElement).toBe(trigger);

    act(() => root.unmount());
    container.remove();
  });

  it("restores focus to the Space button after running a Space menu action", () => {
    const state = createDefaultState();
    const activeWorkspace = getActiveWorkspace(state);
    const onOpenSettings = vi.fn();
    const container = document.createElement("div");
    document.body.append(container);
    const root = createRoot(container);

    act(() => {
      root.render(createElement(WorkspaceStrip, {
        activeWorkspaceId: activeWorkspace.id,
        compactMode: false,
        draggingGroupId: null,
        draggingTabId: null,
        draggingWorkspaceId: null,
        floatingSidebarOpen: false,
        onDragEnd: vi.fn(),
        onDragOver: vi.fn(),
        onDragStart: vi.fn(),
        onDrop: vi.fn(),
        onDeleteWorkspace: vi.fn(),
        onNewWorkspace: vi.fn(),
        onNewWorkspaceDrop: vi.fn(),
        onOpenSettings,
        onSelect: vi.fn(),
        onToggleSidebar: vi.fn(),
        onUpdateWorkspace: vi.fn(),
        sidebarCollapsed: false,
        workspaces: state.workspaces
      }));
    });

    const trigger = container.querySelector<HTMLButtonElement>(".workspace-button")!;
    trigger.focus();

    act(() => {
      trigger.dispatchEvent(new MouseEvent("contextmenu", {
        bubbles: true,
        clientX: 10,
        clientY: 10
      }));
    });

    act(() => {
      Array.from(container.querySelectorAll(".workspace-context-menu button"))
        .find((button) => button.textContent === "Space settings")
        ?.dispatchEvent(new MouseEvent("click", { bubbles: true }));
    });

    expect(onOpenSettings).toHaveBeenCalledWith(activeWorkspace.id);
    expect(container.querySelector(".workspace-context-menu")).toBeNull();
    expect(document.activeElement).toBe(trigger);

    act(() => root.unmount());
    container.remove();
  });

  it("moves focus through Space controls with Arrow, Home, and End", () => {
    const state = createDefaultState();
    const activeWorkspace = getActiveWorkspace(state);
    const container = document.createElement("div");
    document.body.append(container);
    const root = createRoot(container);

    act(() => {
      root.render(createElement(WorkspaceStrip, {
        activeWorkspaceId: activeWorkspace.id,
        compactMode: false,
        draggingGroupId: null,
        draggingTabId: null,
        draggingWorkspaceId: null,
        floatingSidebarOpen: false,
        onDragEnd: vi.fn(),
        onDragOver: vi.fn(),
        onDragStart: vi.fn(),
        onDrop: vi.fn(),
        onDeleteWorkspace: vi.fn(),
        onNewWorkspace: vi.fn(),
        onNewWorkspaceDrop: vi.fn(),
        onOpenSettings: vi.fn(),
        onSelect: vi.fn(),
        onToggleSidebar: vi.fn(),
        onUpdateWorkspace: vi.fn(),
        sidebarCollapsed: false,
        workspaces: state.workspaces
      }));
    });

    const buttons = container.querySelectorAll<HTMLButtonElement>(".workspace-button");
    buttons[0]?.focus();

    act(() => {
      buttons[0]?.dispatchEvent(new KeyboardEvent("keydown", {
        bubbles: true,
        key: "ArrowDown"
      }));
    });
    expect(document.activeElement).toBe(buttons[1]);

    act(() => {
      document.activeElement?.dispatchEvent(new KeyboardEvent("keydown", {
        bubbles: true,
        key: "End"
      }));
    });
    expect(document.activeElement).toBe(buttons[buttons.length - 1]);
    expect(document.activeElement?.getAttribute("aria-label")).toBe("Collapse sidebar");

    act(() => {
      document.activeElement?.dispatchEvent(new KeyboardEvent("keydown", {
        bubbles: true,
        key: "Home"
      }));
    });
    expect(document.activeElement).toBe(buttons[0]);

    act(() => root.unmount());
    container.remove();
  });

  it("keeps only the active Space in the normal Tab order while Arrow navigation reaches the rest", () => {
    const state = createDefaultState();
    const activeWorkspace = getActiveWorkspace(state);
    const container = document.createElement("div");
    document.body.append(container);
    const root = createRoot(container);

    act(() => {
      root.render(createElement(WorkspaceStrip, {
        activeWorkspaceId: activeWorkspace.id,
        compactMode: false,
        draggingGroupId: null,
        draggingTabId: null,
        draggingWorkspaceId: null,
        floatingSidebarOpen: false,
        onDragEnd: vi.fn(),
        onDragOver: vi.fn(),
        onDragStart: vi.fn(),
        onDrop: vi.fn(),
        onDeleteWorkspace: vi.fn(),
        onNewWorkspace: vi.fn(),
        onNewWorkspaceDrop: vi.fn(),
        onOpenSettings: vi.fn(),
        onSelect: vi.fn(),
        onToggleSidebar: vi.fn(),
        onUpdateWorkspace: vi.fn(),
        sidebarCollapsed: false,
        workspaces: state.workspaces
      }));
    });

    const workspaceButtons = Array.from(container.querySelectorAll<HTMLButtonElement>(".workspace-button:not(.workspace-new-button):not(.sidebar-toggle)"));
    expect(workspaceButtons.map((button) => button.tabIndex)).toEqual([0, -1]);

    workspaceButtons[0]?.focus();
    act(() => {
      workspaceButtons[0]?.dispatchEvent(new KeyboardEvent("keydown", {
        bubbles: true,
        key: "ArrowDown"
      }));
    });

    expect(document.activeElement).toBe(workspaceButtons[1]);

    act(() => root.unmount());
    container.remove();
  });

  it("keeps Space menu name editing keys inside the input", () => {
    const state = createDefaultState();
    const activeWorkspace = getActiveWorkspace(state);
    const container = document.createElement("div");
    document.body.append(container);
    const root = createRoot(container);

    act(() => {
      root.render(createElement(WorkspaceStrip, {
        activeWorkspaceId: activeWorkspace.id,
        compactMode: false,
        draggingGroupId: null,
        draggingTabId: null,
        draggingWorkspaceId: null,
        floatingSidebarOpen: false,
        onDragEnd: vi.fn(),
        onDragOver: vi.fn(),
        onDragStart: vi.fn(),
        onDrop: vi.fn(),
        onDeleteWorkspace: vi.fn(),
        onNewWorkspace: vi.fn(),
        onNewWorkspaceDrop: vi.fn(),
        onOpenSettings: vi.fn(),
        onSelect: vi.fn(),
        onToggleSidebar: vi.fn(),
        onUpdateWorkspace: vi.fn(),
        sidebarCollapsed: false,
        workspaces: state.workspaces
      }));
    });

    act(() => {
      container.querySelector(".workspace-button")?.dispatchEvent(new MouseEvent("contextmenu", {
        bubbles: true,
        clientX: 10,
        clientY: 10
      }));
    });

    const input = container.querySelector<HTMLInputElement>(".workspace-menu-field input");
    input?.focus();

    act(() => {
      input?.dispatchEvent(new KeyboardEvent("keydown", {
        bubbles: true,
        key: "ArrowDown"
      }));
    });

    expect(document.activeElement).toBe(input);

    act(() => root.unmount());
    container.remove();
  });

  it("runs Space settings and creation from the Space context menu", () => {
    const state = createDefaultState();
    const activeWorkspace = getActiveWorkspace(state);
    const onNewWorkspace = vi.fn();
    const onOpenSettings = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(WorkspaceStrip, {
        activeWorkspaceId: activeWorkspace.id,
        compactMode: false,
        draggingGroupId: null,
        draggingTabId: null,
        draggingWorkspaceId: null,
        floatingSidebarOpen: false,
        onDragEnd: vi.fn(),
        onDragOver: vi.fn(),
        onDragStart: vi.fn(),
        onDrop: vi.fn(),
        onDeleteWorkspace: vi.fn(),
        onNewWorkspace,
        onNewWorkspaceDrop: vi.fn(),
        onOpenSettings,
        onSelect: vi.fn(),
        onToggleSidebar: vi.fn(),
        onUpdateWorkspace: vi.fn(),
        sidebarCollapsed: false,
        workspaces: state.workspaces
      }));
    });

    act(() => {
      container.querySelector(".workspace-button")?.dispatchEvent(new MouseEvent("contextmenu", {
        bubbles: true,
        clientX: 10,
        clientY: 10
      }));
    });
    Array.from(container.querySelectorAll(".workspace-context-menu button"))
      .find((button) => button.textContent === "Space settings")
      ?.dispatchEvent(new MouseEvent("click", { bubbles: true }));

    act(() => {
      container.querySelector(".workspace-button")?.dispatchEvent(new MouseEvent("contextmenu", {
        bubbles: true,
        clientX: 10,
        clientY: 10
      }));
    });
    Array.from(container.querySelectorAll(".workspace-context-menu button"))
      .find((button) => button.textContent === "New Space")
      ?.dispatchEvent(new MouseEvent("click", { bubbles: true }));

    expect(onOpenSettings).toHaveBeenCalledWith(activeWorkspace.id);
    expect(onNewWorkspace).toHaveBeenCalled();

    act(() => root.unmount());
  });

  it("styles keyboard focus for Space buttons", () => {
    const focusBlock = getRuleBlock(workspaceCss, ".workspace-button:focus-visible");

    expect(focusBlock).toContain("border-color: transparent");
    expect(focusBlock).toContain("background: rgba(255, 255, 255, 0.105)");
    expect(focusBlock).toContain("outline: none");
    expect(focusBlock).toContain("box-shadow: none");
    expect(focusBlock).not.toContain("var(--accent)");
  });

  it("keeps Space drop target styling quiet", () => {
    const dropTargetBlock = getRuleBlock(workspaceCss, '.workspace-button[data-drop-target="true"]');
    const draggingBlock = getRuleBlock(workspaceCss, '.workspace-button[data-dragging="true"]');
    const dropIndicatorBlock = getRuleBlock(workspaceCss, ".workspace-button[data-drop-placement]::before");

    expect(dropTargetBlock).toContain("background: rgba(255, 255, 255, 0.095)");
    expect(dropTargetBlock).toContain("box-shadow: none");
    expect(dropTargetBlock).not.toContain("var(--accent)");
    expect(draggingBlock).toContain("cursor: grabbing");
    expect(dropIndicatorBlock).toContain("box-shadow: none");
  });

  it("reveals Space tab counts only for active or engaged rail items", () => {
    const buttonBlock = getRuleBlock(workspaceCss, ".workspace-button");
    const countBlock = getRuleBlock(workspaceCss, ".workspace-tab-count");
    const revealBlock = getRuleBlock(workspaceCss, ".workspace-button:hover .workspace-tab-count,\n.workspace-button:focus-visible .workspace-tab-count,\n.workspace-button[aria-current=\"true\"] .workspace-tab-count,\n.workspace-button[data-drop-target=\"true\"] .workspace-tab-count");

    expect(buttonBlock).not.toContain("transform");
    expect(countBlock).toContain("border: 0");
    expect(countBlock).toContain("background: transparent");
    expect(countBlock).not.toContain("border-radius");
    expect(countBlock).toContain("opacity: 0");
    expect(countBlock).toContain("transition: opacity 120ms ease");
    expect(countBlock).not.toContain("transform");
    expect(revealBlock).toContain("opacity: 1");
    expect(revealBlock).not.toContain("transform");
    expect(workspaceCss).toContain(".workspace-button:hover .workspace-tab-count");
    expect(workspaceCss).toContain(".workspace-button:focus-visible .workspace-tab-count");
    expect(workspaceCss).toContain(".workspace-button[aria-current=\"true\"] .workspace-tab-count");
    expect(workspaceCss).toContain(".workspace-button[data-drop-target=\"true\"] .workspace-tab-count");
  });

  it("styles Space drag insertion indicators", () => {
    expect(workspaceCss).toContain(".workspace-button[data-drop-placement]::before");
    expect(workspaceCss).toContain('.workspace-button[data-drop-placement="before"]::before');
    expect(workspaceCss).toContain('.workspace-button[data-drop-placement="after"]::before');
  });

  it("styles Space context menu actions as quiet icon menu rows", () => {
    const itemBlock = getRuleBlock(sidebarMenuCss, ".sidebar-menu-surface .sidebar-menu-item");
    const iconBlock = getRuleBlock(sidebarMenuCss, ".sidebar-menu-item-icon");
    const labelBlock = getRuleBlock(sidebarMenuCss, ".sidebar-menu-item-label");
    const dangerBlock = getRuleBlock(sidebarMenuCss, ".sidebar-menu-surface button.danger");

    expect(itemBlock).toContain("grid-template-columns: 18px minmax(0, 1fr)");
    expect(iconBlock).toContain("color: color-mix");
    expect(labelBlock).toContain("text-overflow: ellipsis");
    expect(sidebarMenuCss).toContain(".sidebar-menu-separator");
    expect(dangerBlock).not.toContain("inline-flex");
  });
});

function renderStrip({
  compactMode,
  draggingClosedTabIndex = null,
  draggingFavoriteId = null,
  draggingGroupId = null,
  draggingTabId = null,
  floatingSidebarOpen,
  sidebarCollapsed
}: {
  compactMode: boolean;
  draggingClosedTabIndex?: number | null;
  draggingFavoriteId?: string | null;
  draggingGroupId?: string | null;
  draggingTabId?: string | null;
  floatingSidebarOpen: boolean;
  sidebarCollapsed: boolean;
}) {
  const state = createDefaultState();
  const activeWorkspace = getActiveWorkspace(state);

  return renderToStaticMarkup(createElement(WorkspaceStrip, {
    activeWorkspaceId: activeWorkspace.id,
    compactMode,
    draggingClosedTabIndex,
    draggingFavoriteId,
    draggingGroupId,
    draggingTabId,
    draggingWorkspaceId: null,
    floatingSidebarOpen,
    onDragEnd: vi.fn(),
    onDragOver: vi.fn(),
    onDragStart: vi.fn(),
    onDrop: vi.fn(),
    onDeleteWorkspace: vi.fn(),
    onNewWorkspace: vi.fn(),
    onNewWorkspaceDrop: vi.fn(),
    onOpenSettings: vi.fn(),
    onSelect: vi.fn(),
    onToggleSidebar: vi.fn(),
    onUpdateWorkspace: vi.fn(),
    sidebarCollapsed,
    workspaces: state.workspaces
  }));
}

function createDragEvent(type: string, dragData: Record<string, string> = {}) {
  const event = new Event(type, { bubbles: true, cancelable: true });
  Object.defineProperty(event, "dataTransfer", {
    value: {
      dropEffect: "none",
      effectAllowed: "all",
      getData: vi.fn((dataType: string) => dragData[dataType] ?? ""),
      setData: vi.fn((dataType: string, value: string) => {
        dragData[dataType] = value;
      })
    }
  });
  return event;
}

function getRuleBlock(css: string, selector: string): string {
  const start = css.indexOf(selector);
  expect(start).toBeGreaterThanOrEqual(0);
  const bodyStart = css.indexOf("{", start);
  const bodyEnd = css.indexOf("}", bodyStart);
  return css.slice(bodyStart + 1, bodyEnd);
}
