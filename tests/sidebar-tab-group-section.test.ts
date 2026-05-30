import { readFileSync } from "node:fs";
import { join } from "node:path";
import { createElement } from "react";
import { act } from "react";
import { createRoot } from "react-dom/client";
import { describe, expect, it, vi } from "vitest";

import { createTab, type TabGroup } from "../src/renderer/domain/browser";
import { TabGroupSection } from "../src/renderer/surfaces/sidebar/components/tabs/TabGroupSection";

const sidebarGroupsCss = readFileSync(join(__dirname, "../src/renderer/styles/sidebar-groups.css"), "utf8");

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
    expect(toggle.hasAttribute("title")).toBe(false);

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
    expect(toggle.hasAttribute("title")).toBe(false);

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

    container.querySelector(".tab-group-toggle")?.dispatchEvent(new KeyboardEvent("keydown", {
      bubbles: true,
      key: "F10",
      shiftKey: true
    }));

    expect(onGroupContextMenu).toHaveBeenCalledWith(expect.objectContaining({ type: "contextmenu" }), group);

    act(() => root.unmount());
  });

  it("keeps tab group titles as display text and routes editing through the context menu", () => {
    const group = tabGroup();
    const activeTab = { ...createTab("Docs", "https://docs.example"), groupId: group.id };
    const onGroupContextMenu = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(TabGroupSection, props({
        activeTab,
        group,
        onGroupContextMenu,
        tabs: [activeTab]
      })));
    });

    expect(container.querySelector(".tab-group-title")?.textContent).toBe(group.name);
    expect(container.querySelector(".tab-group-title-input")).toBeNull();
    expect(container.querySelector(".tab-group-title")?.getAttribute("tabindex")).toBeNull();
    expect(onGroupContextMenu).not.toHaveBeenCalled();

    act(() => root.unmount());
  });

  it("keeps tab group headers compact and moves color editing to the context menu", () => {
    const group = tabGroup();
    const activeTab = { ...createTab("Docs", "https://docs.example"), groupId: group.id };
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(TabGroupSection, props({
        activeTab,
        group,
        tabs: [activeTab]
      })));
    });

    const header = container.querySelector<HTMLElement>(".tab-group-header")!;

    expect(header.getAttribute("data-collapsed")).toBe("false");
    expect(container.querySelector(".tab-group-dot")).not.toBeNull();
    expect(container.querySelector(".tab-group-count")?.textContent).toBe("1");
    expect(container.querySelector(".tab-group-color")).toBeNull();
    expect(container.querySelector('input[type="color"]')).toBeNull();

    act(() => root.unmount());
  });

  it("keeps expanded tab group counts quiet until the header is engaged", () => {
    const headerBlock = getRuleBlock(sidebarGroupsCss, ".tab-group-header");
    const titleBlock = getRuleBlock(sidebarGroupsCss, ".tab-group-title");
    const countBlock = getRuleBlock(sidebarGroupsCss, ".tab-group-count");
    const revealBlock = getRuleBlock(sidebarGroupsCss, ".tab-group-header:hover .tab-group-count,\n.tab-group-header:focus-within .tab-group-count,\n.tab-group-header[data-collapsed=\"true\"] .tab-group-count");

    expect(headerBlock).toContain("grid-template-columns: 22px minmax(0, 1fr) auto");
    expect(headerBlock).not.toContain("24px");
    expect(titleBlock).toContain("text-overflow: ellipsis");
    expect(titleBlock).not.toContain("outline");
    expect(titleBlock).not.toContain("border");
    expect(countBlock).toContain("opacity: 0");
    expect(countBlock).toContain("transform: translateX(2px)");
    expect(revealBlock).toContain("opacity: 1");
    expect(revealBlock).toContain("transform: translateX(0)");
    expect(sidebarGroupsCss).not.toContain(".tab-group-color");
    expect(sidebarGroupsCss).not.toContain(".tab-group-title-input");
  });

  it("moves dropped tabs into the group folder through the shared tab folder action", () => {
    const group = tabGroup();
    const activeTab = { ...createTab("Docs", "https://docs.example"), groupId: group.id };
    const onMoveTabToGroupFolder = vi.fn();
    const setDraggingTabId = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(TabGroupSection, props({
        activeTab,
        group,
        onMoveTabToGroupFolder,
        setDraggingTabId,
        tabs: [activeTab]
      })));
    });

    const header = container.querySelector<HTMLElement>(".tab-group-header")!;
    header.dispatchEvent(createDragEvent("drop", {
      "application/x-astra-sidebar-tab-id": "favorite-tab",
      "text/favorite-id": "favorite"
    }));

    expect(onMoveTabToGroupFolder).toHaveBeenCalledWith("favorite-tab", group.id);
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
    onClose: vi.fn(),
    onContextMenu: vi.fn(),
    onDrop: vi.fn(),
    onGroupContextMenu: vi.fn(),
    onGroupDrop: vi.fn(),
    onMoveTabToGroupFolder: vi.fn(),
    onPreview: vi.fn(),
    onSelect: vi.fn(),
    onSplit: vi.fn(),
    onToggle: vi.fn(),
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

function getRuleBlock(css: string, selector: string): string {
  const start = css.indexOf(selector);
  expect(start).toBeGreaterThanOrEqual(0);
  const bodyStart = css.indexOf("{", start);
  const bodyEnd = css.indexOf("}", bodyStart);
  return css.slice(bodyStart + 1, bodyEnd);
}
