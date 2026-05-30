import { createElement } from "react";
import { act } from "react";
import { createRoot } from "react-dom/client";
import { describe, expect, it, vi } from "vitest";
import { readFileSync } from "node:fs";
import { join } from "node:path";

import { createDefaultState, createFavorite, createTab, type TabGroup } from "../src/renderer/domain/browser";
import type { BrowserController } from "../src/renderer/app/controller/types";
import { SIDEBAR_TAB_DRAG_TYPE } from "../src/renderer/common/drag-drop/sidebarDragPayload";
import { FavoriteButton, TabRow } from "../src/renderer/surfaces/sidebar/components/tabs/SidebarItems";
import { SidebarPinnedTabs } from "../src/renderer/surfaces/sidebar/components/tabs/SidebarPinnedTabs";
import { SidebarTabsSection } from "../src/renderer/surfaces/sidebar/components/tabs/SidebarTabsSection";
import { TabGroupSection } from "../src/renderer/surfaces/sidebar/components/tabs/TabGroupSection";
import { WorkspaceStrip } from "../src/renderer/surfaces/sidebar/components/workspaces/WorkspaceStrip";

const sidebarGroupsCss = readFileSync(join(__dirname, "../src/renderer/styles/sidebar-groups.css"), "utf8");
const sidebarDropZonesCss = readFileSync(join(__dirname, "../src/renderer/styles/sidebar-drop-zones.css"), "utf8");

