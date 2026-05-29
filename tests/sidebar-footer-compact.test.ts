import { readFileSync } from "node:fs";
import { join } from "node:path";
import { createElement } from "react";
import { act } from "react";
import { createRoot } from "react-dom/client";
import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, it, vi } from "vitest";

import type { BrowserController } from "../src/renderer/app/controller/types";
import { SidebarFooter } from "../src/renderer/surfaces/sidebar/components/chrome/SidebarFooter";

const sidebarCss = readFileSync(join(__dirname, "../src/renderer/styles/sidebar.css"), "utf8");

describe("sidebar footer compact controls", () => {
  it("turns the sidebar toggle into a floating-sidebar pin control in compact mode", () => {
    expect(renderFooter({ compactMode: false, floatingSidebarOpen: false })).toContain('aria-label="Focus sidebar"');

    const unpinned = renderFooter({ compactMode: true, floatingSidebarOpen: false });
    expect(unpinned).toContain('aria-label="Pin floating sidebar"');
    expect(unpinned).toContain('aria-pressed="false"');

    const pinned = renderFooter({ compactMode: true, floatingSidebarOpen: true });
    expect(pinned).toContain('aria-label="Unpin floating sidebar"');
    expect(pinned).toContain('aria-pressed="true"');
  });

  it("marks the split button as a tab drop target while dragging another tab", () => {
    const html = renderFooter({
      compactMode: false,
      draggingTabId: "other-tab",
      floatingSidebarOpen: false
    });

    expect(html).toContain('aria-label="Split view"');
    expect(html).toContain('data-drop-target="true"');
  });

  it("labels icon-only footer controls", () => {
    const html = renderFooter({
      compactMode: false,
      floatingSidebarOpen: false
    });

    expect(html).toContain('aria-label="Compact mode"');
    expect(html).toContain('aria-label="History"');
    expect(html).toContain('aria-label="Downloads"');
    expect(html).toContain('aria-label="Settings"');
  });

  it("does not mark the split button as a target for the active tab", () => {
    expect(renderFooter({
      activeTabId: "active-tab",
      compactMode: false,
      draggingTabId: "active-tab",
      floatingSidebarOpen: false
    })).toContain('data-drop-target="false"');
  });

  it("styles the split drop target state", () => {
    expect(sidebarCss).toContain('.sidebar-footer .icon-button[data-drop-target="true"]');
  });

  it("renders compact Memory Saver status and action", () => {
    const html = renderFooter({
      compactMode: false,
      floatingSidebarOpen: false,
      memorySaver: {
        mountedWebviews: 5,
        protectedTabs: 2,
        reclaimableTabs: 3,
        sleepAfterMinutes: 30,
        sleepEnabled: true,
        sleepingTabs: 0,
        summary: "3 releasable · 0 sleeping · 2 protected"
      }
    });

    expect(html).toContain('class="sidebar-memory-saver"');
    expect(html).toContain("3 ready");
    expect(html).toContain("Auto 30m");
    expect(html).toContain('aria-label="Memory Saver, 3 releasable · 0 sleeping · 2 protected"');
    expect(html).not.toContain("disabled");
  });

  it("disables Memory Saver action when no tabs are releasable", () => {
    const html = renderFooter({
      compactMode: false,
      floatingSidebarOpen: false,
      memorySaver: {
        mountedWebviews: 2,
        protectedTabs: 2,
        reclaimableTabs: 0,
        sleepAfterMinutes: 15,
        sleepEnabled: false,
        sleepingTabs: 4,
        summary: "0 releasable · 4 sleeping · 2 protected"
      }
    });

    expect(html).toContain("4 asleep");
    expect(html).toContain("Manual");
    expect(html).toContain("disabled");
  });

  it("styles the Memory Saver footer pill", () => {
    expect(sidebarCss).toContain(".sidebar-memory-saver");
    expect(sidebarCss).toContain(".sidebar-memory-saver:disabled");
  });

  it("moves focus through footer controls with ArrowLeft, ArrowRight, Home, and End", () => {
    const container = document.createElement("div");
    document.body.append(container);
    const root = createRoot(container);

    act(() => {
      root.render(createElement(SidebarFooter, footerProps({
        compactMode: false,
        floatingSidebarOpen: false,
        memorySaver: {
          mountedWebviews: 5,
          protectedTabs: 2,
          reclaimableTabs: 3,
          sleepAfterMinutes: 30,
          sleepEnabled: true,
          sleepingTabs: 0,
          summary: "3 releasable · 0 sleeping · 2 protected"
        }
      })));
    });

    const buttons = container.querySelectorAll<HTMLButtonElement>(".sidebar-footer button");
    buttons[0]?.focus();

    act(() => {
      buttons[0]?.dispatchEvent(new KeyboardEvent("keydown", {
        bubbles: true,
        key: "ArrowRight"
      }));
    });
    expect(document.activeElement).toBe(buttons[1]);

    act(() => {
      document.activeElement?.dispatchEvent(new KeyboardEvent("keydown", {
        bubbles: true,
        key: "End"
      }));
    });
    expect(document.activeElement).toBe(buttons[buttons.length - 1]);
    expect(document.activeElement?.getAttribute("aria-label")).toBe("Settings");

    act(() => {
      document.activeElement?.dispatchEvent(new KeyboardEvent("keydown", {
        bubbles: true,
        key: "ArrowLeft"
      }));
    });
    expect(document.activeElement).toBe(buttons[buttons.length - 2]);

    act(() => {
      document.activeElement?.dispatchEvent(new KeyboardEvent("keydown", {
        bubbles: true,
        key: "Home"
      }));
    });
    expect(document.activeElement).toBe(buttons[0]);

    act(() => root.unmount());
    container.remove();
  });
});

function renderFooter({
  activeTabId = "active-tab",
  compactMode,
  draggingTabId = null,
  floatingSidebarOpen,
  memorySaver = {
    mountedWebviews: 1,
    protectedTabs: 1,
    reclaimableTabs: 0,
    sleepAfterMinutes: 30,
    sleepEnabled: true,
    sleepingTabs: 0,
    summary: "0 releasable · 0 sleeping · 1 protected"
  }
}: {
  activeTabId?: string;
  compactMode: boolean;
  draggingTabId?: string | null;
  floatingSidebarOpen: boolean;
  memorySaver?: Parameters<typeof SidebarFooter>[0]["memorySaver"];
}) {
  return renderToStaticMarkup(createElement(SidebarFooter, footerProps({
    activeTabId,
    compactMode,
    draggingTabId,
    floatingSidebarOpen,
    memorySaver
  })));
}

function footerProps({
  activeTabId = "active-tab",
  compactMode,
  draggingTabId = null,
  floatingSidebarOpen,
  memorySaver
}: {
  activeTabId?: string;
  compactMode: boolean;
  draggingTabId?: string | null;
  floatingSidebarOpen: boolean;
  memorySaver: Parameters<typeof SidebarFooter>[0]["memorySaver"];
}) {
  return {
    actions: {
      openTabInSplit: vi.fn(),
      setSplitLayout: vi.fn(),
      sleepInactiveTabs: vi.fn(),
      toggleCompactMode: vi.fn(),
      toggleSidebar: vi.fn(),
      toggleSplitMode: vi.fn()
    } as unknown as BrowserController["actions"],
    activeTabId,
    compactMode,
    draggingTabId,
    floatingSidebarOpen,
    memorySaver,
    setPanel: vi.fn(),
    setDraggingTabId: vi.fn(),
    splitLayout: "horizontal",
    splitMode: false
  } satisfies Parameters<typeof SidebarFooter>[0];
}
