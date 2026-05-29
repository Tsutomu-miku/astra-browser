import { describe, expect, it } from "vitest";

import {
  clearDropPlacement,
  getPointerDropPlacement,
  updateDropPlacement
} from "../src/renderer/common/drag-drop/dropPlacement";

describe("drop placement", () => {
  it("calculates vertical before and after placements", () => {
    const target = targetWithRect({ top: 10, height: 40 });

    expect(getPointerDropPlacement(target, { clientX: 0, clientY: 20 }, "vertical")).toBe("before");
    expect(getPointerDropPlacement(target, { clientX: 0, clientY: 40 }, "vertical")).toBe("after");
  });

  it("calculates horizontal before and after placements", () => {
    const target = targetWithRect({ left: 20, width: 80 });

    expect(getPointerDropPlacement(target, { clientX: 40, clientY: 0 }, "horizontal")).toBe("before");
    expect(getPointerDropPlacement(target, { clientX: 80, clientY: 0 }, "horizontal")).toBe("after");
  });

  it("writes and clears placement dataset state", () => {
    const target = targetWithRect({ top: 0, height: 20 });

    expect(updateDropPlacement(target, { clientX: 0, clientY: 15 })).toBe("after");
    expect(target.dataset.dropPlacement).toBe("after");

    clearDropPlacement(target);
    expect(target.dataset.dropPlacement).toBeUndefined();
  });
});

function targetWithRect(rect: Partial<DOMRect>): HTMLElement {
  const target = document.createElement("button");
  Object.defineProperty(target, "getBoundingClientRect", {
    value: () => ({
      bottom: (rect.top ?? 0) + (rect.height ?? 0),
      height: rect.height ?? 0,
      left: rect.left ?? 0,
      right: (rect.left ?? 0) + (rect.width ?? 0),
      top: rect.top ?? 0,
      width: rect.width ?? 0,
      x: rect.left ?? 0,
      y: rect.top ?? 0,
      toJSON: () => undefined
    })
  });
  return target;
}
