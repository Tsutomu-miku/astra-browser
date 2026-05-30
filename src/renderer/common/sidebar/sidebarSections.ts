export const SIDEBAR_SECTION_IDS = [
  "essentials",
  "pinned",
  "favorites",
  "tabs",
  "recentlyClosed"
] as const;

export type SidebarSectionId = typeof SIDEBAR_SECTION_IDS[number];
export type SidebarSectionCollapsedState = Record<SidebarSectionId, boolean>;

export const DEFAULT_SIDEBAR_SECTION_COLLAPSED: SidebarSectionCollapsedState = {
  essentials: false,
  pinned: false,
  favorites: false,
  tabs: false,
  recentlyClosed: false
};

export function normalizeSidebarSectionCollapsedState(
  value: unknown,
  fallback: SidebarSectionCollapsedState = DEFAULT_SIDEBAR_SECTION_COLLAPSED
): SidebarSectionCollapsedState {
  const candidate = isRecord(value) ? value : {};

  return SIDEBAR_SECTION_IDS.reduce<SidebarSectionCollapsedState>((state, sectionId) => {
    state[sectionId] = typeof candidate[sectionId] === "boolean"
      ? candidate[sectionId]
      : fallback[sectionId];
    return state;
  }, { ...DEFAULT_SIDEBAR_SECTION_COLLAPSED });
}

export function toggleSidebarSectionCollapsed(
  current: SidebarSectionCollapsedState,
  sectionId: SidebarSectionId
): SidebarSectionCollapsedState {
  return {
    ...current,
    [sectionId]: !current[sectionId]
  };
}

function isRecord(value: unknown): value is Record<string, unknown> {
  return Boolean(value) && typeof value === "object";
}
