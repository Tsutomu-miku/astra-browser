import { describe, expect, it } from "vitest";

import { getAnchoredContextMenuPosition } from "../src/renderer/common/context-menu/menuPosition";

describe("anchored context menu position", () => {
  it("keeps menus inside the viewport with a quiet edge gap", () => {
    expect(getAnchoredContextMenuPosition({
      clientX: 780,
      clientY: 590
    }, {
      height: 160,
      viewport: {
        innerHeight: 600,
        innerWidth: 800,
        scrollX: 0,
        scrollY: 0
      },
      width: 220
    })).toEqual({
      left: 568,
      top: 428
    });
  });

  it("keeps menus away from the top-left edge and accounts for scroll", () => {
    expect(getAnchoredContextMenuPosition({
      clientX: 2,
      clientY: -8
    }, {
      height: 160,
      viewport: {
        innerHeight: 600,
        innerWidth: 800,
        scrollX: 50,
        scrollY: 100
      },
      width: 220
    })).toEqual({
      left: 62,
      top: 112
    });
  });
});
