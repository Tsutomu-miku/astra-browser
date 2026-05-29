import { describe, expect, it } from "vitest";

import { getClosedTabAccessibilityLabel } from "../src/renderer/surfaces/sidebar/model/closedTabItemState";

describe("closed tab item state", () => {
  it("labels recently closed restore rows by title and restore position", () => {
    expect(getClosedTabAccessibilityLabel({
      closedIndex: 1,
      isDragging: false,
      tab: { closedAt: 1, title: "Docs", url: "https://docs.example/" }
    })).toBe("Docs, recently closed tab, restore position 2");
  });

  it("falls back to URL and includes dragging state", () => {
    expect(getClosedTabAccessibilityLabel({
      closedIndex: 0,
      isDragging: true,
      tab: { closedAt: 1, title: "", url: "https://docs.example/" }
    })).toBe("https://docs.example/, recently closed tab, restore position 1, dragging");
  });
});
