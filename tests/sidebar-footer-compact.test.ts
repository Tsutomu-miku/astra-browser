import { describe, expect, it, vi } from "vitest";

import { SIDEBAR_TAB_DRAG_TYPE } from "../src/renderer/common/drag-drop/sidebarDragPayload";
import { createClosedTab, createFavorite, createTab } from "../src/renderer/domain/browser";
import {
  acceptSidebarSplitDropTarget,
  getSidebarSplitDropSource,
  resolveSidebarSplitDrop
} from "../src/renderer/surfaces/sidebar/model/sidebarSplitDropTarget";

describe("sidebar split drop helpers", () => {
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
    // Workspace favorites drag with tab identity and resolve through the tab path.
    expect(getSidebarSplitDropSource({ ...state }, (type) => type === SIDEBAR_TAB_DRAG_TYPE ? "favorite-tab" : "")).toEqual({
      tabId: "favorite-tab",
      title: "Design",
      type: "tab"
    });
    // A favorite tab matching the active tab is excluded.
    expect(getSidebarSplitDropSource({ ...state, activeTabId: "favorite-tab" }, (type) => type === SIDEBAR_TAB_DRAG_TYPE ? "favorite-tab" : "")).toBeNull();
    // Tab identity takes priority when both tab and essential data are present.
    expect(getSidebarSplitDropSource({ ...state }, (type) => {
      if (type === SIDEBAR_TAB_DRAG_TYPE) return "other-tab";
      if (type === "text/essential-id") return "essential";
      return "";
    })).toEqual({
      tabId: "other-tab",
      title: "Docs",
      type: "tab"
    });
  });

  it("accepts and resolves split drops through the shared drop target helper", () => {
    const state = splitDropState();
    const dragoverEvent = createSplitDropEvent((type) => type === SIDEBAR_TAB_DRAG_TYPE ? "other-tab" : "");

    expect(acceptSidebarSplitDropTarget(dragoverEvent, state)).toEqual({
      tabId: "other-tab",
      title: "Docs",
      type: "tab"
    });
    expect(dragoverEvent.preventDefault).toHaveBeenCalled();
    expect(dragoverEvent.dataTransfer.dropEffect).toBe("move");

    // Workspace favorite tabs resolve through tab identity.
    const dropEvent = createSplitDropEvent((type) => type === SIDEBAR_TAB_DRAG_TYPE ? "favorite-tab" : "");
    expect(resolveSidebarSplitDrop(dropEvent, state)).toEqual({
      tabId: "favorite-tab",
      title: "Design",
      type: "tab"
    });
    expect(dropEvent.preventDefault).toHaveBeenCalled();
  });
});

function splitDropState() {
  const favoriteTab = { ...createTab("Design", "https://design.example"), id: "favorite-tab", isFavorite: true };
  return {
    activeTabId: "active-tab",
    closedTabs: [createClosedTab("Closed Docs", "https://closed.example", { closedAt: 1 })],
    draggingClosedTabIndex: null,
    draggingEssentialId: null,
    draggingFavoriteId: null,
    draggingTabId: null,
    essentials: [{ ...createFavorite("Inbox", "https://inbox.example"), id: "essential" }],
    favoriteTabs: [favoriteTab],
    tabs: [
      { ...createTab("Active", "https://active.example"), id: "active-tab", isFavorite: false },
      { ...createTab("Docs", "https://docs.example"), id: "other-tab", isFavorite: false },
      favoriteTab
    ]
  };
}

function createSplitDropEvent(getData: (type: string) => string) {
  return {
    dataTransfer: {
      dropEffect: "none",
      getData
    },
    preventDefault: vi.fn()
  };
}
