import { describe, expect, it } from "vitest";

import { getSidebarSectionKeyboardToggleIntent } from "../src/renderer/surfaces/sidebar/model/sidebarSectionKeyboard";

describe("sidebar section keyboard toggles", () => {
  it("maps horizontal arrows to tree-style section toggles", () => {
    expect(getSidebarSectionKeyboardToggleIntent("ArrowLeft", false)).toBe("collapse");
    expect(getSidebarSectionKeyboardToggleIntent("ArrowRight", true)).toBe("expand");
  });

  it("ignores arrows that would not change the current section state", () => {
    expect(getSidebarSectionKeyboardToggleIntent("ArrowLeft", true)).toBeNull();
    expect(getSidebarSectionKeyboardToggleIntent("ArrowRight", false)).toBeNull();
    expect(getSidebarSectionKeyboardToggleIntent("ArrowDown", false)).toBeNull();
  });
});
