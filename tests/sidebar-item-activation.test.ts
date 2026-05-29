import { describe, expect, it } from "vitest";
import type { KeyboardEvent as ReactKeyboardEvent } from "react";

import { getSidebarItemKeyboardActivation } from "../src/renderer/surfaces/sidebar/model/sidebarItemActivation";

describe("sidebar item keyboard activation", () => {
  it("maps Enter modifiers to sidebar item actions", () => {
    expect(getSidebarItemKeyboardActivation(key("Enter"))).toBe("primary");
    expect(getSidebarItemKeyboardActivation(key("Enter", { altKey: true }))).toBe("preview");
    expect(getSidebarItemKeyboardActivation(key("Enter", { shiftKey: true }))).toBe("split");
    expect(getSidebarItemKeyboardActivation(key("Enter", { altKey: true, shiftKey: true }))).toBe("preview");
  });

  it("leaves global modifier shortcuts alone", () => {
    expect(getSidebarItemKeyboardActivation(key("Enter", { ctrlKey: true }))).toBeNull();
    expect(getSidebarItemKeyboardActivation(key("Enter", { metaKey: true }))).toBeNull();
    expect(getSidebarItemKeyboardActivation(key(" "))).toBeNull();
  });
});

function key(
  keyValue: string,
  modifiers: Partial<Pick<ReactKeyboardEvent<HTMLElement>, "altKey" | "ctrlKey" | "metaKey" | "shiftKey">> = {}
) {
  return {
    altKey: false,
    ctrlKey: false,
    key: keyValue,
    metaKey: false,
    shiftKey: false,
    ...modifiers
  } as ReactKeyboardEvent<HTMLElement>;
}
