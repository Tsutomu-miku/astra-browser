import { describe, expect, it } from "vitest";

import { createTab, type Workspace } from "../src/renderer/domain/browser";
import {
  WORKSPACE_ACCENT_SWATCHES,
  getAdjacentWorkspaceId,
  getWorkspaceButtonLabel,
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
  });

  it("handles blank names and singular tab labels", () => {
    const unnamed = workspace(" ", 1);

    expect(getWorkspaceInitial(unnamed)).toBe("S");
    expect(getWorkspaceButtonLabel(unnamed)).toBe("Space, 1 tab");
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
