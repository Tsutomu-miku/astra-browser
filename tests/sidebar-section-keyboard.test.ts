import { describe, expect, it } from "vitest";

import { getDisclosureKeyboardToggleIntent } from "../src/renderer/common/disclosure/disclosureKeyboard";

describe("disclosure keyboard toggles", () => {
  it("maps horizontal arrows to tree-style section toggles", () => {
    expect(getDisclosureKeyboardToggleIntent("ArrowLeft", false)).toBe("collapse");
    expect(getDisclosureKeyboardToggleIntent("ArrowRight", true)).toBe("expand");
  });

  it("ignores arrows that would not change the current section state", () => {
    expect(getDisclosureKeyboardToggleIntent("ArrowLeft", true)).toBeNull();
    expect(getDisclosureKeyboardToggleIntent("ArrowRight", false)).toBeNull();
    expect(getDisclosureKeyboardToggleIntent("ArrowDown", false)).toBeNull();
  });
});
