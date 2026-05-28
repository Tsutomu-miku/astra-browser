import { describe, expect, it } from "vitest";

import { isCloseTabKey } from "../src/renderer/surfaces/sidebar/model/sidebarTabKeyboard";

describe("sidebar tab keyboard rules", () => {
  it("limits direct keyboard closing to destructive close keys", () => {
    expect(isCloseTabKey("Delete")).toBe(true);
    expect(isCloseTabKey("Backspace")).toBe(true);
    expect(isCloseTabKey("Enter")).toBe(false);
    expect(isCloseTabKey(" ")).toBe(false);
  });
});
