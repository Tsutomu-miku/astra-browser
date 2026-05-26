import { describe, expect, it } from "vitest";

import {
  clampListIndex,
  getNextListIndex,
  isListNavigationKey
} from "../src/renderer/common/navigation/listNavigation";

describe("list navigation", () => {
  it("clamps selected indexes to available items", () => {
    expect(clampListIndex(2, 5)).toBe(2);
    expect(clampListIndex(-1, 5)).toBe(0);
    expect(clampListIndex(9, 5)).toBe(4);
    expect(clampListIndex(Number.NaN, 5)).toBe(0);
    expect(clampListIndex(4, 0)).toBe(0);
  });

  it("wraps arrow navigation and supports Home and End", () => {
    expect(getNextListIndex(0, 4, "ArrowDown")).toBe(1);
    expect(getNextListIndex(3, 4, "ArrowDown")).toBe(0);
    expect(getNextListIndex(0, 4, "ArrowUp")).toBe(3);
    expect(getNextListIndex(2, 4, "ArrowUp")).toBe(1);
    expect(getNextListIndex(2, 4, "Home")).toBe(0);
    expect(getNextListIndex(2, 4, "End")).toBe(3);
  });

  it("identifies supported list navigation keys", () => {
    expect(isListNavigationKey("ArrowDown")).toBe(true);
    expect(isListNavigationKey("PageDown")).toBe(false);
  });
});
