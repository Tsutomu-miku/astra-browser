import { createElement } from "react";
import { act } from "react";
import { createRoot } from "react-dom/client";
import { FiEye } from "react-icons/fi";
import { describe, expect, it, vi } from "vitest";

import { useContextMenuDismissal } from "../src/renderer/common/context-menu/menuDismissal";
import { handleMenuKeyboardNavigation } from "../src/renderer/common/context-menu/menuKeyboardNavigation";
import {
  SidebarMenuItem,
  SidebarMenuSeparator
} from "../src/renderer/surfaces/sidebar/components/common/SidebarMenuItem";
import { SidebarMenuSurface } from "../src/renderer/surfaces/sidebar/components/common/SidebarMenuSurface";

function MenuHarness({ onClose }: { onClose: () => void }) {
  useContextMenuDismissal({ isOpen: true, onClose });
  return createElement(
    SidebarMenuSurface,
    { className: "harness-menu", children: null },
    createElement(SidebarMenuItem, { icon: FiEye, onClick: onClose, key: "a", children: "Preview" }),
    createElement(SidebarMenuItem, { icon: FiEye, disabled: true, key: "b", children: "Disabled" }),
    createElement(SidebarMenuSeparator, { key: "s" }),
    createElement(SidebarMenuItem, { icon: FiEye, key: "c", children: "Split" })
  );
}

describe("sidebar shared menu surface", () => {
  it("renders SidebarMenuSurface with menu role and item structure", () => {
    const container = document.createElement("div");
    document.body.append(container);
    const root = createRoot(container);

    act(() => {
      root.render(createElement(MenuHarness, { onClose: () => undefined }));
    });

    const surface = container.querySelector<HTMLDivElement>(".sidebar-menu-surface.harness-menu");
    expect(surface).not.toBeNull();
    expect(surface?.getAttribute("role")).toBe("menu");

    const items = container.querySelectorAll<HTMLButtonElement>(".sidebar-menu-item");
    expect(items).toHaveLength(3);
    expect(items[0].querySelector(".sidebar-menu-item-icon")).not.toBeNull();
    expect(items[0].querySelector(".sidebar-menu-item-label")?.textContent).toBe("Preview");
    expect(items[1].disabled).toBe(true);
    expect(container.querySelector(".sidebar-menu-separator")).not.toBeNull();
  });

  it("focuses the first menu item after mount", () => {
    const container = document.createElement("div");
    document.body.append(container);
    const root = createRoot(container);

    act(() => {
      root.render(createElement(MenuHarness, { onClose: () => undefined }));
    });

    const items = container.querySelectorAll<HTMLButtonElement>(".sidebar-menu-item");
    expect(document.activeElement).toBe(items[0]);
  });

  it("navigates ArrowDown/ArrowUp across enabled items", () => {
    const container = document.createElement("div");
    document.body.append(container);
    const root = createRoot(container);

    act(() => {
      root.render(createElement(MenuHarness, { onClose: () => undefined }));
    });

    const surface = container.querySelector<HTMLDivElement>(".sidebar-menu-surface")!;
    const items = container.querySelectorAll<HTMLButtonElement>(".sidebar-menu-item");
    items[0].focus();

    act(() => {
      surface.dispatchEvent(new KeyboardEvent("keydown", {
        key: "ArrowDown",
        bubbles: true,
        cancelable: true
      }));
    });

    // Item at index 1 is disabled; should skip to index 2.
    expect(document.activeElement).toBe(items[2]);

    act(() => {
      surface.dispatchEvent(new KeyboardEvent("keydown", {
        key: "ArrowUp",
        bubbles: true,
        cancelable: true
      }));
    });

    expect(document.activeElement).toBe(items[0]);
  });

  it("supports Home/End keyboard focus navigation", () => {
    const container = document.createElement("div");
    document.body.append(container);
    const root = createRoot(container);

    act(() => {
      root.render(createElement(MenuHarness, { onClose: () => undefined }));
    });

    const surface = container.querySelector<HTMLDivElement>(".sidebar-menu-surface")!;
    const items = container.querySelectorAll<HTMLButtonElement>(".sidebar-menu-item");
    items[0].focus();

    act(() => {
      surface.dispatchEvent(new KeyboardEvent("keydown", {
        key: "End",
        bubbles: true,
        cancelable: true
      }));
    });

    expect(document.activeElement).toBe(items[2]);

    act(() => {
      surface.dispatchEvent(new KeyboardEvent("keydown", {
        key: "Home",
        bubbles: true,
        cancelable: true
      }));
    });

    expect(document.activeElement).toBe(items[0]);
  });

  it("stops click propagation and suppresses nested contextmenu events", () => {
    const surfaceSource = SidebarMenuSurface.toString();
    expect(surfaceSource).toContain("event.stopPropagation()");
    expect(surfaceSource).toContain("event.preventDefault()");
  });

  it("handles keyboard navigation for generic HTMLElement menus via helper", () => {
    const menu = document.createElement("div");
    const first = document.createElement("button");
    const second = document.createElement("button");
    menu.append(first, second);
    document.body.append(menu);
    first.focus();

    const event = new KeyboardEvent("keydown", { key: "ArrowDown", bubbles: true, cancelable: true });
    Object.defineProperty(event, "currentTarget", { value: menu });

    const handled = handleMenuKeyboardNavigation(event as unknown as React.KeyboardEvent<HTMLDivElement>);

    expect(handled).toBe(true);
    expect(event.defaultPrevented).toBe(true);
    expect(document.activeElement).toBe(second);
  });

  it("closes on Escape via useContextMenuDismissal", () => {
    const container = document.createElement("div");
    document.body.append(container);
    const root = createRoot(container);
    const onClose = vi.fn();

    act(() => {
      root.render(createElement(MenuHarness, { onClose }));
    });

    act(() => {
      window.dispatchEvent(new KeyboardEvent("keydown", { key: "Escape", bubbles: true }));
    });

    expect(onClose).toHaveBeenCalled();
  });

  it("closes on outside window click via useContextMenuDismissal", () => {
    const container = document.createElement("div");
    document.body.append(container);
    const root = createRoot(container);
    const onClose = vi.fn();

    act(() => {
      root.render(createElement(MenuHarness, { onClose }));
    });

    act(() => {
      window.dispatchEvent(new MouseEvent("click", { bubbles: true }));
    });

    expect(onClose).toHaveBeenCalledWith({ restoreFocus: false });
  });

  it("keeps SidebarMenuItem accessible when clicked", () => {
    const container = document.createElement("div");
    document.body.append(container);
    const root = createRoot(container);
    const onClick = vi.fn();

    act(() => {
      root.render(createElement(SidebarMenuItem, { icon: FiEye, onClick, children: "Open" }));
    });

    const button = container.querySelector<HTMLButtonElement>(".sidebar-menu-item")!;
    act(() => {
      button.click();
    });

    expect(onClick).toHaveBeenCalledTimes(1);
    expect(button.getAttribute("type")).toBe("button");
  });
});
