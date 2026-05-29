import { describe, expect, it } from "vitest";

import { createFavorite } from "../src/renderer/domain/browser";
import { getQuickEntryAccessibilityLabel } from "../src/renderer/surfaces/sidebar/model/quickEntryItemState";

describe("quick entry item state", () => {
  it("labels active Essentials with search and drag state", () => {
    const entry = createFavorite("Docs", "https://docs.example");

    expect(getQuickEntryAccessibilityLabel({
      entry,
      isActive: true,
      isDragging: true,
      isDropTarget: false,
      isSearchSelected: true,
      kind: "essential"
    })).toBe("Docs, Essential, current page, selected search result, dragging");
  });

  it("labels favorite drop targets", () => {
    const entry = createFavorite("", "https://docs.example");

    expect(getQuickEntryAccessibilityLabel({
      entry,
      isActive: false,
      isDragging: false,
      isDropTarget: true,
      isSearchSelected: false,
      kind: "favorite"
    })).toBe("https://docs.example, Favorite, drop target");
  });
});
