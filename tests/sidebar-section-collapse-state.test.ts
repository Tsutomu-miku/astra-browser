import { describe, expect, it } from "vitest";

import {
  DEFAULT_SIDEBAR_SECTION_COLLAPSED,
  normalizeSidebarSectionCollapsedState,
  toggleSidebarSectionCollapsed
} from "../src/renderer/common/sidebar/sidebarSections";

describe("sidebar section collapse state", () => {
  it("normalizes known sidebar folders and ignores unknown persisted keys", () => {
    expect(normalizeSidebarSectionCollapsedState({
      essentials: true,
      pinned: true,
      unknown: true
    })).toEqual({
      essentials: true,
      pinned: true,
      favorites: false,
      tabs: false,
      recentlyClosed: false
    });
  });

  it("uses an existing fallback when restoring partial persisted state", () => {
    const fallback = {
      ...DEFAULT_SIDEBAR_SECTION_COLLAPSED,
      favorites: true,
      tabs: true
    };

    expect(normalizeSidebarSectionCollapsedState({ tabs: false }, fallback)).toEqual({
      essentials: false,
      pinned: false,
      favorites: true,
      tabs: false,
      recentlyClosed: false
    });
  });

  it("toggles one section without mutating the current state", () => {
    const next = toggleSidebarSectionCollapsed(DEFAULT_SIDEBAR_SECTION_COLLAPSED, "favorites");

    expect(next.favorites).toBe(true);
    expect(DEFAULT_SIDEBAR_SECTION_COLLAPSED.favorites).toBe(false);
  });
});
