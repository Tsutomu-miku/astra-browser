import { createElement } from "react";
import { act } from "react";
import { createRoot } from "react-dom/client";
import { describe, expect, it, vi } from "vitest";

import { createTab, type TabGroup } from "../src/renderer/domain/browser";
import { TabGroupSection } from "../src/renderer/surfaces/sidebar/components/tabs/TabGroupSection";

describe("sidebar tab group section", () => {
  it("collapses and expands tab groups with Left and Right arrows", () => {
    const group = tabGroup();
    const activeTab = { ...createTab("Docs", "https://docs.example"), groupId: group.id };
    const onToggle = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(TabGroupSection, props({
        activeTab,
        group,
        onToggle,
        tabs: [activeTab]
      })));
    });

    let toggle = container.querySelector<HTMLButtonElement>(".tab-group-toggle")!;
    expect(toggle.getAttribute("aria-expanded")).toBe("true");
    expect(toggle.getAttribute("aria-label")).toBe("Collapse tab group Research");

    act(() => {
      toggle.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "ArrowRight" }));
      toggle.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "ArrowLeft" }));
    });

    expect(onToggle).toHaveBeenCalledTimes(1);

    act(() => {
      root.render(createElement(TabGroupSection, props({
        activeTab,
        group: { ...group, isCollapsed: true },
        onToggle,
        tabs: [activeTab]
      })));
    });

    toggle = container.querySelector<HTMLButtonElement>(".tab-group-toggle")!;
    expect(toggle.getAttribute("aria-expanded")).toBe("false");
    expect(toggle.getAttribute("aria-label")).toBe("Expand tab group Research");

    act(() => {
      toggle.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "ArrowLeft" }));
      toggle.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "ArrowRight" }));
    });

    expect(onToggle).toHaveBeenCalledTimes(2);

    act(() => root.unmount());
  });

  it("opens tab group context menus from the keyboard", () => {
    const group = tabGroup();
    const activeTab = { ...createTab("Docs", "https://docs.example"), groupId: group.id };
    const onGroupContextMenu = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(TabGroupSection, {
        ...props({
          activeTab,
          group,
          onGroupContextMenu,
          tabs: [activeTab]
        }),
        onGroupContextMenu,
        onToggle: vi.fn()
      }));
    });

    container.querySelector(".tab-group-title-input")?.dispatchEvent(new KeyboardEvent("keydown", {
      bubbles: true,
      key: "F10",
      shiftKey: true
    }));

    expect(onGroupContextMenu).toHaveBeenCalledWith(expect.objectContaining({ type: "contextmenu" }), group);

    act(() => root.unmount());
  });

  it("moves a Favorite-backed tab out of Favorites when dropped into a group", () => {
    const group = tabGroup();
    const activeTab = { ...createTab("Docs", "https://docs.example"), groupId: group.id };
    const onAssignTab = vi.fn();
    const onMoveTabOutOfFavoritesFolder = vi.fn();
    const setDraggingTabId = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(TabGroupSection, props({
        activeTab,
        group,
        onAssignTab,
        onMoveTabOutOfFavoritesFolder,
        setDraggingTabId,
        tabs: [activeTab]
      })));
    });

    const header = container.querySelector<HTMLElement>(".tab-group-header")!;
    header.dispatchEvent(createDragEvent("drop", {
      "application/x-astra-sidebar-tab-id": "favorite-tab",
      "text/favorite-id": "favorite"
    }));

    expect(onMoveTabOutOfFavoritesFolder).toHaveBeenCalledWith(expect.objectContaining({ type: "drop" }));
    expect(onAssignTab).toHaveBeenCalledWith("favorite-tab", group.id);
    expect(setDraggingTabId).toHaveBeenCalledWith(null);

    act(() => root.unmount());
  });
});

function props(overrides: Partial<Parameters<typeof TabGroupSection>[0]> = {}): Parameters<typeof TabGroupSection>[0] {
  const group = tabGroup();
  const activeTab = { ...createTab("Docs", "https://docs.example"), groupId: group.id };

  return {
    activeTab,
    draggingGroupId: null,
    draggingTabId: null,
    group,
    onAssignTab: vi.fn(),
    onClose: vi.fn(),
    onContextMenu: vi.fn(),
    onDrop: vi.fn(),
    onGroupContextMenu: vi.fn(),
    onGroupDrop: vi.fn(),
    onPreview: vi.fn(),
    onSelect: vi.fn(),
    onSplit: vi.fn(),
    onToggle: vi.fn(),
    onUpdate: vi.fn(),
    setDraggingGroupId: vi.fn(),
    setDraggingTabId: vi.fn(),
    splitTabIds: [],
    tabs: [activeTab],
    ...overrides
  };
}

function tabGroup(): TabGroup {
  return {
    color: "#7dd3fc",
    id: "group",
    isCollapsed: false,
    name: "Research"
  };
}

function createDragEvent(type: string, data: Record<string, string>) {
  const event = new Event(type, { bubbles: true, cancelable: true }) as Event & {
    dataTransfer: DataTransfer;
  };
  Object.defineProperty(event, "dataTransfer", {
    value: {
      dropEffect: "none",
      getData: (key: string) => data[key] ?? "",
      setData: vi.fn()
    }
  });
  return event;
}
