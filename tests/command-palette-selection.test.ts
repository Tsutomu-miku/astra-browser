import { describe, expect, it } from "vitest";

import {
  clampCommandIndex,
  getNextCommandIndex
} from "../src/renderer/hooks/commandPaletteSelection";

describe("command palette selection", () => {
  it("clamps active command indexes to available commands", () => {
    expect(clampCommandIndex(2, 5)).toBe(2);
    expect(clampCommandIndex(-1, 5)).toBe(0);
    expect(clampCommandIndex(9, 5)).toBe(4);
    expect(clampCommandIndex(Number.NaN, 5)).toBe(0);
    expect(clampCommandIndex(4, 0)).toBe(0);
  });

  it("wraps arrow navigation and supports Home and End", () => {
    expect(getNextCommandIndex(0, 4, "ArrowDown")).toBe(1);
    expect(getNextCommandIndex(3, 4, "ArrowDown")).toBe(0);
    expect(getNextCommandIndex(0, 4, "ArrowUp")).toBe(3);
    expect(getNextCommandIndex(2, 4, "ArrowUp")).toBe(1);
    expect(getNextCommandIndex(2, 4, "Home")).toBe(0);
    expect(getNextCommandIndex(2, 4, "End")).toBe(3);
  });
});
