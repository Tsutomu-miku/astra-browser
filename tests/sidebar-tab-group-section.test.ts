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

  it("renders tab group titles as display text by default, with inline rename and context menu both available", () => {
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
    expect(container.querySelector(".tab-group-folder-icon")).not.toBeNull();
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
    expect(countBlock).toContain("transition: opacity 120ms ease");
    expect(countBlock).not.toContain("transform");
    expect(revealBlock).toContain("opacity: 1");
    expect(revealBlock).not.toContain("transform");
    expect(sidebarGroupsCss).not.toContain(".tab-group-color");
    expect(sidebarGroupsCss).toContain(".tab-group-folder-icon");
    expect(sidebarGroupsCss).not.toContain(".tab-group-dot");
    expect(sidebarGroupsCss).toContain(".tab-group-title-input");
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

  it("stops tab drops on the group header from bubbling to parent React drop handlers", () => {
    const group = tabGroup();
    const activeTab = { ...createTab("Docs", "https://docs.example"), groupId: group.id };
    const onMoveTabToGroupFolder = vi.fn();
    const parentDrop = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement("div", {
        onDrop: parentDrop
      }, createElement(TabGroupSection, props({
        activeTab,
        group,
        onMoveTabToGroupFolder,
        setDraggingTabId: vi.fn(),
        tabs: [activeTab]
      }))));
    });

    const header = container.querySelector<HTMLElement>(".tab-group-header")!;
    const drop = createDragEvent("drop", {
      "application/x-astra-sidebar-tab-id": "dragged-tab",
      "text/plain": "dragged-tab"
    });
    header.dispatchEvent(drop);

    expect(onMoveTabToGroupFolder).toHaveBeenCalledWith("dragged-tab", group.id);
    expect(drop.defaultPrevented).toBe(true);
    expect(parentDrop).not.toHaveBeenCalled();

    act(() => root.unmount());
  });

  it("stops group reorder drops on the group header from bubbling to parent React drop handlers", () => {
    const group = tabGroup();
    const activeTab = { ...createTab("Docs", "https://docs.example"), groupId: group.id };
    const onGroupDrop = vi.fn();
    const parentDrop = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement("div", {
        onDrop: parentDrop
      }, createElement(TabGroupSection, props({
        activeTab,
        draggingGroupId: "other-group",
        group,
        onGroupDrop,
        setDraggingGroupId: vi.fn(),
        tabs: [activeTab]
      }))));
    });

    const header = container.querySelector<HTMLElement>(".tab-group-header")!;
    const drop = createDragEvent("drop", { "text/group-id": "other-group" });
    header.dispatchEvent(drop);

    expect(onGroupDrop).toHaveBeenCalledWith(expect.objectContaining({ type: "drop" }), group.id);
    expect(drop.defaultPrevented).toBe(true);
    expect(parentDrop).not.toHaveBeenCalled();

    act(() => root.unmount());
  });

  it("stops same-group header drops from bubbling and clears drop state", () => {
    const group = tabGroup();
    const activeTab = { ...createTab("Docs", "https://docs.example"), groupId: group.id };
    const onGroupDrop = vi.fn();
    const setDraggingGroupId = vi.fn();
    const parentDrop = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement("div", {
        onDrop: parentDrop
      }, createElement(TabGroupSection, props({
        activeTab,
        draggingGroupId: group.id,
        group,
        onGroupDrop,
        setDraggingGroupId,
        tabs: [activeTab]
      }))));
    });

    const header = container.querySelector<HTMLElement>(".tab-group-header")!;
    const drop = createDragEvent("drop", { "text/group-id": group.id });
    header.dispatchEvent(drop);

    expect(onGroupDrop).not.toHaveBeenCalled();
    expect(setDraggingGroupId).toHaveBeenCalledWith(null);
    expect(drop.defaultPrevented).toBe(true);
    expect(parentDrop).not.toHaveBeenCalled();
    expect(header.dataset.dropPlacement).toBeUndefined();

    act(() => root.unmount());
  });

  it("clears same-group drops through the shared header drop resolver", () => {
    const group = tabGroup();
    const activeTab = { ...createTab("Docs", "https://docs.example"), groupId: group.id };
    const onGroupDrop = vi.fn();
    const setDraggingGroupId = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(TabGroupSection, props({
        activeTab,
        draggingGroupId: group.id,
        group,
        onGroupDrop,
        setDraggingGroupId,
        tabs: [activeTab]
      })));
    });

    const header = container.querySelector<HTMLElement>(".tab-group-header")!;
    const drop = createDragEvent("drop", { "text/group-id": group.id });
    header.dispatchEvent(drop);

    expect(drop.defaultPrevented).toBe(true);
    expect(onGroupDrop).not.toHaveBeenCalled();
    expect(setDraggingGroupId).toHaveBeenCalledWith(null);

    act(() => root.unmount());
  });

  it("starts inline rename on title double-click, commits on Enter, cancels on Escape", () => {
    const group = tabGroup();
    const activeTab = { ...createTab("Docs", "https://docs.example"), groupId: group.id };
    const onRenameGroup = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(TabGroupSection, props({
        activeTab,
        group,
        onRenameGroup,
        tabs: [activeTab]
      })));
    });

    // By default: no input
    expect(container.querySelector(".tab-group-title-input")).toBeNull();

    // Double-click the inner title span to start renaming
    const titleInner = container.querySelector<HTMLSpanElement>(".tab-group-title > span")!;
    act(() => {
      titleInner.dispatchEvent(new MouseEvent("dblclick", { bubbles: true, cancelable: true }));
    });

    const input = container.querySelector<HTMLInputElement>(".tab-group-title-input")!;
    expect(input).not.toBeNull();
    expect(input.value).toBe(group.name);

    // Type a new name via native setter to bypass React controlled-input handling
    act(() => {
      const nativeInputValueSetter = Object.getOwnPropertyDescriptor(
        window.HTMLInputElement.prototype,
        "value"
      )!.set!;
      nativeInputValueSetter.call(input, "New Name");
      input.dispatchEvent(new Event("input", { bubbles: true }));
      const evt = new KeyboardEvent("keydown", { bubbles: true, cancelable: true, key: "Enter" });
      input.dispatchEvent(evt);
    });

    expect(onRenameGroup).toHaveBeenCalledWith(group.id, "New Name");
    expect(container.querySelector(".tab-group-title-input")).toBeNull();

    // Start again, cancel with Escape
    act(() => {
      root.render(createElement(TabGroupSection, props({
        activeTab,
        group: { ...group, name: "New Name" },
        onRenameGroup,
        tabs: [activeTab]
      })));
    });
    const titleInner2 = container.querySelector<HTMLSpanElement>(".tab-group-title > span")!;
    act(() => {
      titleInner2.dispatchEvent(new MouseEvent("dblclick", { bubbles: true, cancelable: true }));
    });
    const input2 = container.querySelector<HTMLInputElement>(".tab-group-title-input")!;
    act(() => {
      const nativeInputValueSetter = Object.getOwnPropertyDescriptor(
        window.HTMLInputElement.prototype,
        "value"
      )!.set!;
      nativeInputValueSetter.call(input2, "Cancel This");
      input2.dispatchEvent(new Event("input", { bubbles: true }));
      const evt = new KeyboardEvent("keydown", { bubbles: true, cancelable: true, key: "Escape" });
      input2.dispatchEvent(evt);
    });

    expect(onRenameGroup).toHaveBeenCalledTimes(1); // no additional call
    expect(container.querySelector(".tab-group-title-input")).toBeNull();

    act(() => root.unmount());
  });

  it("starts inline rename with F2 and submits via Enter", () => {
    const group = tabGroup();
    const activeTab = { ...createTab("Docs", "https://docs.example"), groupId: group.id };
    const onRenameGroup = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(TabGroupSection, props({
        activeTab,
        group,
        onRenameGroup,
        tabs: [activeTab]
      })));
    });

    const header = container.querySelector<HTMLElement>(".tab-group-header")!;
    act(() => {
      header.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, cancelable: true, key: "F2" }));
    });

    const input = container.querySelector<HTMLInputElement>(".tab-group-title-input")!;
    expect(input).not.toBeNull();

    // Type and commit with Enter
    act(() => {
      const nativeInputValueSetter = Object.getOwnPropertyDescriptor(
        window.HTMLInputElement.prototype,
        "value"
      )!.set!;
      nativeInputValueSetter.call(input, "F2 Renamed");
      input.dispatchEvent(new Event("input", { bubbles: true }));
      input.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, cancelable: true, key: "Enter" }));
    });

    expect(onRenameGroup).toHaveBeenCalledWith(group.id, "F2 Renamed");

    act(() => root.unmount());
  });

  it("prevents starting a drag while renaming", () => {
    const group = tabGroup();
    const activeTab = { ...createTab("Docs", "https://docs.example"), groupId: group.id };
    const onRenameGroup = vi.fn();
    const setDraggingGroupId = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(TabGroupSection, props({
        activeTab,
        group,
        onRenameGroup,
        setDraggingGroupId,
        tabs: [activeTab]
      })));
    });

    // Start rename via F2
    const header = container.querySelector<HTMLElement>(".tab-group-header")!;
    act(() => {
      header.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, cancelable: true, key: "F2" }));
    });
    expect(container.querySelector(".tab-group-title-input")).not.toBeNull();

    // Attempt to drag while renaming: should be prevented
    const dragStart = new Event("dragstart", { bubbles: true, cancelable: true });
    Object.defineProperty(dragStart, "dataTransfer", {
      value: { effectAllowed: "move", setData: vi.fn(), getData: vi.fn() }
    });
    header.dispatchEvent(dragStart);

    expect(dragStart.defaultPrevented).toBe(true);
    expect(setDraggingGroupId).not.toHaveBeenCalled();

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
