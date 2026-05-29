import { describe, expect, it } from "vitest";

import { isSidebarContextMenuKey } from "../src/renderer/surfaces/sidebar/model/sidebarKeyboardContextMenu";
import { isCloseTabKey } from "../src/renderer/surfaces/sidebar/model/sidebarTabKeyboard";

describe("sidebar tab keyboard rules", () => {
  it("limits direct keyboard closing to destructive close keys", () => {
    expect(isCloseTabKey("Delete")).toBe(true);
    expect(isCloseTabKey("Backspace")).toBe(true);
    expect(isCloseTabKey("Enter")).toBe(false);
    expect(isCloseTabKey(" ")).toBe(false);
  });

  it("opens sidebar context menus with platform context-menu keys", () => {
    expect(isSidebarContextMenuKey("ContextMenu")).toBe(true);
    expect(isSidebarContextMenuKey("F10", true)).toBe(true);
    expect(isSidebarContextMenuKey("F10", false)).toBe(false);
    expect(isSidebarContextMenuKey("Enter")).toBe(false);
  });
});