describe("sidebar drag placement", () => {
  it("keeps folder reordering feedback to local insertion lines", () => {
    const tabIndicatorBlock = getRuleBlock(sidebarDropZonesCss, ".tab-row[data-drop-placement]::before");
    const tileIndicatorBlock = getRuleBlock(sidebarDropZonesCss, ".essentials .favorite-button[data-drop-placement]::before");

    expect(sidebarDropZonesCss).not.toContain('.favorite-button[data-drop-target="true"]');
    expect(sidebarGroupsCss).not.toContain('.tab-group-header[data-drop-target="true"]');
    expect(sidebarDropZonesCss).toContain(".favorites .favorite-button[data-drop-placement]::before");
    expect(sidebarGroupsCss).toContain(".tab-group-header[data-drop-placement]::before");
    expect(tabIndicatorBlock).toContain("var(--muted)");
    expect(tabIndicatorBlock).toContain("box-shadow: none");
    expect(tabIndicatorBlock).not.toContain("var(--accent)");
    expect(tileIndicatorBlock).toContain("var(--muted)");
    expect(tileIndicatorBlock).toContain("box-shadow: none");
    expect(tileIndicatorBlock).not.toContain("var(--accent)");
  });

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

  it("accepts payload-backed tab drags when React drag state is not synced yet", () => {
    const targetTab = createTab("Docs", "https://docs.example");
    const onDrop = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(TabRow, {
        activeTabId: targetTab.id,
        draggingTabId: null,
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
    const dragOver = createDragEvent("dragover", { clientY: 35 }, { [SIDEBAR_TAB_DRAG_TYPE]: "dragged-tab" });

    row.dispatchEvent(dragOver);
    expect(dragOver.defaultPrevented).toBe(true);
    expect(row.dataset.dropPlacement).toBe("after");

    row.dispatchEvent(createDragEvent("drop", { clientY: 35 }, { [SIDEBAR_TAB_DRAG_TYPE]: "dragged-tab" }));
    expect(onDrop).toHaveBeenCalledWith(expect.objectContaining({ type: "drop" }), targetTab.id);

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

  it("accepts payload-backed Essential reorders when React drag state is not synced yet", () => {
    const target = createFavorite("Calendar", "https://calendar.example");
    const onDrop = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(FavoriteButton, {
        draggable: true,
        draggingQuickEntryId: null,
        dropAxis: "horizontal",
        favorite: target,
        kind: "essential",
        onDragStart: vi.fn(),
        onDrop,
        onOpen: vi.fn(),
        onOpenInSplit: vi.fn(),
        onPreview: vi.fn()
      }));
    });

    const button = container.querySelector<HTMLElement>(".favorite-button")!;
    stubRect(button, { left: 0, width: 80 });
    const dragOver = createDragEvent("dragover", { clientX: 60 }, { "text/essential-id": "dragged-essential" });

    button.dispatchEvent(dragOver);
    expect(dragOver.defaultPrevented).toBe(true);
    expect(button.dataset.dropPlacement).toBe("after");

    button.dispatchEvent(createDragEvent("drop", { clientX: 60 }, { "text/essential-id": "dragged-essential" }));
    expect(onDrop).toHaveBeenCalledWith(expect.objectContaining({ type: "drop" }), target.id, "horizontal");
    expect(button.dataset.dropPlacement).toBeUndefined();

    act(() => root.unmount());
  });

  it("accepts payload-backed Favorite reorders when React drag state is not synced yet", () => {
    const target = createFavorite("Second", "https://second.example");
    const onDrop = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(FavoriteButton, {
        draggable: true,
        draggingQuickEntryId: null,
        favorite: target,
        kind: "favorite",
        onDragStart: vi.fn(),
        onDrop,
        onOpen: vi.fn(),
        onOpenInSplit: vi.fn(),
        onPreview: vi.fn()
      }));
    });

    const button = container.querySelector<HTMLElement>(".favorite-button")!;
    stubRect(button, { top: 0, height: 40 });
    const dragOver = createDragEvent("dragover", { clientY: 5 }, { "text/favorite-id": "dragged-favorite" });

    button.dispatchEvent(dragOver);
    expect(dragOver.defaultPrevented).toBe(true);
    expect(button.dataset.dropPlacement).toBe("before");

    button.dispatchEvent(createDragEvent("drop", { clientY: 5 }, { "text/favorite-id": "dragged-favorite" }));
    expect(onDrop).toHaveBeenCalledWith(expect.objectContaining({ type: "drop" }), target.id, "vertical");
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

  it("accepts payload-backed drags over pinned tabs", () => {
    const pinned = { ...createTab("Mail", "https://mail.example"), isPinned: true };
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(SidebarPinnedTabs, {
        actions: createActions(),
        activeTab: pinned,
        draggingTabId: null,
        pinnedTabs: [pinned],
        splitTabIds: [],
        onPinDrop: vi.fn(),
        onTabContextMenu: vi.fn(),
        onTabDrop: vi.fn(),
        setDraggingTabId: vi.fn()
      }));
    });

    const button = container.querySelector<HTMLElement>(".pinned-tab-button")!;
    stubRect(button, { left: 0, width: 40 });
    const dragOver = createDragEvent("dragover", { clientX: 8 }, { "text/plain": "other-tab" });

    button.dispatchEvent(dragOver);
    expect(dragOver.defaultPrevented).toBe(true);
    expect(button.dataset.dropPlacement).toBe("before");

    act(() => root.unmount());
  });

  it("ignores non-tab payload drops on pinned tab buttons", () => {
    const pinned = { ...createTab("Mail", "https://mail.example"), isPinned: true };
    const onTabDrop = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(SidebarPinnedTabs, {
        actions: createActions(),
        activeTab: pinned,
        draggingTabId: null,
        pinnedTabs: [pinned],
        splitTabIds: [],
        onPinDrop: vi.fn(),
        onTabContextMenu: vi.fn(),
        onTabDrop,
        setDraggingTabId: vi.fn()
      }));
    });

    const button = container.querySelector<HTMLElement>(".pinned-tab-button")!;
    const drop = createDragEvent("drop", {}, { "text/favorite-id": "favorite" });
    button.dispatchEvent(drop);

    expect(onTabDrop).not.toHaveBeenCalled();
    expect(drop.defaultPrevented).toBe(false);

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

  it("marks before and after insertion placement while reordering tab groups", () => {
    const group = tabGroup("target", "Target");
    const tab = { ...createTab("Docs", "https://docs.example"), groupId: group.id };
    const onGroupDrop = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(TabGroupSection, {
        activeTab: tab,
        draggingGroupId: "dragged-group",
        draggingTabId: null,
        group,
        onClose: vi.fn(),
        onContextMenu: vi.fn(),
        onDrop: vi.fn(),
        onGroupContextMenu: vi.fn(),
        onGroupDrop,
        onMoveTabToGroupFolder: vi.fn(),
        onPreview: vi.fn(),
        onSelect: vi.fn(),
        onSplit: vi.fn(),
        onToggle: vi.fn(),
        searchSelectedTabId: undefined,
        setDraggingGroupId: vi.fn(),
        setDraggingTabId: vi.fn(),
        splitTabIds: [],
        tabs: [tab]
      }));
    });

    const header = container.querySelector<HTMLElement>(".tab-group-header")!;
    stubRect(header, { top: 0, height: 36 });

    header.dispatchEvent(createDragEvent("dragover", { clientY: 30 }));
    expect(header.dataset.dropPlacement).toBe("after");

    header.dispatchEvent(createDragEvent("dragover", { clientY: 4 }));
    expect(header.dataset.dropPlacement).toBe("before");

    header.dispatchEvent(createDragEvent("drop", { clientY: 4 }));
    expect(onGroupDrop).toHaveBeenCalledWith(expect.objectContaining({ type: "drop" }), group.id);
    expect(header.dataset.dropPlacement).toBeUndefined();

    act(() => root.unmount());
  });

  it("accepts payload-backed tab group reorders when React drag state is not synced yet", () => {
    const group = tabGroup("target", "Target");
    const tab = { ...createTab("Docs", "https://docs.example"), groupId: group.id };
    const onGroupDrop = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(TabGroupSection, {
        activeTab: tab,
        draggingGroupId: null,
        draggingTabId: null,
        group,
        onClose: vi.fn(),
        onContextMenu: vi.fn(),
        onDrop: vi.fn(),
        onGroupContextMenu: vi.fn(),
        onGroupDrop,
        onMoveTabToGroupFolder: vi.fn(),
        onPreview: vi.fn(),
        onSelect: vi.fn(),
        onSplit: vi.fn(),
        onToggle: vi.fn(),
        searchSelectedTabId: undefined,
        setDraggingGroupId: vi.fn(),
        setDraggingTabId: vi.fn(),
        splitTabIds: [],
        tabs: [tab]
      }));
    });

    const header = container.querySelector<HTMLElement>(".tab-group-header")!;
    stubRect(header, { top: 0, height: 36 });
    const dragOver = createDragEvent("dragover", { clientY: 4 }, { "text/group-id": "dragged-group" });

    header.dispatchEvent(dragOver);
    expect(dragOver.defaultPrevented).toBe(true);
    expect(header.dataset.dropPlacement).toBe("before");

    header.dispatchEvent(createDragEvent("drop", { clientY: 4 }, { "text/group-id": "dragged-group" }));
    expect(onGroupDrop).toHaveBeenCalledWith(expect.objectContaining({ type: "drop" }), group.id);

    act(() => root.unmount());
  });

  it("recovers payload-backed tab group reorders in the Tabs section", () => {
    const group = tabGroup("target", "Target");
    const tab = { ...createTab("Docs", "https://docs.example"), groupId: group.id };
    const actions = {
      ...createActions(),
      reorderTabGroup: vi.fn(),
      toggleTabGroupCollapsed: vi.fn(),
      updateTabGroup: vi.fn()
    } as unknown as BrowserController["actions"];
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(SidebarTabsSection, {
        actions,
        activeTab: tab,
        draggingGroupId: null,
        draggingTabId: null,
        filteredItems: {
          essentials: [],
          favorites: [],
          groupedTabs: [{ group, tabs: [tab] }],
          hasMatches: true,
          isFiltering: false,
          pinnedTabs: [],
          regularTabs: []
        },
        isCollapsed: false,
        onTabContextMenu: vi.fn(),
        onTabDrop: vi.fn(),
        onTabGroupContextMenu: vi.fn(),
        onTabsDrop: vi.fn(),
        onToggle: vi.fn(),
        setDraggingGroupId: vi.fn(),
        setDraggingTabId: vi.fn(),
        splitTabIds: [],
        tabCount: 1
      }));
    });

    const header = container.querySelector<HTMLElement>(".tab-group-header")!;
    stubRect(header, { top: 0, height: 36 });
    header.dispatchEvent(createDragEvent("drop", { clientY: 32 }, { "text/group-id": "dragged-group" }));

    expect(actions.reorderTabGroup).toHaveBeenCalledWith("dragged-group", group.id, "after");

    act(() => root.unmount());
  });

  it("assigns payload-backed tab drags to tab groups", () => {
    const group = tabGroup("target", "Target");
    const tab = { ...createTab("Docs", "https://docs.example"), groupId: group.id };
    const onMoveTabToGroupFolder = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(TabGroupSection, {
        activeTab: tab,
        draggingGroupId: null,
        draggingTabId: null,
        group,
        onClose: vi.fn(),
        onContextMenu: vi.fn(),
        onDrop: vi.fn(),
        onGroupContextMenu: vi.fn(),
        onGroupDrop: vi.fn(),
        onMoveTabToGroupFolder,
        onPreview: vi.fn(),
        onSelect: vi.fn(),
        onSplit: vi.fn(),
        onToggle: vi.fn(),
        searchSelectedTabId: undefined,
        setDraggingGroupId: vi.fn(),
        setDraggingTabId: vi.fn(),
        splitTabIds: [],
        tabs: [tab]
      }));
    });

    const header = container.querySelector<HTMLElement>(".tab-group-header")!;
    const dragOver = createDragEvent("dragover", {}, { "text/plain": "dragged-tab" });
    header.dispatchEvent(dragOver);
    expect(dragOver.defaultPrevented).toBe(true);

    header.dispatchEvent(createDragEvent("drop", {}, { "text/plain": "dragged-tab" }));
    expect(onMoveTabToGroupFolder).toHaveBeenCalledWith("dragged-tab", group.id);

    act(() => root.unmount());
  });

  it("styles tab group insertion indicators", () => {
    expect(sidebarGroupsCss).toContain(".tab-group-header[data-drop-placement]::before");
    expect(sidebarGroupsCss).toContain('.tab-group-header[data-drop-placement="before"]::before');
    expect(sidebarGroupsCss).toContain('.tab-group-header[data-drop-placement="after"]::before');
  });

  it("keeps tab group drag and hover feedback quiet", () => {
    const headerBlock = getRuleBlock(sidebarGroupsCss, ".tab-group-header");
    const draggingBlock = getRuleBlock(sidebarGroupsCss, ".tab-group-header[data-dragging=\"true\"]");
    const hoverBlock = getRuleBlock(sidebarGroupsCss, ".tab-group-header:hover");
    const dropIndicatorBlock = getRuleBlock(sidebarGroupsCss, ".tab-group-header[data-drop-placement]::before");

    expect(headerBlock).toContain("border: 1px solid transparent");
    expect(headerBlock).toContain("background: transparent");
    expect(headerBlock).toContain("cursor: pointer");
    expect(headerBlock).not.toContain("var(--group-color)");
    expect(draggingBlock).toContain("cursor: grabbing");
    expect(draggingBlock).not.toContain("transform");
    expect(hoverBlock).toContain("border-color: transparent");
    expect(hoverBlock).not.toContain("var(--group-color)");
    expect(dropIndicatorBlock).toContain("box-shadow: none");
    expect(dropIndicatorBlock).not.toContain("var(--group-color)");
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

function createDragEvent(
  type: string,
  pointer: Partial<Pick<DragEvent, "clientX" | "clientY">>,
  dragData: Record<string, string> = {}
) {
  const event = new Event(type, { bubbles: true, cancelable: true });
  Object.defineProperty(event, "clientX", { value: pointer.clientX ?? 0 });
  Object.defineProperty(event, "clientY", { value: pointer.clientY ?? 0 });
  Object.defineProperty(event, "dataTransfer", {
    value: {
      dropEffect: "none",
      effectAllowed: "all",
      getData: vi.fn((type: string) => dragData[type] ?? ""),
      setData: vi.fn((type: string, value: string) => {
        dragData[type] = value;
      })
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

function getRuleBlock(css: string, selector: string): string {
  const start = css.indexOf(selector);
  expect(start).toBeGreaterThanOrEqual(0);
  const bodyStart = css.indexOf("{", start);
  const bodyEnd = css.indexOf("}", bodyStart);
  return css.slice(bodyStart + 1, bodyEnd);
}

function tabGroup(id: string, name: string): TabGroup {
  return {
    color: "#7dd3fc",
    id,
    isCollapsed: false,
    name
  };
}
