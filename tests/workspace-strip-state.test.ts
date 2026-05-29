import { describe, expect, it } from "vitest";

import { createTab, type Workspace } from "../src/renderer/domain/browser";
import {
  WORKSPACE_ACCENT_SWATCHES,
  getAdjacentWorkspaceId,
  getNewWorkspaceAccessibilityLabel,
  getWorkspaceAccessibilityLabel,
  getWorkspaceButtonLabel,
  getWorkspaceDropTargetState,
  getWorkspaceInitial,
  getWorkspaceTabCount,
  getWorkspaceWheelDirection
} from "../src/renderer/surfaces/sidebar/model/workspaceStripState";

function workspace(name: string, tabCount: number): Pick<Workspace, "name" | "tabs"> {
  return {
    name,
    tabs: Array.from({ length: tabCount }, (_, index) => createTab(`Tab ${index + 1}`, `https://${index}.example`))
  };
}

describe("workspace strip state", () => {
  it("derives compact workspace labels", () => {
    const personal = workspace("Personal", 2);

    expect(getWorkspaceInitial(personal)).toBe("P");
    expect(getWorkspaceTabCount(personal)).toBe(2);
    expect(getWorkspaceButtonLabel(personal)).toBe("Personal, 2 tabs");
    expect(getWorkspaceAccessibilityLabel(personal, {
      isActive: true,
      isDropTarget: false
    })).toBe("Personal, 2 tabs, active Space");
  });

  it("handles blank names and singular tab labels", () => {
    const unnamed = workspace(" ", 1);

    expect(getWorkspaceInitial(unnamed)).toBe("S");
    expect(getWorkspaceButtonLabel(unnamed)).toBe("Space, 1 tab");
    expect(getWorkspaceAccessibilityLabel(unnamed, {
      isActive: false,
      isDropTarget: true
    })).toBe("Space, 1 tab, Space, drop target");
  });

  it("labels New Space differently when it is a drop target", () => {
    expect(getNewWorkspaceAccessibilityLabel(false)).toBe("New Space");
    expect(getNewWorkspaceAccessibilityLabel(true)).toBe("Drop to create New Space");
  });

  it("does not mark workspace drop targets while dragging a tab", () => {
    expect(getWorkspaceDropTargetState({
      activeWorkspaceId: "personal",
      draggingClosedTabIndex: null,
      draggingFavoriteId: null,
      draggingGroupId: null,
      draggingTabId: "tab",
      draggingWorkspaceId: null,
      workspaceId: "work"
    })).toBe(false);
    expect(getWorkspaceDropTargetState({
      activeWorkspaceId: "personal",
      draggingClosedTabIndex: null,
      draggingFavoriteId: null,
      draggingGroupId: null,
      draggingTabId: "tab",
      draggingWorkspaceId: null,
      workspaceId: "personal"
    })).toBe(false);
  });

  it("marks workspace drop targets for cross-Space drag types that need visible destinations", () => {
    expect(getWorkspaceDropTargetState({
      activeWorkspaceId: "personal",
      draggingClosedTabIndex: null,
      draggingFavoriteId: null,
      draggingGroupId: "group",
      draggingTabId: null,
      draggingWorkspaceId: null,
      workspaceId: "work"
    })).toBe(true);
    expect(getWorkspaceDropTargetState({
      activeWorkspaceId: "personal",
      draggingClosedTabIndex: 0,
      draggingFavoriteId: null,
      draggingGroupId: null,
      draggingTabId: null,
      draggingWorkspaceId: null,
      workspaceId: "personal"
    })).toBe(true);
  });

  it("maps wheel movement to workspace cycle direction", () => {
    expect(getWorkspaceWheelDirection(0, 12)).toBe(1);
    expect(getWorkspaceWheelDirection(0, -12)).toBe(-1);
    expect(getWorkspaceWheelDirection(14, 3)).toBe(1);
    expect(getWorkspaceWheelDirection(-14, 3)).toBe(-1);
    expect(getWorkspaceWheelDirection(0.2, 0.4)).toBe(0);
  });

  it("wraps to adjacent workspaces for wheel cycling", () => {
    const workspaces = [{ id: "personal" }, { id: "work" }, { id: "later" }];

    expect(getAdjacentWorkspaceId(workspaces, "personal", 1)).toBe("work");
    expect(getAdjacentWorkspaceId(workspaces, "personal", -1)).toBe("later");
    expect(getAdjacentWorkspaceId(workspaces, "later", 1)).toBe("personal");
    expect(getAdjacentWorkspaceId([{ id: "only" }], "only", 1)).toBeNull();
  });

  it("offers a compact accent swatch set for direct Space styling", () => {
    expect(WORKSPACE_ACCENT_SWATCHES).toContain("#7dd3fc");
    expect(WORKSPACE_ACCENT_SWATCHES).toContain("#f0abfc");
    expect(new Set(WORKSPACE_ACCENT_SWATCHES).size).toBe(WORKSPACE_ACCENT_SWATCHES.length);
  });
});
