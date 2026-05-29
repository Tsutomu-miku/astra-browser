import { createElement } from "react";
import { act } from "react";
import { createRoot } from "react-dom/client";
import { describe, expect, it, vi } from "vitest";

import { createDefaultState, createFavorite, createTab } from "../src/renderer/domain/browser";
import type { BrowserController } from "../src/renderer/app/controller/types";
import { FavoriteButton, TabRow } from "../src/renderer/surfaces/sidebar/components/tabs/SidebarItems";
import { SidebarPinnedTabs } from "../src/renderer/surfaces/sidebar/components/tabs/SidebarPinnedTabs";
import { WorkspaceStrip } from "../src/renderer/surfaces/sidebar/components/workspaces/WorkspaceStrip";

describe("sidebar drag placement", () => {
  it("marks tab row before and after insertion placement while dragging", () => {
    const targetTab = createTab("Docs", "https://docs.example");
    const onDrop = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(TabRow, {
        activeTabId: targetTab.id,
        draggingTabId: "dragged-tab",
        splitTabIds: [],
        tab: targetTab,
        onClose: vi.fn(),
        onContextMenu: vi.fn(),
        onDrop,
        onPreview: vi.fn(),
        onSelect: vi.fn(),
        onSplit: vi.fn(),
        setDraggingTabId: vi.fn()
      }));
    });

    const row = container.querySelector<HTMLElement>(".tab-row")!;
    stubRect(row, { top: 0, height: 40 });

    row.dispatchEvent(createDragEvent("dragover", { clientY: 35 }));
    expect(row.dataset.dropPlacement).toBe("after");

    row.dispatchEvent(createDragEvent("dragover", { clientY: 5 }));
    expect(row.dataset.dropPlacement).toBe("before");

    row.dispatchEvent(createDragEvent("drop", { clientY: 5 }));
    expect(onDrop).toHaveBeenCalledWith(expect.objectContaining({ type: "drop" }), targetTab.id);
    expect(row.dataset.dropPlacement).toBeUndefined();

    act(() => root.unmount());
  });

  it("uses horizontal insertion placement for Essentials", () => {
    const favorite = createFavorite("Docs", "https://docs.example");
    const onDrop = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(FavoriteButton, {
        draggable: true,
        draggingQuickEntryId: "other-favorite",
        dropAxis: "horizontal",
        favorite,
        onDragStart: vi.fn(),
        onDrop,
        onOpen: vi.fn(),
        onOpenInSplit: vi.fn(),
        onPreview: vi.fn()
      }));
    });

    const button = container.querySelector<HTMLElement>(".favorite-button")!;
    stubRect(button, { left: 0, width: 80 });

    button.dispatchEvent(createDragEvent("dragover", { clientX: 60 }));
    expect(button.dataset.dropPlacement).toBe("after");

    button.dispatchEvent(createDragEvent("drop", { clientX: 60 }));
    expect(onDrop).toHaveBeenCalledWith(expect.objectContaining({ type: "drop" }), favorite.id, "horizontal");
    expect(button.dataset.dropPlacement).toBeUndefined();

    act(() => root.unmount());
  });

  it("passes horizontal placement intent when pinned tabs are dropped", () => {
    const pinned = { ...createTab("Mail", "https://mail.example"), isPinned: true };
    const onTabDrop = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(SidebarPinnedTabs, {
        actions: createActions(),
        activeTab: pinned,
        draggingTabId: "other-tab",
        pinnedTabs: [pinned],
        splitTabIds: [],
        onPinDrop: vi.fn(),
        onTabContextMenu: vi.fn(),
        onTabDrop,
        setDraggingTabId: vi.fn()
      }));
    });

    const button = container.querySelector<HTMLElement>(".pinned-tab-button")!;
    stubRect(button, { left: 0, width: 40 });

    button.dispatchEvent(createDragEvent("dragover", { clientX: 32 }));
    expect(button.dataset.dropPlacement).toBe("after");

    button.dispatchEvent(createDragEvent("drop", { clientX: 32 }));
    expect(onTabDrop).toHaveBeenCalledWith(expect.objectContaining({ type: "drop" }), pinned.id, "horizontal");
    expect(button.dataset.dropPlacement).toBeUndefined();

    act(() => root.unmount());
  });

  it("marks before and after insertion placement while reordering Spaces", () => {
    const state = createDefaultState();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(WorkspaceStrip, {
        activeWorkspaceId: state.workspaces[0].id,
        compactMode: false,
        draggingClosedTabIndex: null,
        draggingFavoriteId: null,
        draggingGroupId: null,
        draggingTabId: null,
        draggingWorkspaceId: state.workspaces[0].id,
        floatingSidebarOpen: false,
        sidebarCollapsed: false,
        workspaces: state.workspaces,
        onDragEnd: vi.fn(),
        onDragOver: vi.fn((event) => event.preventDefault()),
        onDragStart: vi.fn(),
        onDrop: vi.fn(),
        onDeleteWorkspace: vi.fn(),
        onNewWorkspace: vi.fn(),
        onNewWorkspaceDrop: vi.fn(),
        onOpenSettings: vi.fn(),
        onSelect: vi.fn(),
        onToggleSidebar: vi.fn(),
        onUpdateWorkspace: vi.fn()
      }));
    });

    const target = container.querySelectorAll<HTMLElement>(".workspace-button")[1]!;
    stubRect(target, { top: 0, height: 36 });

    target.dispatchEvent(createDragEvent("dragover", { clientY: 30 }));
    expect(target.dataset.dropPlacement).toBe("after");

    target.dispatchEvent(createDragEvent("dragover", { clientY: 4 }));
    expect(target.dataset.dropPlacement).toBe("before");

    target.dispatchEvent(createDragEvent("dragleave", { clientY: 4 }));
    expect(target.dataset.dropPlacement).toBeUndefined();

    act(() => root.unmount());
  });
});

function createActions() {
  return {
    closeTab: vi.fn(),
    openGlance: vi.fn(),
    openTabInSplit: vi.fn(),
    selectTab: vi.fn()
  } as unknown as BrowserController["actions"];
}

function createDragEvent(type: string, pointer: Partial<Pick<DragEvent, "clientX" | "clientY">>) {
  const event = new Event(type, { bubbles: true, cancelable: true });
  Object.defineProperty(event, "clientX", { value: pointer.clientX ?? 0 });
  Object.defineProperty(event, "clientY", { value: pointer.clientY ?? 0 });
  Object.defineProperty(event, "dataTransfer", {
    value: {
      dropEffect: "none",
      effectAllowed: "all",
      getData: vi.fn(() => ""),
      setData: vi.fn()
    }
  });
  return event;
}

function stubRect(target: HTMLElement, rect: Partial<DOMRect>) {
  Object.defineProperty(target, "getBoundingClientRect", {
    value: () => ({
      bottom: (rect.top ?? 0) + (rect.height ?? 0),
      height: rect.height ?? 0,
      left: rect.left ?? 0,
      right: (rect.left ?? 0) + (rect.width ?? 0),
      top: rect.top ?? 0,
      width: rect.width ?? 0,
      x: rect.left ?? 0,
      y: rect.top ?? 0,
      toJSON: () => undefined
    })
  });
}
