import { describe, expect, it } from "vitest";

import { hasSidebarSectionDragReveal } from "../src/renderer/surfaces/sidebar/model/sidebarSectionState";

describe("sidebar section state", () => {
  it("keeps drop sections collapsed while dragging a tab", () => {
    const dragging = { essentialId: null, favoriteId: null, tabId: "tab" };

    expect(hasSidebarSectionDragReveal("essentials", dragging)).toBe(false);
    expect(hasSidebarSectionDragReveal("pinned", dragging)).toBe(false);
    expect(hasSidebarSectionDragReveal("favorites", dragging)).toBe(false);
    expect(hasSidebarSectionDragReveal("tabs", dragging)).toBe(false);
    expect(hasSidebarSectionDragReveal("recentlyClosed", dragging)).toBe(false);
  });

  it("reveals only the matching quick-entry section while reordering", () => {
    expect(hasSidebarSectionDragReveal("essentials", {
      essentialId: "essential",
      favoriteId: null,
      tabId: null
    })).toBe(true);
    expect(hasSidebarSectionDragReveal("favorites", {
      essentialId: "essential",
      favoriteId: null,
      tabId: null
    })).toBe(false);
    expect(hasSidebarSectionDragReveal("favorites", {
      essentialId: null,
      favoriteId: "favorite",
      tabId: null
    })).toBe(true);
  });
});
