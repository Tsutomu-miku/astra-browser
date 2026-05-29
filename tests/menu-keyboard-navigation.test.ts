import { createElement } from "react";
import { act } from "react";
import { createRoot } from "react-dom/client";
import { describe, expect, it, vi } from "vitest";

import { handleMenuKeyboardNavigation } from "../src/renderer/common/context-menu/menuKeyboardNavigation";

describe("menu keyboard navigation", () => {
  it("moves menu focus with Arrow, Home, and End", () => {
    const { buttons, cleanup } = renderMenu();
    buttons[0]?.focus();

    act(() => {
      buttons[0]?.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "ArrowDown" }));
    });
    expect(document.activeElement).toBe(buttons[1]);

    act(() => {
      document.activeElement?.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "End" }));
    });
    expect(document.activeElement).toBe(buttons[2]);

    act(() => {
      document.activeElement?.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "Home" }));
    });
    expect(document.activeElement).toBe(buttons[0]);

    cleanup();
  });

  it("keeps the newly focused menu item visible", () => {
    const { buttons, cleanup } = renderMenu();
    const scrollIntoView = vi.fn();
    buttons[1]!.scrollIntoView = scrollIntoView;
    buttons[0]?.focus();

    act(() => {
      buttons[0]?.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "ArrowDown" }));
    });

    expect(document.activeElement).toBe(buttons[1]);
    expect(scrollIntoView).toHaveBeenCalledWith({ block: "nearest", inline: "nearest" });

    cleanup();
  });

  it("leaves text editing keys inside menu inputs", () => {
    const { input, cleanup } = renderMenuWithInput();
    input.focus();

    act(() => {
      input.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "ArrowDown" }));
    });

    expect(document.activeElement).toBe(input);
    cleanup();
  });
});

function renderMenu() {
  const container = document.createElement("div");
  document.body.append(container);
  const root = createRoot(container);

  act(() => {
    root.render(createElement("div", {
      role: "menu",
      onKeyDown: handleMenuKeyboardNavigation
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

function renderMenuWithInput() {
  const container = document.createElement("div");
  document.body.append(container);
  const root = createRoot(container);

  act(() => {
    root.render(createElement("div", {
      role: "menu",
      onKeyDown: handleMenuKeyboardNavigation
    }, [
      createElement("input", { key: "input", "aria-label": "Rename", defaultValue: "Draft" }),
      createElement("button", { key: "button", type: "button" }, "Save")
    ]));
  });

  return {
    input: container.querySelector<HTMLInputElement>("input")!,
    cleanup: () => {
      act(() => root.unmount());
      container.remove();
    }
  };
}
