import { createElement } from "react";
import { act } from "react";
import { createRoot } from "react-dom/client";
import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, it, vi } from "vitest";

import { SidebarSearchBox } from "../src/renderer/surfaces/sidebar/components/chrome/SidebarSearchBox";

describe("sidebar search box", () => {
  it("renders an explicitly labelled clear control while searching", () => {
    const html = renderToStaticMarkup(createElement(SidebarSearchBox, {
      query: "docs",
      onClear: vi.fn(),
      onKeyDown: vi.fn(),
      onQueryChange: vi.fn()
    }));

    expect(html).toContain('aria-label="Clear sidebar search"');
    expect(html).not.toContain('title="Clear sidebar search"');
  });

  it("clears search without dropping keyboard focus", () => {
    const onClear = vi.fn();
    const container = document.createElement("div");
    document.body.append(container);
    const root = createRoot(container);

    act(() => {
      root.render(createElement(SidebarSearchBox, {
        query: "docs",
        onClear,
        onKeyDown: vi.fn(),
        onQueryChange: vi.fn()
      }));
    });

    const input = container.querySelector<HTMLInputElement>("input")!;
    input.focus();

    act(() => {
      container.querySelector<HTMLButtonElement>(".icon-button")?.dispatchEvent(new MouseEvent("click", {
        bubbles: true
      }));
    });

    expect(onClear).toHaveBeenCalled();
    expect(document.activeElement).toBe(input);

    act(() => root.unmount());
    container.remove();
  });

  it("wires Escape to the parent keyboard handler", () => {
    const onKeyDown = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(SidebarSearchBox, {
        query: "docs",
        onClear: vi.fn(),
        onKeyDown,
        onQueryChange: vi.fn()
      }));
    });

    container.querySelector("input")?.dispatchEvent(new KeyboardEvent("keydown", {
      bubbles: true,
      key: "Escape"
    }));

    expect(onKeyDown).toHaveBeenCalledWith(expect.objectContaining({ key: "Escape" }));

    act(() => root.unmount());
  });
});
