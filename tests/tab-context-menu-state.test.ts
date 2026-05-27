import { describe, expect, it } from "vitest";

import { getMoveWorkspaceTargets, getTabCleanupState } from "../src/renderer/surfaces/sidebar/model/tabContextMenuState";

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

  it("describes which tab cleanup actions are available", () => {
    const tabs = [{ id: "first" }, { id: "second" }, { id: "third" }];

    expect(getTabCleanupState(tabs, "first")).toEqual({
      canCloseOtherTabs: true,
      canCloseTabsToLeft: false,
      canCloseTabsToRight: true
    });
    expect(getTabCleanupState(tabs, "second")).toEqual({
      canCloseOtherTabs: true,
      canCloseTabsToLeft: true,
      canCloseTabsToRight: true
    });
    expect(getTabCleanupState(tabs, "third")).toEqual({
      canCloseOtherTabs: true,
      canCloseTabsToLeft: true,
      canCloseTabsToRight: false
    });
  });

  it("disables tab cleanup actions for unknown or single tabs", () => {
    expect(getTabCleanupState([{ id: "only" }], "only")).toEqual({
      canCloseOtherTabs: false,
      canCloseTabsToLeft: false,
      canCloseTabsToRight: false
    });
    expect(getTabCleanupState([{ id: "only" }], "missing")).toEqual({
      canCloseOtherTabs: false,
      canCloseTabsToLeft: false,
      canCloseTabsToRight: false
    });
  });
});
