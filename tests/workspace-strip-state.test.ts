import { describe, expect, it } from "vitest";

import { createTab, type Workspace } from "../src/renderer/domain/browser-core";
import {
  getWorkspaceButtonLabel,
  getWorkspaceInitial,
  getWorkspaceTabCount
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
});
