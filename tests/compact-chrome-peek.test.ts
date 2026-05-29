import { createElement } from "react";
import { act } from "react";
import { createRoot } from "react-dom/client";
import { readFileSync } from "node:fs";
import { join } from "node:path";
import { afterEach, describe, expect, it, vi } from "vitest";

import { useCompactChromePeek } from "../src/renderer/app/controller/useCompactChromePeek";

const browserCss = readFileSync(join(__dirname, "../src/renderer/styles/browser.css"), "utf8");

describe("compact chrome peek", () => {
  afterEach(() => {
    vi.useRealTimers();
  });

  it("holds compact chrome while the top-edge target is hovered or focused", () => {
    vi.useFakeTimers();
    const container = document.createElement("div");
    document.body.append(container);
    const root = createRoot(container);

    act(() => {
      root.render(createElement(PeekHarness, { compactMode: true }));
    });

    const host = container.querySelector<HTMLElement>("[data-peeking]")!;
    const edge = container.querySelector<HTMLButtonElement>("[data-edge]")!;

    expect(host.dataset.peeking).toBe("false");

    act(() => {
      edge.dispatchEvent(new Event("pointerover", { bubbles: true }));
    });
    expect(host.dataset.peeking).toBe("true");

    act(() => {
      vi.advanceTimersByTime(2000);
    });
    expect(host.dataset.peeking).toBe("true");

    act(() => {
      edge.dispatchEvent(new Event("pointerout", { bubbles: true }));
    });
    expect(host.dataset.peeking).toBe("false");

    act(() => {
      edge.dispatchEvent(new FocusEvent("focusin", { bubbles: true }));
    });
    expect(host.dataset.peeking).toBe("true");

    act(() => {
      edge.dispatchEvent(new FocusEvent("focusout", { bubbles: true }));
    });
    expect(host.dataset.peeking).toBe("false");

    act(() => root.unmount());
    container.remove();
  });

  it("keeps command-triggered compact chrome peeks temporary", () => {
    vi.useFakeTimers();
    const container = document.createElement("div");
    document.body.append(container);
    const root = createRoot(container);

    act(() => {
      root.render(createElement(PeekHarness, { compactMode: true }));
    });

    const host = container.querySelector<HTMLElement>("[data-peeking]")!;
    const pulse = container.querySelector<HTMLButtonElement>("[data-pulse]")!;

    act(() => {
      pulse.dispatchEvent(new MouseEvent("click", { bubbles: true }));
    });
    expect(host.dataset.peeking).toBe("true");

    act(() => {
      vi.advanceTimersByTime(1400);
    });
    expect(host.dataset.peeking).toBe("false");

    act(() => root.unmount());
    container.remove();
  });

  it("styles the compact top-edge peek target as a focusable hold zone", () => {
    expect(browserCss).toContain(".compact-peek-zone");
    expect(browserCss).toContain(".compact-peek-zone:hover::after");
    expect(browserCss).toContain(".compact-peek-zone:focus-visible::after");
  });
});

function PeekHarness({ compactMode }: { compactMode: boolean }) {
  const peek = useCompactChromePeek(compactMode);

  return createElement("div", { "data-peeking": peek.compactChromePeeking },
    createElement("button", {
      "data-pulse": true,
      onClick: peek.peekCompactChrome,
      type: "button"
    }, "Pulse"),
    createElement("button", {
      "data-edge": true,
      onBlur: peek.releaseCompactChrome,
      onFocus: peek.holdCompactChrome,
      onPointerEnter: peek.holdCompactChrome,
      onPointerLeave: peek.releaseCompactChrome,
      type: "button"
    }, "Edge")
  );
}
