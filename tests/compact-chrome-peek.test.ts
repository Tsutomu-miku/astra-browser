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

  it("holds compact toolbar while the top-edge target is hovered or focused", () => {
    vi.useFakeTimers();
    const container = document.createElement("div");
    document.body.append(container);
    const root = createRoot(container);

    act(() => {
      root.render(createElement(PeekHarness, { compactMode: true }));
    });

    const host = container.querySelector<HTMLElement>("[data-toolbar-peeking]")!;
    const edge = container.querySelector<HTMLButtonElement>("[data-edge]")!;

    expect(host.dataset.toolbarPeeking).toBe("false");

    act(() => {
      edge.dispatchEvent(new Event("pointerover", { bubbles: true }));
    });
    expect(host.dataset.toolbarPeeking).toBe("true");

    act(() => {
      vi.advanceTimersByTime(2000);
    });
    expect(host.dataset.toolbarPeeking).toBe("true");

    act(() => {
      edge.dispatchEvent(new Event("pointerout", { bubbles: true }));
    });
    expect(host.dataset.toolbarPeeking).toBe("false");

    act(() => {
      edge.dispatchEvent(new FocusEvent("focusin", { bubbles: true }));
    });
    expect(host.dataset.toolbarPeeking).toBe("true");

    act(() => {
      edge.dispatchEvent(new FocusEvent("focusout", { bubbles: true }));
    });
    expect(host.dataset.toolbarPeeking).toBe("false");

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

    const host = container.querySelector<HTMLElement>("[data-chrome-peeking]")!;
    const pulse = container.querySelector<HTMLButtonElement>("[data-pulse]")!;

    act(() => {
      pulse.dispatchEvent(new MouseEvent("click", { bubbles: true }));
    });
    expect(host.dataset.chromePeeking).toBe("true");

    act(() => {
      vi.advanceTimersByTime(1400);
    });
    expect(host.dataset.chromePeeking).toBe("false");

    act(() => root.unmount());
    container.remove();
  });

  it("peeks floating toolbar and sidebar independently", () => {
    vi.useFakeTimers();
    const container = document.createElement("div");
    document.body.append(container);
    const root = createRoot(container);

    act(() => {
      root.render(createElement(PeekHarness, { compactMode: true }));
    });

    const host = container.querySelector<HTMLElement>("[data-toolbar-peeking]")!;
    const sidebarPulse = container.querySelector<HTMLButtonElement>("[data-sidebar-pulse]")!;
    const toolbarPulse = container.querySelector<HTMLButtonElement>("[data-toolbar-pulse]")!;

    act(() => {
      toolbarPulse.dispatchEvent(new MouseEvent("click", { bubbles: true }));
    });
    expect(host.dataset.toolbarPeeking).toBe("true");
    expect(host.dataset.sidebarPeeking).toBe("false");

    act(() => {
      vi.advanceTimersByTime(1400);
      sidebarPulse.dispatchEvent(new MouseEvent("click", { bubbles: true }));
    });
    expect(host.dataset.toolbarPeeking).toBe("false");
    expect(host.dataset.sidebarPeeking).toBe("true");

    act(() => {
      vi.advanceTimersByTime(1400);
    });
    expect(host.dataset.sidebarPeeking).toBe("false");

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

  return createElement("div", {
    "data-chrome-peeking": peek.compactChromePeeking,
    "data-sidebar-peeking": peek.compactSidebarPeeking,
    "data-toolbar-peeking": peek.compactToolbarPeeking
  },
    createElement("button", {
      "data-pulse": true,
      onClick: peek.peekCompactChrome,
      type: "button"
    }, "Pulse"),
    createElement("button", {
      "data-sidebar-pulse": true,
      onClick: peek.peekCompactSidebar,
      type: "button"
    }, "Sidebar"),
    createElement("button", {
      "data-toolbar-pulse": true,
      onClick: peek.peekCompactToolbar,
      type: "button"
    }, "Toolbar"),
    createElement("button", {
      "data-edge": true,
      onBlur: peek.releaseCompactToolbar,
      onFocus: peek.holdCompactToolbar,
      onPointerEnter: peek.holdCompactToolbar,
      onPointerLeave: peek.releaseCompactToolbar,
      type: "button"
    }, "Edge")
  );
}
