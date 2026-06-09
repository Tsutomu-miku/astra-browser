import { describe, expect, it } from "vitest";

import { createClosedTab } from "../src/renderer/domain/browser";
import { getClosedTabAccessibilityLabel } from "../src/renderer/surfaces/sidebar/model/closedTabItemState";

describe("closed tab item state", () => {
  it("labels recently closed restore rows by title and restore position", () => {
    expect(getClosedTabAccessibilityLabel({
      closedIndex: 1,
      isDragging: false,
      tab: createClosedTab("Docs", "https://docs.example/", { closedAt: 1 })
    })).toBe("Docs, recently closed tab, restore position 2");
  });

  it("falls back to URL and includes dragging state", () => {
    expect(getClosedTabAccessibilityLabel({
      closedIndex: 0,
      isDragging: true,
      tab: createClosedTab("", "https://docs.example/", { closedAt: 1 })
    })).toBe("https://docs.example/, recently closed tab, restore position 1, dragging");
  });
});
