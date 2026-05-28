import type { BrowserTab, TabGroup, Workspace } from "../browser/types";

export const TAB_GROUP_COLOR_SWATCHES = ["#7dd3fc", "#f0abfc", "#86efac", "#fda4af", "#fde68a", "#c4b5fd"];

export function normalizeTabGroupColor(value: unknown, index = 0): string {
  return isHexColor(value) ? value : getGroupColor(index);
}

export function normalizeTabGroupName(value: unknown): string {
  return String(value ?? "").trim() || "Group";
}

export function createTabGroup(name: string, index = 0): TabGroup {
  return {
    id: createId(),
    name: name.trim() || "Group",
    color: getGroupColor(index),
    isCollapsed: false
  };
}

export function normalizeTabGroups(groups: Array<Partial<TabGroup> | null> | undefined): TabGroup[] {
  if (!Array.isArray(groups)) return [];

  return groups
    .filter((group): group is Partial<TabGroup> => Boolean(group))
    .map((group, index) => ({
      id: group.id ?? createId(),
      name: normalizeTabGroupName(group.name || `Group ${index + 1}`),
      color: normalizeTabGroupColor(group.color, index),
      isCollapsed: Boolean(group.isCollapsed)
    }));
}

export function getGroupedTabs(workspace: Workspace): Array<{ group: TabGroup; tabs: BrowserTab[] }> {
  return workspace.tabGroups
    .map((group) => ({
      group,
      tabs: workspace.tabs.filter((tab) => tab.groupId === group.id && !tab.isPinned)
    }))
    .filter((entry) => entry.tabs.length > 0);
}

export function pruneEmptyTabGroups(workspace: Workspace) {
  const groupIds = new Set(workspace.tabs.map((tab) => tab.groupId).filter(Boolean));
  workspace.tabGroups = workspace.tabGroups.filter((group) => groupIds.has(group.id));
}

function isHexColor(value: unknown): value is string {
  return typeof value === "string" && /^#[0-9a-f]{6}$/i.test(value);
}

function createId(): string {
  return globalThis.crypto?.randomUUID?.() ?? `${Date.now()}-${Math.random().toString(16).slice(2)}`;
}

function getGroupColor(index: number): string {
  return TAB_GROUP_COLOR_SWATCHES[index % TAB_GROUP_COLOR_SWATCHES.length];
}
