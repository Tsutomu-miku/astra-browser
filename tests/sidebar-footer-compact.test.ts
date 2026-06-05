import { describe, expect, it, vi } from "vitest";

import { SIDEBAR_TAB_DRAG_TYPE } from "../src/renderer/common/drag-drop/sidebarDragPayload";
import { createFavorite, createTab } from "../src/renderer/domain/browser";
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

    const dropEvent = createSplitDropEvent((type) => type === "text/favorite-id" ? "favorite" : "");
    expect(resolveSidebarSplitDrop(dropEvent, state)).toEqual({
      title: "Design",
      type: "url",
      url: "https://design.example"
    });
    expect(dropEvent.preventDefault).toHaveBeenCalled();
    expect(dropEvent.dataTransfer.dropEffect).toBe("none");
  });
});

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

function createSplitDropEvent(getData: (type: string) => string) {
  return {
    dataTransfer: {
      dropEffect: "none",
      getData
    },
    preventDefault: vi.fn()
  };
}
