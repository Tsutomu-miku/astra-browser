import { describe, expect, it } from "vitest";

import { resolveShortcut, type ShortcutEventLike } from "../src/renderer/hooks/keyboardShortcuts";

function event(overrides: Partial<ShortcutEventLike>): ShortcutEventLike {
  return {
    altKey: false,
    ctrlKey: false,
    key: "",
    metaKey: false,
    shiftKey: false,
    ...overrides
  };
}

describe("resolveShortcut", () => {
  it("handles core browser command shortcuts", () => {
    expect(resolveShortcut(event({ ctrlKey: true, key: "t" }))).toEqual({ type: "newTab" });
    expect(resolveShortcut(event({ metaKey: true, key: "w" }))).toEqual({ type: "closeTab" });
    expect(resolveShortcut(event({ ctrlKey: true, shiftKey: true, key: "T" }))).toEqual({ type: "restoreClosedTab" });
  });

  it("handles zoom shortcuts", () => {
    expect(resolveShortcut(event({ ctrlKey: true, key: "=" }))).toEqual({ type: "zoomIn" });
    expect(resolveShortcut(event({ ctrlKey: true, key: "-" }))).toEqual({ type: "zoomOut" });
    expect(resolveShortcut(event({ ctrlKey: true, key: "0" }))).toEqual({ type: "resetZoom" });
  });

  it("handles command, address, split, and escape shortcuts", () => {
    expect(resolveShortcut(event({ ctrlKey: true, key: "k" }))).toEqual({ type: "openCommand" });
    expect(resolveShortcut(event({ ctrlKey: true, key: "l" }))).toEqual({ type: "focusAddress" });
    expect(resolveShortcut(event({ ctrlKey: true, key: "f" }))).toEqual({ type: "openFind" });
    expect(resolveShortcut(event({ ctrlKey: true, key: "b" }))).toEqual({ type: "toggleSidebar" });
    expect(resolveShortcut(event({ metaKey: true, key: "\\" }))).toEqual({ type: "toggleSplit" });
    expect(resolveShortcut(event({ key: "Escape" }))).toEqual({ type: "closePanels" });
  });

  it("maps number shortcuts to workspaces and tabs", () => {
    expect(resolveShortcut(event({ ctrlKey: true, key: "3" }))).toEqual({ type: "selectWorkspaceIndex", index: 2 });
    expect(resolveShortcut(event({ altKey: true, key: "2" }))).toEqual({ type: "selectTabIndex", index: 1 });
  });

  it("handles tab cycling shortcuts", () => {
    expect(resolveShortcut(event({ ctrlKey: true, key: "Tab" }))).toEqual({ type: "selectAdjacentTab", direction: 1 });
    expect(resolveShortcut(event({ ctrlKey: true, shiftKey: true, key: "Tab" }))).toEqual({ type: "selectAdjacentTab", direction: -1 });
  });

  it("ignores unrelated shortcuts", () => {
    expect(resolveShortcut(event({ key: "a" }))).toBeNull();
    expect(resolveShortcut(event({ ctrlKey: true, key: "ArrowLeft" }))).toBeNull();
  });
});
