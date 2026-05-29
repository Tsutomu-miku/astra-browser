import { describe, expect, it, vi } from "vitest";

import {
  readSidebarTabDragData,
  readSidebarTabDragPayload,
  SIDEBAR_TAB_DRAG_TYPE,
  writeSidebarTabDragPayload
} from "../src/renderer/common/drag-drop/sidebarDragPayload";
import {
  readSidebarClosedTabDragIndex,
  readSidebarFavoriteDragId,
  readSidebarGroupDragId,
  readSidebarTabDragId,
  SIDEBAR_DRAG_DATA
} from "../src/renderer/surfaces/sidebar/model/sidebarDragSources";

describe("sidebar drag payload", () => {
  it("writes and reads the explicit sidebar tab payload before falling back to plain text", () => {
    const data = new Map<string, string>();
    const dataTransfer = {
      effectAllowed: "all",
      getData: vi.fn((type: string) => data.get(type) ?? ""),
      setData: vi.fn((type: string, value: string) => data.set(type, value))
    } as unknown as DataTransfer;

    writeSidebarTabDragPayload(dataTransfer, "tab-1");

    expect(data.get(SIDEBAR_TAB_DRAG_TYPE)).toBe("tab-1");
    expect(data.get("text/plain")).toBe("tab-1");
    expect(readSidebarTabDragPayload(dataTransfer)).toBe("tab-1");
  });

  it("keeps compatibility with plain-text tab drags", () => {
    expect(readSidebarTabDragData((type) => type === "text/plain" ? "tab-legacy" : "")).toBe("tab-legacy");
  });

  it("resolves sidebar drag identities through the shared source model", () => {
    const data = {
      [SIDEBAR_TAB_DRAG_TYPE]: "tab-from-payload",
      [SIDEBAR_DRAG_DATA.closedTabIndex]: "2",
      [SIDEBAR_DRAG_DATA.favoriteId]: "favorite-from-payload",
      [SIDEBAR_DRAG_DATA.groupId]: "group-from-payload"
    };
    const getData = (type: string) => data[type as keyof typeof data] ?? "";

    expect(readSidebarTabDragId({ draggingTabId: "tab-from-state" }, getData)).toBe("tab-from-state");
    expect(readSidebarTabDragId({ draggingTabId: null }, getData)).toBe("tab-from-payload");
    expect(readSidebarFavoriteDragId({ draggingFavoriteId: null }, getData)).toBe("favorite-from-payload");
    expect(readSidebarGroupDragId({ draggingGroupId: null }, getData)).toBe("group-from-payload");
    expect(readSidebarClosedTabDragIndex({ draggingClosedTabIndex: null }, getData)).toBe(2);
  });

  it("treats missing or invalid closed-tab payloads as no source", () => {
    expect(readSidebarClosedTabDragIndex({ draggingClosedTabIndex: null }, () => "")).toBeNull();
    expect(readSidebarClosedTabDragIndex({ draggingClosedTabIndex: null }, () => "abc")).toBeNull();
  });
});
