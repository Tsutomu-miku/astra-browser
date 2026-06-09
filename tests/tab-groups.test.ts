import { describe, expect, it } from "vitest";

import {
  assignTabToGroup,
  closeActiveTab,
  duplicateTabGroup,
  groupActiveTab,
  groupTab,
  moveTabGroupToNewWorkspace,
  moveTabGroupToWorkspace,
  moveTabToWorkspace,
  openUrlInActiveWorkspace,
  reorderTabGroup,
  toggleActiveTabPinned,
  toggleTabGroupCollapsed,
  ungroupActiveTab,
  ungroupTab,
  ungroupTabGroup,
  updateTabGroup
} from "../src/renderer/domain/actions";
import { createDefaultState, normalizeState } from "../src/renderer/domain/browser";
import { getActiveTab, getActiveWorkspace } from "../src/renderer/domain/browser/selectors";
import { getGroupedTabs } from "../src/renderer/domain/tabs/groups";

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
            { id: "tab", title: "Docs", url: "docs.example", groupId: "group", isFavorite: false },
            { id: "orphan", title: "Orphan", url: "orphan.example", groupId: "missing", isFavorite: false }
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

  it("moves a whole tab group to another workspace", () => {
    const first = groupActiveTab(openUrlInActiveWorkspace(createDefaultState(), "docs.example", "Docs"));
    const group = getActiveWorkspace(first).tabGroups[0];
    const opened = openUrlInActiveWorkspace(first, "news.example", "News");
    const secondTab = getActiveTab(getActiveWorkspace(opened));
    const assigned = assignTabToGroup(opened, secondTab.id, group.id);
    const moved = moveTabGroupToWorkspace(assigned, group.id, "work");
    const source = moved.workspaces.find((workspace) => workspace.id === "personal")!;
    const target = moved.workspaces.find((workspace) => workspace.id === "work")!;

    expect(moved.activeWorkspaceId).toBe("work");
    expect(source.tabGroups).toHaveLength(0);
    expect(source.tabs.map((tab) => tab.title)).toEqual(["New Tab", "Chromium", "MDN"]);
    expect(target.tabGroups[0]).toMatchObject({ id: group.id, name: group.name, color: group.color });
    expect(target.tabs.filter((tab) => tab.groupId === group.id).map((tab) => tab.title)).toEqual(["Docs", "News"]);
    expect(target.activeTabId).toBe(secondTab.id);
  });

  it("moves a whole tab group into a new workspace", () => {
    const first = groupActiveTab(openUrlInActiveWorkspace(createDefaultState(), "docs.example", "Docs"));
    const group = getActiveWorkspace(first).tabGroups[0];
    const opened = openUrlInActiveWorkspace(first, "news.example", "News");
    const secondTab = getActiveTab(getActiveWorkspace(opened));
    const assigned = assignTabToGroup(opened, secondTab.id, group.id);
    const moved = moveTabGroupToNewWorkspace(assigned, group.id);
    const source = moved.workspaces.find((workspace) => workspace.id === "personal")!;
    const target = getActiveWorkspace(moved);

    expect(moved.workspaces).toHaveLength(assigned.workspaces.length + 1);
    expect(target.name).toBe(group.name);
    expect(source.tabGroups).toHaveLength(0);
    expect(target.tabGroups[0]).toMatchObject({ id: group.id, name: group.name, color: group.color });
    expect(target.tabs.filter((tab) => tab.groupId === group.id).map((tab) => tab.title)).toEqual(["Docs", "News"]);
    expect(target.activeTabId).toBe(secondTab.id);
  });

  it("duplicates a whole tab group next to the source group", () => {
    const first = groupActiveTab(openUrlInActiveWorkspace(createDefaultState(), "docs.example", "Docs"));
    const group = getActiveWorkspace(first).tabGroups[0];
    const opened = openUrlInActiveWorkspace(first, "news.example", "News");
    const secondTab = getActiveTab(getActiveWorkspace(opened));
    const assigned = assignTabToGroup(opened, secondTab.id, group.id);
    const duplicated = duplicateTabGroup(assigned, group.id);
    const workspace = getActiveWorkspace(duplicated);
    const copyGroup = workspace.tabGroups[1];
    const copiedTabs = workspace.tabs.filter((tab) => tab.groupId === copyGroup.id);

    expect(copyGroup).toMatchObject({ name: `${group.name} Copy`, color: group.color, isCollapsed: false });
    expect(copiedTabs.map((tab) => tab.title)).toEqual(["Docs", "News"]);
    expect(copiedTabs.map((tab) => tab.url)).toEqual(["https://docs.example/", "https://news.example/"]);
    expect(copiedTabs.every((tab) => !tab.isSleeping && !tab.isLoading && !tab.canGoBack && !tab.canGoForward)).toBe(true);
    expect(workspace.activeTabId).toBe(copiedTabs[0].id);
    expect(new Set(copiedTabs.map((tab) => tab.id)).has(secondTab.id)).toBe(false);
    expect(duplicated.splitMode).toBe(false);
  });

  it("reorders tab groups inside the active workspace", () => {
    const state = createDefaultState();
    const workspace = getActiveWorkspace(state);
    workspace.tabGroups = [
      { id: "first", name: "First", color: "#7dd3fc", isCollapsed: false },
      { id: "second", name: "Second", color: "#f0abfc", isCollapsed: false },
      { id: "third", name: "Third", color: "#86d39d", isCollapsed: false }
    ];

    const movedBefore = reorderTabGroup(state, "third", "first", "before");
    const movedAfter = reorderTabGroup(movedBefore, "first", "second", "after");
    const ignored = reorderTabGroup(movedAfter, "second", "second", "before");

    expect(getActiveWorkspace(movedBefore).tabGroups.map((group) => group.name)).toEqual(["Third", "First", "Second"]);
    expect(getActiveWorkspace(movedAfter).tabGroups.map((group) => group.name)).toEqual(["Third", "Second", "First"]);
    expect(ignored).toBe(movedAfter);
  });

  it("ungroups every tab in a group without changing selection", () => {
    const first = groupActiveTab(openUrlInActiveWorkspace(createDefaultState(), "docs.example", "Docs"));
    const group = getActiveWorkspace(first).tabGroups[0];
    const opened = openUrlInActiveWorkspace(first, "news.example", "News");
    const secondTab = getActiveTab(getActiveWorkspace(opened));
    const second = assignTabToGroup(opened, secondTab.id, group.id);
    const activeBefore = getActiveTab(getActiveWorkspace(second));
    const ungrouped = ungroupTabGroup(second, group.id);
    const workspace = getActiveWorkspace(ungrouped);

    expect(workspace.tabGroups).toHaveLength(0);
    expect(workspace.tabs.filter((tab) => tab.groupId === group.id)).toHaveLength(0);
    expect(getActiveTab(workspace).id).toBe(activeBefore.id);
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
