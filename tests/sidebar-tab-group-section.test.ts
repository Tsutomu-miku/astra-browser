import { createElement } from "react";
import { act } from "react";
import { createRoot } from "react-dom/client";
import { describe, expect, it, vi } from "vitest";

import { createTab, type TabGroup } from "../src/renderer/domain/browser";
import { TabGroupSection } from "../src/renderer/surfaces/sidebar/components/tabs/TabGroupSection";

describe("sidebar tab group section", () => {
  it("opens tab group context menus from the keyboard", () => {
    const group = tabGroup();
    const activeTab = { ...createTab("Docs", "https://docs.example"), groupId: group.id };
    const onGroupContextMenu = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(TabGroupSection, {
        activeTab,
        draggingGroupId: null,
        draggingTabId: null,
        group,
        onAssignTab: vi.fn(),
        onClose: vi.fn(),
        onContextMenu: vi.fn(),
        onDrop: vi.fn(),
        onGroupContextMenu,
        onPreview: vi.fn(),
        onSelect: vi.fn(),
        onSplit: vi.fn(),
        onToggle: vi.fn(),
        onUpdate: vi.fn(),
        setDraggingGroupId: vi.fn(),
        setDraggingTabId: vi.fn(),
        splitTabIds: [],
        tabs: [activeTab]
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
});

function tabGroup(): TabGroup {
  return {
    color: "#7dd3fc",
    id: "group",
    isCollapsed: false,
    name: "Research"
  };
}
