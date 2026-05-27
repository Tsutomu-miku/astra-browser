import { describe, expect, it } from "vitest";

import {
  clampOmniboxIndex,
  getNextOmniboxIndex
} from "../src/renderer/common/omnibox/omniboxSelection";

describe("omnibox selection", () => {
  it("clamps selected suggestion indexes", () => {
    expect(clampOmniboxIndex(2, 4)).toBe(2);
    expect(clampOmniboxIndex(-1, 4)).toBe(0);
    expect(clampOmniboxIndex(9, 4)).toBe(3);
    expect(clampOmniboxIndex(Number.NaN, 4)).toBe(0);
    expect(clampOmniboxIndex(2, 0)).toBe(0);
  });

  it("wraps arrow navigation and supports Home and End", () => {
    expect(getNextOmniboxIndex(0, 4, "ArrowDown")).toBe(1);
    expect(getNextOmniboxIndex(3, 4, "ArrowDown")).toBe(0);
    expect(getNextOmniboxIndex(0, 4, "ArrowUp")).toBe(3);
    expect(getNextOmniboxIndex(2, 4, "ArrowUp")).toBe(1);
    expect(getNextOmniboxIndex(2, 4, "Home")).toBe(0);
    expect(getNextOmniboxIndex(2, 4, "End")).toBe(3);
  });
});
