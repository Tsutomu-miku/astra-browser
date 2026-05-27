import { describe, expect, it } from "vitest";

import {
  assignTabToGroup,
  closeActiveTab,
  groupActiveTab,
  groupTab,
  moveTabToWorkspace,
  openUrlInActiveWorkspace,
  toggleActiveTabPinned,
  toggleTabGroupCollapsed,
  ungroupActiveTab,
  ungroupTab,
  updateTabGroup
} from "../src/renderer/domain/browser-actions";
import { createDefaultState, normalizeState } from "../src/renderer/domain/browser-core";
import { getActiveTab, getActiveWorkspace } from "../src/renderer/domain/selectors";
import { getGroupedTabs } from "../src/renderer/domain/tab-groups";

describe("tab groups", () => {
  it("normalizes persisted groups and tab membership", () => {
    const state = normalizeState({
      activeWorkspaceId: "space",
      workspaces: [
        {
          id: "space",
          name: "Space",
          tabGroups: [{ id: "group", name: "Docs", color: "bad", isCollapsed: true }],
          tabs: [
            { id: "tab", title: "Docs", url: "docs.example", groupId: "group" },
            { id: "orphan", title: "Orphan", url: "orphan.example", groupId: "missing" }
          ]
        }
      ]
    });
    const workspace = getActiveWorkspace(state);

    expect(workspace.tabGroups[0]).toMatchObject({ id: "group", name: "Docs", isCollapsed: true });
    expect(workspace.tabs[0].groupId).toBe("group");
    expect(workspace.tabs[1].groupId).toBeNull();
  });

  it("groups, collapses, and ungroups the active tab", () => {
    const opened = openUrlInActiveWorkspace(createDefaultState(), "docs.example", "Docs");
    const grouped = groupActiveTab(opened);
    const group = getActiveWorkspace(grouped).tabGroups[0];
    const collapsed = toggleTabGroupCollapsed(grouped, group.id);
    const ungrouped = ungroupActiveTab(collapsed);

    expect(getActiveTab(getActiveWorkspace(grouped)).groupId).toBe(group.id);
    expect(getGroupedTabs(getActiveWorkspace(grouped))[0].tabs).toHaveLength(1);
    expect(getActiveWorkspace(collapsed).tabGroups[0].isCollapsed).toBe(true);
    expect(getActiveWorkspace(ungrouped).tabGroups).toHaveLength(0);
  });

  it("groups and ungroups a background target tab without requiring selection first", () => {
    const first = openUrlInActiveWorkspace(createDefaultState(), "docs.example", "Docs");
    const second = openUrlInActiveWorkspace(first, "news.example", "News");
    const workspace = getActiveWorkspace(second);
    const docsTab = workspace.tabs.find((tab) => tab.title === "Docs")!;
    const grouped = groupTab(second, docsTab.id);
    const groupedWorkspace = getActiveWorkspace(grouped);
    const groupedDocsTab = groupedWorkspace.tabs.find((tab) => tab.id === docsTab.id)!;
    const group = groupedWorkspace.tabGroups[0];
    const ungrouped = ungroupTab(grouped, docsTab.id);

    expect(getActiveTab(groupedWorkspace).title).toBe("News");
    expect(groupedDocsTab.groupId).toBe(group.id);
    expect(getActiveTab(getActiveWorkspace(ungrouped)).title).toBe("News");
    expect(getActiveWorkspace(ungrouped).tabs.find((tab) => tab.id === docsTab.id)?.groupId).toBeNull();
    expect(getActiveWorkspace(ungrouped).tabGroups).toHaveLength(0);
  });

  it("moves another regular tab into an existing group", () => {
    const first = groupActiveTab(openUrlInActiveWorkspace(createDefaultState(), "docs.example", "Docs"));
    const group = getActiveWorkspace(first).tabGroups[0];
    const second = openUrlInActiveWorkspace(first, "news.example", "News");
    const secondTab = getActiveTab(getActiveWorkspace(second));
    const assigned = assignTabToGroup(second, secondTab.id, group.id);

    expect(getGroupedTabs(getActiveWorkspace(assigned))[0].tabs.map((tab) => tab.title)).toEqual(["Docs", "News"]);
  });

  it("updates tab group name and color with normalized fallbacks", () => {
    const grouped = groupActiveTab(openUrlInActiveWorkspace(createDefaultState(), "docs.example", "Docs"));
    const group = getActiveWorkspace(grouped).tabGroups[0];
    const renamed = updateTabGroup(grouped, group.id, { name: "Research", color: "#123456" });
    const fallback = updateTabGroup(renamed, group.id, { name: "", color: "bad" });

    expect(getActiveWorkspace(renamed).tabGroups[0]).toMatchObject({ name: "Research", color: "#123456" });
    expect(getActiveWorkspace(fallback).tabGroups[0].name).toBe("Group");
    expect(getActiveWorkspace(fallback).tabGroups[0].color).toBe("#7dd3fc");
  });

  it("prunes empty groups when grouped tabs leave the workspace or become pinned", () => {
    const grouped = groupActiveTab(openUrlInActiveWorkspace(createDefaultState(), "docs.example", "Docs"));
    const activeTab = getActiveTab(getActiveWorkspace(grouped));
    const moved = moveTabToWorkspace(grouped, activeTab.id, "work");
    const source = moved.workspaces.find((workspace) => workspace.id === "personal")!;
    const pinned = toggleActiveTabPinned(grouped);
    const closed = closeActiveTab(grouped);

    expect(source.tabGroups).toHaveLength(0);
    expect(getActiveWorkspace(pinned).tabGroups).toHaveLength(0);
    expect(getActiveWorkspace(closed).tabGroups).toHaveLength(0);
  });
});
