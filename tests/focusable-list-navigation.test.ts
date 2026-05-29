import { createElement } from "react";
import { act } from "react";
import { createRoot } from "react-dom/client";
import { describe, expect, it } from "vitest";

import { handleFocusableListNavigation } from "../src/renderer/common/focus/focusableListNavigation";

describe("focusable list navigation", () => {
  it("uses vertical Arrow keys by default", () => {
    const { buttons, cleanup } = renderList();
    buttons[0]?.focus();

    act(() => {
      buttons[0]?.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "ArrowDown" }));
    });
    expect(document.activeElement).toBe(buttons[1]);

    act(() => {
      document.activeElement?.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "ArrowRight" }));
    });
    expect(document.activeElement).toBe(buttons[1]);

    cleanup();
  });

  it("uses horizontal Arrow keys when requested", () => {
    const { buttons, cleanup } = renderList("horizontal");
    buttons[0]?.focus();

    act(() => {
      buttons[0]?.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "ArrowRight" }));
    });
    expect(document.activeElement).toBe(buttons[1]);

    act(() => {
      document.activeElement?.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "ArrowLeft" }));
    });
    expect(document.activeElement).toBe(buttons[0]);

    act(() => {
      document.activeElement?.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "End" }));
    });
    expect(document.activeElement).toBe(buttons[2]);

    cleanup();
  });
});

function renderList(orientation?: "horizontal" | "vertical") {
  const container = document.createElement("div");
  document.body.append(container);
  const root = createRoot(container);

  act(() => {
    root.render(createElement("div", {
      onKeyDown: (event) => handleFocusableListNavigation(event, "button", orientation)
    }, [
      createElement("button", { key: "one", type: "button" }, "One"),
      createElement("button", { key: "two", type: "button" }, "Two"),
      createElement("button", { key: "three", type: "button" }, "Three")
    ]));
  });

  return {
    buttons: container.querySelectorAll<HTMLButtonElement>("button"),
    cleanup: () => {
      act(() => root.unmount());
      container.remove();
    }
  };
}
