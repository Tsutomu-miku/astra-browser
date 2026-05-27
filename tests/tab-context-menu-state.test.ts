import { describe, expect, it } from "vitest";

import { getMoveWorkspaceTargets } from "../src/renderer/surfaces/sidebar/model/tabContextMenuState";

describe("tab context menu state", () => {
  it("lists non-active workspaces as move targets", () => {
    expect(getMoveWorkspaceTargets([
      { id: "personal", name: "Personal" },
      { id: "work", name: "Work" },
      { id: "blank", name: " " }
    ], "personal")).toEqual([
      { id: "work", name: "Work" },
      { id: "blank", name: "Space" }
    ]);
  });
});
