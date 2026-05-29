import { createElement } from "react";
import { act } from "react";
import { createRoot } from "react-dom/client";
import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, it, vi } from "vitest";

import type { TabGroup } from "../src/renderer/domain/browser";
import { TAB_GROUP_COLOR_SWATCHES } from "../src/renderer/domain/tabs/groups";
import { TabGroupContextMenu } from "../src/renderer/surfaces/sidebar/components/tabs/TabGroupContextMenu";

describe("sidebar tab group context menu", () => {
  it("renders direct group management actions", () => {
    const html = renderToStaticMarkup(createElement(TabGroupContextMenu, props()));

    expect(html).toContain("Collapse group");
    expect(html).toContain("Sleep group");
    expect(html).toContain("Duplicate group");
    expect(html).toContain("Name");
    expect(html).toContain("Group color");
    expect(html).toContain("Move group to Work");
    expect(html).toContain("Close group");
    expect(html).toContain("Ungroup 2 tabs");
    expect(html).toContain(TAB_GROUP_COLOR_SWATCHES[0]);
  });

  it("switches the collapsed action label for collapsed groups", () => {
    const html = renderToStaticMarkup(createElement(TabGroupContextMenu, props({
      group: { ...group(), isCollapsed: true }
    })));

    expect(html).toContain("Expand group");
  });

  it("wires group update and ungroup callbacks", () => {
    const onClose = vi.fn();
    const onCloseGroup = vi.fn();
    const onDuplicateGroup = vi.fn();
    const onMoveToWorkspace = vi.fn();
    const onSleepGroup = vi.fn();
    const onToggleCollapsed = vi.fn();
    const onUngroupGroup = vi.fn();
    const onUpdate = vi.fn();
    const menu = createElement(TabGroupContextMenu, props({
      onClose,
      onCloseGroup,
      onDuplicateGroup,
      onMoveToWorkspace,
      onSleepGroup,
      onToggleCollapsed,
      onUngroupGroup,
      onUpdate
    }));

    menu.props.onCloseGroup("group");
    menu.props.onDuplicateGroup("group");
    menu.props.onMoveToWorkspace("group", "work");
    menu.props.onSleepGroup("group");
    menu.props.onToggleCollapsed("group");
    menu.props.onUpdate("group", { name: "Planning" });
    menu.props.onUpdate("group", { color: "#f0abfc" });
    menu.props.onUngroupGroup("group");

    expect(onCloseGroup).toHaveBeenCalledWith("group");
    expect(onDuplicateGroup).toHaveBeenCalledWith("group");
    expect(onMoveToWorkspace).toHaveBeenCalledWith("group", "work");
    expect(onSleepGroup).toHaveBeenCalledWith("group");
    expect(onToggleCollapsed).toHaveBeenCalledWith("group");
    expect(onUpdate).toHaveBeenCalledWith("group", { name: "Planning" });
    expect(onUpdate).toHaveBeenCalledWith("group", { color: "#f0abfc" });
    expect(onUngroupGroup).toHaveBeenCalledWith("group");
  });

  it("keeps group name editing keys inside the input", () => {
    const container = document.createElement("div");
    document.body.append(container);
    const root = createRoot(container);

    act(() => {
      root.render(createElement(TabGroupContextMenu, props()));
    });

    const input = container.querySelector<HTMLInputElement>(".tab-group-menu-field input");
    input?.focus();

    act(() => {
      input?.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "ArrowDown" }));
    });

    expect(document.activeElement).toBe(input);

    act(() => root.unmount());
    container.remove();
  });
});

type TabGroupContextMenuProps = Parameters<typeof TabGroupContextMenu>[0];

function group(overrides: Partial<TabGroup> = {}): TabGroup {
  return {
    color: "#7dd3fc",
    id: "group",
    isCollapsed: false,
    name: "Research",
    ...overrides
  };
}

function props(overrides: Partial<TabGroupContextMenuProps> = {}): TabGroupContextMenuProps {
  return {
    canSleepGroup: true,
    group: group(),
    left: 10,
    moveWorkspaceTargets: [{ id: "work", name: "Work" }],
    onClose: vi.fn(),
    onCloseGroup: vi.fn(),
    onDuplicateGroup: vi.fn(),
    onMoveToWorkspace: vi.fn(),
    onSleepGroup: vi.fn(),
    onToggleCollapsed: vi.fn(),
    onUngroupGroup: vi.fn(),
    onUpdate: vi.fn(),
    tabCount: 2,
    top: 20,
    ...overrides
  };
}
