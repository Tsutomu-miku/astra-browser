import { describe, expect, it } from "vitest";

import { formatZoomPercent, normalizeZoomFactor, stepZoomFactor } from "../src/renderer/domain/browser/zoom";

describe("zoom helpers", () => {
  it("normalizes invalid and out-of-range zoom factors", () => {
    expect(normalizeZoomFactor("bad")).toBe(1);
    expect(normalizeZoomFactor(0.1)).toBe(0.25);
    expect(normalizeZoomFactor(4)).toBe(3);
  });

  it("steps and formats zoom factors", () => {
    expect(stepZoomFactor(1, 1)).toBe(1.1);
    expect(stepZoomFactor(1, -1)).toBe(0.9);
    expect(formatZoomPercent(1.25)).toBe("125%");
  });
});
