import { createElement } from "react";
import { act } from "react";
import { createRoot } from "react-dom/client";
import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, it, vi } from "vitest";
import { readFileSync } from "fs";
import { join } from "path";

import { createDefaultState } from "../src/renderer/domain/browser";
import { getActiveWorkspace } from "../src/renderer/domain/browser/selectors";
import { WorkspaceStrip } from "../src/renderer/surfaces/sidebar/components/workspaces/WorkspaceStrip";

const workspaceCss = readFileSync(join(__dirname, "../src/renderer/styles/sidebar-workspaces.css"), "utf8");

describe("workspace strip compact controls", () => {
  it("uses the always-visible sidebar toggle as a compact floating-sidebar pin", () => {
    const normal = renderStrip({ compactMode: false, floatingSidebarOpen: false, sidebarCollapsed: false });
    expect(normal).toContain('aria-label="Collapse sidebar"');

    const compact = renderStrip({ compactMode: true, floatingSidebarOpen: false, sidebarCollapsed: true });
    expect(compact).toContain('aria-label="Pin floating sidebar"');
    expect(compact).toContain('aria-pressed="false"');

    const pinned = renderStrip({ compactMode: true, floatingSidebarOpen: true, sidebarCollapsed: true });
    expect(pinned).toContain('aria-label="Unpin floating sidebar"');
    expect(pinned).toContain('aria-pressed="true"');
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
    expect(html).toContain('aria-label="New Space" data-drop-target="true"');
  });

  it("marks other Spaces as drop targets while dragging a Space favorite", () => {
    const html = renderStrip({
      compactMode: false,
      draggingFavoriteId: "favorite",
      floatingSidebarOpen: false,
      sidebarCollapsed: false
    });

    expect(html.match(/data-drop-target="true"/g)).toHaveLength(2);
    expect(html).toContain('aria-label="New Space" data-drop-target="true"');
  });

  it("marks New Space as a drop target while dragging tabs or groups", () => {
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
    expect(tabHtml).toContain('aria-label="New Space" data-drop-target="true"');
    expect(groupHtml).toContain('aria-label="New Space" data-drop-target="true"');
  });

  it("marks New Space as a drop target while dragging a recently closed tab", () => {
    const html = renderStrip({
      compactMode: false,
      draggingClosedTabIndex: 0,
      floatingSidebarOpen: false,
      sidebarCollapsed: false
    });

    expect(html).toContain('aria-label="New Space" data-drop-target="true"');
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
    expect(workspaceCss).toContain(".workspace-button:focus-visible");
    expect(workspaceCss).toContain("outline: none");
    expect(workspaceCss).toContain("0 0 0 3px color-mix(in srgb, var(--accent) 22%, transparent)");
  });

  it("styles Space drag insertion indicators", () => {
    expect(workspaceCss).toContain(".workspace-button[data-drop-placement]::before");
    expect(workspaceCss).toContain('.workspace-button[data-drop-placement="before"]::before');
    expect(workspaceCss).toContain('.workspace-button[data-drop-placement="after"]::before');
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
