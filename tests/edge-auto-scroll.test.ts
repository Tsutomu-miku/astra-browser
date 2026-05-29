import { describe, expect, it } from "vitest";

import {
  getEdgeAutoScrollDelta,
  scrollElementNearEdge
} from "../src/renderer/common/drag-drop/edgeAutoScroll";

describe("edge auto scroll", () => {
  it("does not scroll when the pointer is away from the vertical edges", () => {
    const element = createScrollableElement({ clientHeight: 100, scrollHeight: 800, top: 0 });

    expect(getEdgeAutoScrollDelta(element, 50, { edgeSize: 20 })).toBe(0);
    expect(scrollElementNearEdge(element, 50, { edgeSize: 20 })).toBe(false);
    expect(element.scrollTop).toBe(0);
  });

  it("scrolls upward near the top edge and clamps at the start", () => {
    const element = createScrollableElement({ clientHeight: 100, scrollHeight: 800, top: 0 });
    element.scrollTop = 20;

    expect(getEdgeAutoScrollDelta(element, 4, { edgeSize: 20, maxDelta: 10, minDelta: 2 })).toBeLessThan(0);
    expect(scrollElementNearEdge(element, 4, { edgeSize: 20, maxDelta: 10, minDelta: 2 })).toBe(true);
    expect(element.scrollTop).toBeLessThan(20);

    element.scrollTop = 0;
    expect(scrollElementNearEdge(element, 4, { edgeSize: 20, maxDelta: 10 })).toBe(false);
    expect(element.scrollTop).toBe(0);
  });

  it("scrolls downward near the bottom edge and clamps at the end", () => {
    const element = createScrollableElement({ clientHeight: 100, scrollHeight: 800, top: 0 });
    element.scrollTop = 40;

    expect(getEdgeAutoScrollDelta(element, 96, { edgeSize: 20, maxDelta: 10, minDelta: 2 })).toBeGreaterThan(0);
    expect(scrollElementNearEdge(element, 96, { edgeSize: 20, maxDelta: 10, minDelta: 2 })).toBe(true);
    expect(element.scrollTop).toBeGreaterThan(40);

    element.scrollTop = 700;
    expect(scrollElementNearEdge(element, 96, { edgeSize: 20, maxDelta: 10 })).toBe(false);
    expect(element.scrollTop).toBe(700);
  });
});

function createScrollableElement({
  clientHeight,
  scrollHeight,
  top
}: {
  clientHeight: number;
  scrollHeight: number;
  top: number;
}) {
  const element = document.createElement("div");
  Object.defineProperty(element, "clientHeight", { configurable: true, value: clientHeight });
  Object.defineProperty(element, "scrollHeight", { configurable: true, value: scrollHeight });
  Object.defineProperty(element, "getBoundingClientRect", {
    configurable: true,
    value: () => ({
      bottom: top + clientHeight,
      height: clientHeight,
      left: 0,
      right: 200,
      top,
      width: 200,
      x: 0,
      y: top,
      toJSON: () => ({})
    })
  });

  return element;
}
