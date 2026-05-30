import { readFileSync } from "node:fs";
import { join } from "node:path";
import { createElement } from "react";
import { act } from "react";
import { createRoot } from "react-dom/client";
import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, it, vi } from "vitest";

import type { BrowserController } from "../src/renderer/app/controller/types";
import { SIDEBAR_TAB_DRAG_TYPE } from "../src/renderer/common/drag-drop/sidebarDragPayload";
import { createFavorite, createTab } from "../src/renderer/domain/browser";
import { getSidebarSplitDropSource } from "../src/renderer/surfaces/sidebar/model/sidebarSplitDropTarget";
import { SidebarFooter } from "../src/renderer/surfaces/sidebar/components/chrome/SidebarFooter";

const sidebarCss = readFileSync(join(__dirname, "../src/renderer/styles/sidebar.css"), "utf8");

describe("sidebar footer compact controls", () => {
  it("turns the sidebar toggle into a floating-sidebar pin control in compact mode", () => {
    const normal = renderFooter({ compactMode: false, floatingSidebarOpen: false });
    expect(normal).toContain('aria-label="Focus sidebar"');
    expect(normal).not.toContain('title="Focus sidebar"');

    const unpinned = renderFooter({ compactMode: true, floatingSidebarOpen: false });
    expect(unpinned).toContain('aria-label="Pin floating sidebar"');
    expect(unpinned).toContain('aria-pressed="false"');
    expect(unpinned).not.toContain('title="Pin floating sidebar"');

    const pinned = renderFooter({ compactMode: true, floatingSidebarOpen: true });
    expect(pinned).toContain('aria-label="Unpin floating sidebar"');
    expect(pinned).toContain('aria-pressed="true"');
    expect(pinned).not.toContain('title="Unpin floating sidebar"');
  });

  it("marks the split button as a drop target while dragging another tab", () => {
    const html = renderFooter({
      compactMode: false,
      draggingTabId: "other-tab",
      floatingSidebarOpen: false
    });

    expect(html).toContain('aria-label="Split view, drop Docs here"');
    expect(html).toContain('data-drop-target="true"');
  });

  it("marks the split button as a drop target for sidebar URL entries", () => {
    expect(renderFooter({
      compactMode: false,
      draggingEssentialId: "essential",
      floatingSidebarOpen: false
    })).toContain('aria-label="Split view, drop Inbox here"');

    expect(renderFooter({
      compactMode: false,
      draggingFavoriteId: "favorite",
      floatingSidebarOpen: false
    })).toContain('aria-label="Split view, drop Design here"');
    expect(renderFooter({
      compactMode: false,
      draggingFavoriteId: "favorite",
      floatingSidebarOpen: false
    })).toContain('data-drop-target="true"');

    expect(renderFooter({
      compactMode: false,
      draggingClosedTabIndex: 0,
      floatingSidebarOpen: false
    })).toContain('aria-label="Split view, drop Closed Docs here"');
  });

  it("opens dropped sidebar URL entries in split view", () => {
    const actions = createActions();
    const setDraggingEssentialId = vi.fn();
    const setDraggingFavoriteId = vi.fn();
    const setDraggingClosedTabIndex = vi.fn();
    const container = document.createElement("div");
    document.body.append(container);
    const root = createRoot(container);

    act(() => {
      root.render(createElement(SidebarFooter, footerProps({
        actions,
        compactMode: false,
        draggingFavoriteId: "favorite",
        floatingSidebarOpen: false,
        memorySaver: defaultMemorySaver(),
        setDraggingClosedTabIndex,
        setDraggingEssentialId,
        setDraggingFavoriteId
      })));
    });

    const splitButton = container.querySelector<HTMLButtonElement>('[aria-pressed="false"][data-drop-target="true"]')!;
    act(() => {
      splitButton.dispatchEvent(createDragEvent("drop"));
    });

    expect(actions.openUrlInSplit).toHaveBeenCalledWith("https://design.example", "Design");
    expect(setDraggingClosedTabIndex).toHaveBeenCalledWith(null);
    expect(setDraggingEssentialId).toHaveBeenCalledWith(null);
    expect(setDraggingFavoriteId).toHaveBeenCalledWith(null);

    act(() => root.unmount());
    container.remove();
  });

  it("opens dropped tab-backed Favorites in split view by tab id", () => {
    const actions = createActions();
    const setDraggingFavoriteId = vi.fn();
    const favorite = { ...createFavorite("Docs Favorite", "https://docs.example", "other-tab"), id: "favorite" };
    const container = document.createElement("div");
    document.body.append(container);
    const root = createRoot(container);

    act(() => {
      root.render(createElement(SidebarFooter, footerProps({
        actions,
        compactMode: false,
        draggingFavoriteId: "favorite",
        favorites: [favorite],
        floatingSidebarOpen: false,
        memorySaver: defaultMemorySaver(),
        setDraggingFavoriteId
      })));
    });

    const splitButton = container.querySelector<HTMLButtonElement>('button[aria-label^="Split view"]')!;
    act(() => {
      splitButton.dispatchEvent(createDragEvent("drop"));
    });

    expect(actions.openTabInSplit).toHaveBeenCalledWith("other-tab");
    expect(actions.openUrlInSplit).not.toHaveBeenCalled();
    expect(setDraggingFavoriteId).toHaveBeenCalledWith(null);

    act(() => root.unmount());
    container.remove();
  });

  it("resolves split drop sources from drag state or data transfer", () => {
    const state = splitDropState();

    expect(getSidebarSplitDropSource({ ...state, draggingTabId: "other-tab" })).toEqual({
      tabId: "other-tab",
      title: "Docs",
      type: "tab"
    });
    expect(getSidebarSplitDropSource({ ...state, draggingTabId: "active-tab" })).toBeNull();
    expect(getSidebarSplitDropSource({ ...state }, (type) => type === SIDEBAR_TAB_DRAG_TYPE ? "other-tab" : "")).toEqual({
      tabId: "other-tab",
      title: "Docs",
      type: "tab"
    });
    expect(getSidebarSplitDropSource({ ...state }, (type) => type === "text/favorite-id" ? "favorite" : "")).toEqual({
      title: "Design",
      type: "url",
      url: "https://design.example"
    });
    expect(getSidebarSplitDropSource({
      ...state,
      favorites: [{ ...createFavorite("Docs Favorite", "https://docs.example", "other-tab"), id: "favorite" }]
    }, (type) => type === "text/favorite-id" ? "favorite" : "")).toEqual({
      tabId: "other-tab",
      title: "Docs",
      type: "tab"
    });
    expect(getSidebarSplitDropSource({
      ...state,
      favorites: [{ ...createFavorite("Active Favorite", "https://active.example", "active-tab"), id: "favorite" }]
    }, (type) => type === "text/favorite-id" ? "favorite" : "")).toBeNull();
    expect(getSidebarSplitDropSource({ ...state }, (type) => {
      if (type === SIDEBAR_TAB_DRAG_TYPE) return "other-tab";
      if (type === "text/favorite-id") return "favorite";
      return "";
    })).toEqual({
      tabId: "other-tab",
      title: "Docs",
      type: "tab"
    });
  });

  it("labels icon-only footer controls", () => {
    const html = renderFooter({
      compactMode: false,
      floatingSidebarOpen: false,
      splitMode: true
    });

    expect(html).toContain('aria-label="Horizontal split layout"');
    expect(html).toContain('aria-label="Vertical split layout"');
    expect(html).toContain('aria-label="Grid split layout"');
    expect(html).toContain('aria-label="Compact mode"');
    expect(html).toContain('aria-label="History"');
    expect(html).toContain('aria-label="Downloads"');
    expect(html).toContain('aria-label="Settings"');
    expect(html).not.toContain('title="Horizontal split layout"');
    expect(html).not.toContain('title="Compact mode"');
    expect(html).not.toContain('title="Split view"');
    expect(html).not.toContain('title="History"');
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
    expect(html).not.toContain('title="Memory Saver: 3 releasable · 0 sleeping · 2 protected"');
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

    expect(html).toContain("0 ready");
    expect(html).not.toContain("asleep");
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
  draggingClosedTabIndex = null,
  draggingEssentialId = null,
  draggingFavoriteId = null,
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
  },
  splitMode = false
}: {
  activeTabId?: string;
  compactMode: boolean;
  draggingClosedTabIndex?: number | null;
  draggingEssentialId?: string | null;
  draggingFavoriteId?: string | null;
  draggingTabId?: string | null;
  floatingSidebarOpen: boolean;
  memorySaver?: Parameters<typeof SidebarFooter>[0]["memorySaver"];
  splitMode?: boolean;
}) {
  return renderToStaticMarkup(createElement(SidebarFooter, footerProps({
    activeTabId,
    compactMode,
    draggingClosedTabIndex,
    draggingEssentialId,
    draggingFavoriteId,
    draggingTabId,
    floatingSidebarOpen,
    memorySaver,
    splitMode
  })));
}

function footerProps({
  actions = createActions(),
  activeTabId = "active-tab",
  closedTabs = [{ closedAt: 1, title: "Closed Docs", url: "https://closed.example" }],
  compactMode,
  draggingClosedTabIndex = null,
  draggingEssentialId = null,
  draggingFavoriteId = null,
  draggingTabId = null,
  essentials = [{ ...createFavorite("Inbox", "https://inbox.example"), id: "essential" }],
  favorites = [{ ...createFavorite("Design", "https://design.example"), id: "favorite" }],
  floatingSidebarOpen,
  memorySaver,
  setDraggingClosedTabIndex = vi.fn(),
  setDraggingEssentialId = vi.fn(),
  setDraggingFavoriteId = vi.fn(),
  setDraggingTabId = vi.fn(),
  splitMode = false,
  tabs = [
    { ...createTab("Active", "https://active.example"), id: "active-tab" },
    { ...createTab("Docs", "https://docs.example"), id: "other-tab" }
  ]
}: {
  actions?: BrowserController["actions"];
  activeTabId?: string;
  closedTabs?: Parameters<typeof SidebarFooter>[0]["closedTabs"];
  compactMode: boolean;
  draggingClosedTabIndex?: number | null;
  draggingEssentialId?: string | null;
  draggingFavoriteId?: string | null;
  draggingTabId?: string | null;
  essentials?: Parameters<typeof SidebarFooter>[0]["essentials"];
  favorites?: Parameters<typeof SidebarFooter>[0]["favorites"];
  floatingSidebarOpen: boolean;
  memorySaver: Parameters<typeof SidebarFooter>[0]["memorySaver"];
  setDraggingClosedTabIndex?: (closedTabIndex: number | null) => void;
  setDraggingEssentialId?: (essentialId: string | null) => void;
  setDraggingFavoriteId?: (favoriteId: string | null) => void;
  setDraggingTabId?: (tabId: string | null) => void;
  splitMode?: boolean;
  tabs?: Parameters<typeof SidebarFooter>[0]["tabs"];
}) {
  return {
    actions,
    activeTabId,
    closedTabs,
    compactMode,
    draggingClosedTabIndex,
    draggingEssentialId,
    draggingFavoriteId,
    draggingTabId,
    essentials,
    favorites,
    floatingSidebarOpen,
    memorySaver,
    setPanel: vi.fn(),
    setDraggingClosedTabIndex,
    setDraggingEssentialId,
    setDraggingFavoriteId,
    setDraggingTabId,
    splitLayout: "horizontal",
    splitMode,
    tabs
  } satisfies Parameters<typeof SidebarFooter>[0];
}

function createActions() {
  return {
    openTabInSplit: vi.fn(),
    openUrlInSplit: vi.fn(),
    setSplitLayout: vi.fn(),
    sleepInactiveTabs: vi.fn(),
    toggleCompactMode: vi.fn(),
    toggleSidebar: vi.fn(),
    toggleSplitMode: vi.fn()
  } as unknown as BrowserController["actions"];
}

function defaultMemorySaver() {
  return {
    mountedWebviews: 1,
    protectedTabs: 1,
    reclaimableTabs: 0,
    sleepAfterMinutes: 30,
    sleepEnabled: true,
    sleepingTabs: 0,
    summary: "0 releasable · 0 sleeping · 1 protected"
  };
}

function splitDropState() {
  return {
    activeTabId: "active-tab",
    closedTabs: [{ closedAt: 1, title: "Closed Docs", url: "https://closed.example" }],
    draggingClosedTabIndex: null,
    draggingEssentialId: null,
    draggingFavoriteId: null,
    draggingTabId: null,
    essentials: [{ ...createFavorite("Inbox", "https://inbox.example"), id: "essential" }],
    favorites: [{ ...createFavorite("Design", "https://design.example"), id: "favorite" }],
    tabs: [
      { ...createTab("Active", "https://active.example"), id: "active-tab" },
      { ...createTab("Docs", "https://docs.example"), id: "other-tab" }
    ]
  };
}

function createDragEvent(type: string) {
  const event = new Event(type, { bubbles: true, cancelable: true });
  Object.defineProperty(event, "dataTransfer", {
    value: {
      dropEffect: "none",
      effectAllowed: "all",
      getData: vi.fn(() => ""),
      setData: vi.fn()
    }
  });
  return event;
}
