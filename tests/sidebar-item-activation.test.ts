import { describe, expect, it } from "vitest";
import type { KeyboardEvent as ReactKeyboardEvent, MouseEvent as ReactMouseEvent } from "react";

import {
  getSidebarItemKeyboardActivation,
  getSidebarItemPointerActivation,
  runSidebarItemPointerActivation
} from "../src/renderer/surfaces/sidebar/model/sidebarItemActivation";

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

  it("maps pointer modifiers to the same sidebar item actions", () => {
    expect(getSidebarItemPointerActivation(pointer())).toBe("primary");
    expect(getSidebarItemPointerActivation(pointer({ altKey: true }))).toBe("preview");
    expect(getSidebarItemPointerActivation(pointer({ shiftKey: true }))).toBe("split");
    expect(getSidebarItemPointerActivation(pointer({ altKey: true, shiftKey: true }))).toBe("preview");
  });

  it("runs pointer activation handlers without remapping item semantics per section", () => {
    const calls: string[] = [];

    runSidebarItemPointerActivation(pointer({ shiftKey: true }), {
      primary: () => calls.push("primary"),
      preview: () => calls.push("preview"),
      split: () => calls.push("split")
    });

    expect(calls).toEqual(["split"]);
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

function pointer(modifiers: Partial<Pick<ReactMouseEvent<HTMLElement>, "altKey" | "shiftKey">> = {}) {
  return {
    altKey: false,
    shiftKey: false,
    ...modifiers
  } as ReactMouseEvent<HTMLElement>;
}
