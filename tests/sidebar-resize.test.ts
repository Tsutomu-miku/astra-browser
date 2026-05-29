import { readFileSync } from "node:fs";
import { join } from "node:path";
import { createElement } from "react";
import { act } from "react";
import { createRoot } from "react-dom/client";
import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, it, vi } from "vitest";

import {
  SIDEBAR_MAX_WIDTH,
  SIDEBAR_MIN_WIDTH,
  clampSidebarWidth,
  getSidebarKeyboardResizeWidth,
  getSidebarPointerResizeWidth
} from "../src/renderer/common/layout/sidebarSizing";
import { SidebarResizeHandle } from "../src/renderer/surfaces/sidebar/components/chrome/SidebarResizeHandle";

const sidebarCss = readFileSync(join(__dirname, "../src/renderer/styles/sidebar.css"), "utf8");

describe("sidebar resize", () => {
  it("clamps sidebar width for pointer and keyboard resize", () => {
    expect(clampSidebarWidth(Number.NaN)).toBe(292);
    expect(clampSidebarWidth(100)).toBe(SIDEBAR_MIN_WIDTH);
    expect(clampSidebarWidth(800)).toBe(SIDEBAR_MAX_WIDTH);
    expect(getSidebarPointerResizeWidth({
      currentClientX: 360,
      startClientX: 100,
      startWidth: 292
    })).toBe(SIDEBAR_MAX_WIDTH);
    expect(getSidebarKeyboardResizeWidth(292, "ArrowLeft")).toBe(268);
    expect(getSidebarKeyboardResizeWidth(292, "ArrowRight")).toBe(316);
    expect(getSidebarKeyboardResizeWidth(292, "Home")).toBe(SIDEBAR_MIN_WIDTH);
    expect(getSidebarKeyboardResizeWidth(292, "End")).toBe(SIDEBAR_MAX_WIDTH);
    expect(getSidebarKeyboardResizeWidth(292, "Escape")).toBeNull();
  });

  it("renders an accessible separator handle only for expanded sidebars", () => {
    const expanded = renderToStaticMarkup(createElement(SidebarResizeHandle, {
      isCollapsed: false,
      onResize: vi.fn(),
      width: 316
    }));
    const collapsed = renderToStaticMarkup(createElement(SidebarResizeHandle, {
      isCollapsed: true,
      onResize: vi.fn(),
      width: 316
    }));

    expect(expanded).toContain('role="separator"');
    expect(expanded).toContain('aria-label="Resize sidebar"');
    expect(expanded).toContain('aria-orientation="vertical"');
    expect(expanded).toContain('aria-valuenow="316"');
    expect(collapsed).toBe("");
  });

  it("resizes from keyboard controls", () => {
    const onResize = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(SidebarResizeHandle, {
        isCollapsed: false,
        onResize,
        width: 292
      }));
    });

    const handle = container.querySelector<HTMLElement>(".sidebar-resize-handle")!;
    handle.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "ArrowRight" }));
    handle.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, key: "Home" }));

    expect(onResize).toHaveBeenCalledWith(316);
    expect(onResize).toHaveBeenCalledWith(SIDEBAR_MIN_WIDTH);

    act(() => root.unmount());
  });

  it("resets to default width on double click", () => {
    const onResize = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(SidebarResizeHandle, {
        isCollapsed: false,
        onResize,
        width: 360
      }));
    });

    container.querySelector<HTMLElement>(".sidebar-resize-handle")
      ?.dispatchEvent(new MouseEvent("dblclick", { bubbles: true }));

    expect(onResize).toHaveBeenCalledWith(292);

    act(() => root.unmount());
  });

  it("resizes from pointer drag and clears dragging state", () => {
    const onResize = vi.fn();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(SidebarResizeHandle, {
        isCollapsed: false,
        onResize,
        width: 292
      }));
    });

    const handle = container.querySelector<HTMLElement>(".sidebar-resize-handle")!;

    act(() => {
      handle.dispatchEvent(createPointerEvent("pointerdown", {
        button: 0,
        clientX: 100,
        pointerId: 7
      }));
    });
    expect(handle.dataset.dragging).toBe("true");

    act(() => {
      window.dispatchEvent(createPointerEvent("pointermove", {
        clientX: 360,
        pointerId: 7
      }));
    });
    expect(onResize).toHaveBeenCalledWith(SIDEBAR_MAX_WIDTH);

    act(() => {
      window.dispatchEvent(createPointerEvent("pointerup", {
        clientX: 360,
        pointerId: 7
      }));
    });
    expect(container.querySelector<HTMLElement>(".sidebar-resize-handle")?.dataset.dragging).toBe("false");

    act(() => root.unmount());
  });

  it("styles the sidebar resize handle and variable width", () => {
    expect(sidebarCss).toContain("width: var(--sidebar-width, 292px)");
    expect(sidebarCss).toContain(".sidebar-resize-handle");
    expect(sidebarCss).toContain("cursor: ew-resize");
    expect(sidebarCss).toContain('.sidebar-resize-handle[data-dragging="true"]::before');
  });
});

function createPointerEvent(
  type: string,
  props: { button?: number; clientX: number; pointerId: number }
) {
  const event = new Event(type, { bubbles: true, cancelable: true });
  Object.defineProperty(event, "button", { value: props.button ?? 0 });
  Object.defineProperty(event, "clientX", { value: props.clientX });
  Object.defineProperty(event, "pointerId", { value: props.pointerId });
  return event;
}
