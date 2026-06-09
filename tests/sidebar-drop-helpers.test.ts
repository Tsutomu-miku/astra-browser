import { describe, expect, it, vi } from "vitest";

import { clearDropPlacement } from "../src/renderer/common/drag-drop/dropPlacement";
import {
  SIDEBAR_TAB_DRAG_TYPE,
  readSidebarTabDragData,
  readSidebarTabDragPayload,
  writeSidebarTabDragPayload
} from "../src/renderer/common/drag-drop/sidebarDragPayload";
import {
  acceptSidebarRowReorderDrag,
  clearSidebarRowReorderDrop,
  resolveSidebarRowReorderDrop
} from "../src/renderer/surfaces/sidebar/model/sidebarRowReorderDrop";
import {
  acceptSidebarTabFolderDrag,
  getSidebarTabFolderDragId
} from "../src/renderer/surfaces/sidebar/model/sidebarTabFolderDrop";
import {
  resolveSidebarQuickEntryReorderDrop
} from "../src/renderer/surfaces/sidebar/model/sidebarQuickEntryReorderDrop";
import {
  acceptSidebarSplitDropTarget,
  getSidebarSplitDropSource,
  resolveSidebarSplitDrop
} from "../src/renderer/surfaces/sidebar/model/sidebarSplitDropTarget";
import {
  acceptSidebarTabGroupHeaderDrag,
  resolveSidebarTabGroupHeaderDrop
} from "../src/renderer/surfaces/sidebar/model/sidebarTabGroupHeaderDrop";
import {
  acceptSidebarNewWorkspaceDropTarget,
  getSidebarNewWorkspaceDropIntent,
  getSidebarWorkspaceDropIntent
} from "../src/renderer/surfaces/sidebar/model/sidebarWorkspaceDropIntent";
import {
  hasNewWorkspaceDragSource,
  readSidebarClosedTabDragIndex,
  readSidebarEssentialDragId,
  readSidebarFavoriteDragId,
  readSidebarGroupDragId,
  readSidebarTabDragId,
  readSidebarWorkspaceDragId,
  SIDEBAR_DRAG_DATA
} from "../src/renderer/surfaces/sidebar/model/sidebarDragSources";
import type { BrowserTab, ClosedTab, Favorite } from "../src/renderer/domain/browser";

function buildTransfer(map: Map<string, string>) {
  return {
    effectAllowed: "none",
    dropEffect: "none",
    setData: (type: string, value: string) => map.set(type, value),
    getData: (type: string) => map.get(type) ?? "",
    types: Array.from(map.keys())
  } as unknown as DataTransfer;
}

function tabTransfer(tabId: string) {
  const map = new Map<string, string>();
  const transfer = buildTransfer(map);
  writeSidebarTabDragPayload(transfer, tabId);
  return transfer;
}

describe("sidebar shared drag helpers", () => {
  describe("sidebar tab drag payload", () => {
    it("writes custom and text/plain mime types and reads them back", () => {
      const map = new Map<string, string>();
      writeSidebarTabDragPayload(buildTransfer(map), "tab-42");
      expect(map.get(SIDEBAR_TAB_DRAG_TYPE)).toBe("tab-42");
      expect(map.get("text/plain")).toBe("tab-42");
      expect(readSidebarTabDragPayload(buildTransfer(map))).toBe("tab-42");
      expect(readSidebarTabDragData((type) => map.get(type) ?? "")).toBe("tab-42");
    });

    it("falls back to text/plain when the custom mime type is missing", () => {
      const map = new Map<string, string>([["text/plain", "fallback-tab"]]);
      expect(readSidebarTabDragData((type) => map.get(type) ?? "")).toBe("fallback-tab");
    });
  });

  describe("sidebar drag source readers", () => {
    it("prefers React state over the native payload", () => {
      expect(readSidebarTabDragId({ draggingTabId: "react" }, () => "native")).toBe("react");
      expect(readSidebarGroupDragId({ draggingGroupId: "react" }, () => "native")).toBe("react");
      expect(readSidebarFavoriteDragId({ draggingFavoriteId: "react" }, () => "native")).toBe("react");
      expect(readSidebarEssentialDragId({ draggingEssentialId: "react" }, () => "native")).toBe("react");
      expect(readSidebarWorkspaceDragId({ draggingWorkspaceId: "react" }, () => "native")).toBe("react");
      expect(readSidebarClosedTabDragIndex({ draggingClosedTabIndex: 7 }, () => "0")).toBe(7);
    });

    it("falls back to the native payload when React state has not synced", () => {
      const transfer = tabTransfer("native-tab");
      expect(readSidebarTabDragId({ draggingTabId: null }, (type) => transfer.getData(type))).toBe("native-tab");
      expect(readSidebarGroupDragId({ draggingGroupId: null }, () => "native-group")).toBe("native-group");
      expect(readSidebarFavoriteDragId({ draggingFavoriteId: null }, () => "native-fav")).toBe("native-fav");
      expect(readSidebarEssentialDragId({ draggingEssentialId: null }, () => "native-ess")).toBe("native-ess");
      expect(readSidebarWorkspaceDragId({ draggingWorkspaceId: null }, () => "native-ws")).toBe("native-ws");
      expect(readSidebarClosedTabDragIndex(
        { draggingClosedTabIndex: null },
        () => "3"
      )).toBe(3);
      expect(readSidebarClosedTabDragIndex(
        { draggingClosedTabIndex: null },
        () => ""
      )).toBeNull();
      expect(readSidebarClosedTabDragIndex(
        { draggingClosedTabIndex: null },
        () => "not-a-number"
      )).toBeNull();
    });

    it("uses the documented mime type constants for non-tab payloads", () => {
      expect(SIDEBAR_DRAG_DATA.groupId).toBe("text/group-id");
      expect(SIDEBAR_DRAG_DATA.favoriteId).toBe("text/favorite-id");
      expect(SIDEBAR_DRAG_DATA.essentialId).toBe("text/essential-id");
      expect(SIDEBAR_DRAG_DATA.workspaceId).toBe("text/workspace-id");
      expect(SIDEBAR_DRAG_DATA.closedTabIndex).toBe("text/closed-tab-index");
    });

    it("detects any New Space drag source across tab/group/favorite/closed-tab state", () => {
      expect(hasNewWorkspaceDragSource({ draggingTabId: "tab" })).toBe(true);
      expect(hasNewWorkspaceDragSource({ draggingGroupId: "group" })).toBe(true);
      expect(hasNewWorkspaceDragSource({ draggingFavoriteId: "fav" })).toBe(true);
      expect(hasNewWorkspaceDragSource({ draggingClosedTabIndex: 0 })).toBe(true);
      expect(hasNewWorkspaceDragSource({ draggingWorkspaceId: "ws" })).toBe(false);
      expect(hasNewWorkspaceDragSource({})).toBe(false);
    });
  });

  describe("sidebar row reorder helper", () => {
    it("accepts a valid drag and writes a before/after drop placement", () => {
      const target = document.createElement("div");
      const event = {
        currentTarget: target,
        clientY: 1,
        dataTransfer: { dropEffect: "none" } as unknown as DataTransfer,
        preventDefault: vi.fn()
      } as unknown as React.DragEvent<HTMLDivElement>;

      const result = acceptSidebarRowReorderDrag(event, {
        readDragId: () => "tab-a",
        targetId: "tab-b"
      });

      expect(result).toBe("tab-a");
      expect(event.preventDefault).toHaveBeenCalled();
      expect(event.dataTransfer.dropEffect).toBe("move");
      expect(target.getAttribute("data-drop-placement")).toMatch(/before|after/);

      clearDropPlacement(target);
      expect(target.hasAttribute("data-drop-placement")).toBe(false);
    });

    it("rejects same-id and missing-id drags without preventing default", () => {
      const target = document.createElement("div");
      const makeEvent = () => ({
        currentTarget: target,
        clientY: 0,
        dataTransfer: { dropEffect: "none" } as unknown as DataTransfer,
        preventDefault: vi.fn()
      } as unknown as React.DragEvent<HTMLDivElement>);

      expect(acceptSidebarRowReorderDrag(makeEvent(), {
        readDragId: () => "tab-a",
        targetId: "tab-a"
      })).toBeNull();

      expect(acceptSidebarRowReorderDrag(makeEvent(), {
        readDragId: () => null,
        targetId: "tab-b"
      })).toBeNull();

      expect(makeEvent().preventDefault).not.toHaveBeenCalled();
    });

    it("resolves the drop id and clears the drop placement attribute", () => {
      const target = document.createElement("div");
      target.setAttribute("data-drop-placement", "before");
      const event = {
        currentTarget: target,
        dataTransfer: { dropEffect: "move" } as unknown as DataTransfer
      } as unknown as React.DragEvent<HTMLDivElement>;

      expect(resolveSidebarRowReorderDrop(event, {
        readDragId: () => "tab-a",
        targetId: "tab-b"
      })).toBe("tab-a");

      expect(resolveSidebarRowReorderDrop(event, {
        readDragId: () => "tab-a",
        targetId: "tab-a"
      })).toBeNull();

      expect(target.hasAttribute("data-drop-placement")).toBe(false);
    });

    it("clears drop placement on drag leave", () => {
      const target = document.createElement("div");
      target.setAttribute("data-drop-placement", "after");
      const event = {
        currentTarget: target,
        dataTransfer: {} as unknown as DataTransfer
      } as unknown as React.DragEvent<HTMLDivElement>;

      clearSidebarRowReorderDrop(event);
      expect(target.hasAttribute("data-drop-placement")).toBe(false);
    });
  });

  describe("sidebar tab folder drop helper", () => {
    const makeFolderEvent = (transfer: DataTransfer) => {
      const target = document.createElement("section");
      return {
        currentTarget: target,
        dataTransfer: transfer,
        preventDefault: vi.fn()
      } as unknown as React.DragEvent<HTMLElement>;
    };

    it("reads the dragged tab id from React state first, then the native payload", () => {
      const native = tabTransfer("native-tab");
      expect(getSidebarTabFolderDragId(
        makeFolderEvent(native),
        null
      )).toBe("native-tab");

      expect(getSidebarTabFolderDragId(
        makeFolderEvent(native),
        "react-tab"
      )).toBe("react-tab");
    });

    it("accepts a tab folder drop and sets the requested drop effect", () => {
      const event = makeFolderEvent(tabTransfer("tab-a"));

      expect(acceptSidebarTabFolderDrag(event, null, "copy")).toBe(true);
      expect(event.preventDefault).toHaveBeenCalled();
      expect(event.dataTransfer.dropEffect).toBe("copy");
      expect(event.currentTarget.dataset.dropActive).toBe("true");
    });

    it("rejects a folder drop when no tab id is available", () => {
      const empty = buildTransfer(new Map());
      const event = makeFolderEvent(empty);
      expect(acceptSidebarTabFolderDrag(event, null)).toBe(false);
      expect(event.preventDefault).not.toHaveBeenCalled();
      expect(event.currentTarget.hasAttribute("data-drop-active")).toBe(false);
    });
  });

  describe("sidebar quick entry reorder helper", () => {
    it("resolves valid favorite and essential reorder drops", () => {
      const favorite = buildTransfer(new Map([[SIDEBAR_DRAG_DATA.favoriteId, "fav-a"]]));
      const essential = buildTransfer(new Map([[SIDEBAR_DRAG_DATA.essentialId, "ess-a"]]));
      const target = document.createElement("div");

      const makeEvent = (transfer: DataTransfer) => ({
        currentTarget: target,
        clientY: 0,
        dataTransfer: transfer,
        preventDefault: vi.fn(),
        stopPropagation: vi.fn()
      } as unknown as React.DragEvent<HTMLElement>);

      const favoriteDrop = resolveSidebarQuickEntryReorderDrop(
        makeEvent(favorite),
        { draggingEssentialId: null, draggingFavoriteId: null },
        { kind: "favorite", targetQuickEntryId: "fav-b" }
      );
      expect(favoriteDrop).toEqual({
        placement: expect.stringMatching(/before|after/),
        quickEntryId: "fav-a",
        targetQuickEntryId: "fav-b"
      });

      const essentialDrop = resolveSidebarQuickEntryReorderDrop(
        makeEvent(essential),
        { draggingEssentialId: null, draggingFavoriteId: null },
        { kind: "essential", targetQuickEntryId: "ess-b" }
      );
      expect(essentialDrop).toEqual({
        placement: expect.stringMatching(/before|after/),
        quickEntryId: "ess-a",
        targetQuickEntryId: "ess-b"
      });
    });

    it("keeps same-id quick entry drops as a no-op", () => {
      const favorite = buildTransfer(new Map([[SIDEBAR_DRAG_DATA.favoriteId, "fav-a"]]));
      const target = document.createElement("div");
      const event = {
        currentTarget: target,
        clientY: 0,
        dataTransfer: favorite,
        preventDefault: vi.fn(),
        stopPropagation: vi.fn()
      } as unknown as React.DragEvent<HTMLElement>;

      expect(resolveSidebarQuickEntryReorderDrop(
        event,
        { draggingEssentialId: null, draggingFavoriteId: "fav-a" },
        { kind: "favorite", targetQuickEntryId: "fav-a" }
      )).toBeNull();
    });
  });

  describe("sidebar tab group header drop helper", () => {
    it("accepts tab-into-group folder drops", () => {
      const transfer = tabTransfer("tab-a");
      const target = document.createElement("div");
      const event = {
        currentTarget: target,
        clientY: 0,
        dataTransfer: transfer,
        preventDefault: vi.fn()
      } as unknown as React.DragEvent<HTMLElement>;

      expect(acceptSidebarTabGroupHeaderDrag(
        event,
        { draggingTabId: null, draggingGroupId: null },
        "group-1"
      )).toEqual({ type: "tab", tabId: "tab-a" });
    });

    it("accepts group reorder drops and ignores the current group", () => {
      const transfer = buildTransfer(new Map([[SIDEBAR_DRAG_DATA.groupId, "group-2"]]));
      const target = document.createElement("div");
      const acceptEvent = {
        currentTarget: target,
        clientY: 0,
        dataTransfer: transfer,
        preventDefault: vi.fn()
      } as unknown as React.DragEvent<HTMLElement>;

      expect(acceptSidebarTabGroupHeaderDrag(
        acceptEvent,
        { draggingTabId: null, draggingGroupId: null },
        "group-1"
      )).toEqual({ type: "group", groupId: "group-2" });

      const resolveEvent = {
        currentTarget: target,
        dataTransfer: buildTransfer(new Map([[SIDEBAR_DRAG_DATA.groupId, "group-1"]])),
        preventDefault: vi.fn()
      } as unknown as React.DragEvent<HTMLElement>;

      expect(resolveSidebarTabGroupHeaderDrop(
        resolveEvent,
        { draggingTabId: null, draggingGroupId: null },
        "group-1"
      )).toEqual({ type: "currentGroup", groupId: "group-1" });
    });

    it("resolves tab-into-group drops", () => {
      const transfer = tabTransfer("tab-a");
      const target = document.createElement("div");
      const event = {
        currentTarget: target,
        dataTransfer: transfer,
        preventDefault: vi.fn()
      } as unknown as React.DragEvent<HTMLElement>;

      expect(resolveSidebarTabGroupHeaderDrop(
        event,
        { draggingTabId: null, draggingGroupId: null },
        "group-1"
      )).toEqual({ type: "tab", tabId: "tab-a" });
    });
  });

  describe("sidebar workspace drop intent helper", () => {
    it("resolves tab/group/closed-tab intents for cross-workspace drops", () => {
      const stateBase = {
        activeWorkspaceId: "ws-a",
        draggingTabId: null,
        draggingGroupId: null,
        draggingClosedTabIndex: null,
        draggingWorkspaceId: null
      };

      expect(getSidebarWorkspaceDropIntent(
        { ...stateBase, targetWorkspaceId: "ws-b" },
        (type) => tabTransfer("tab-a").getData(type)
      )).toEqual({ type: "tab", tabId: "tab-a" });

      expect(getSidebarWorkspaceDropIntent(
        { ...stateBase, targetWorkspaceId: "ws-b" },
        (type) => (type === SIDEBAR_DRAG_DATA.groupId ? "group-1" : "")
      )).toEqual({ type: "group", groupId: "group-1" });

      // Workspace favorites drag with tab identity.
      expect(getSidebarWorkspaceDropIntent(
        { ...stateBase, targetWorkspaceId: "ws-b" },
        (type) => tabTransfer("fav-tab").getData(type)
      )).toEqual({ type: "tab", tabId: "fav-tab" });

      expect(getSidebarWorkspaceDropIntent(
        { ...stateBase, targetWorkspaceId: "ws-b" },
        (type) => type === SIDEBAR_DRAG_DATA.closedTabIndex ? "2" : ""
      )).toEqual({ type: "closedTab", closedTabIndex: 2 });
    });

    it("returns null for same-workspace tab/group drops", () => {
      expect(getSidebarWorkspaceDropIntent({
        activeWorkspaceId: "ws-a",
        targetWorkspaceId: "ws-a",
        draggingTabId: "tab-a",
        draggingGroupId: null,
        draggingClosedTabIndex: null,
        draggingWorkspaceId: null
      })).toBeNull();
    });

    it("resolves workspace reorder drops", () => {
      expect(getSidebarWorkspaceDropIntent({
        activeWorkspaceId: "ws-a",
        targetWorkspaceId: "ws-b",
        draggingTabId: null,
        draggingGroupId: null,
        draggingClosedTabIndex: null,
        draggingWorkspaceId: "ws-c"
      })).toEqual({ type: "workspace", workspaceId: "ws-c" });
    });

    it("resolves New Space drop intents and dragover acceptance", () => {
      expect(getSidebarNewWorkspaceDropIntent({
        draggingTabId: null,
        draggingGroupId: null,
        draggingClosedTabIndex: null
      }, (type) => tabTransfer("tab-a").getData(type))).toEqual({ type: "tab", tabId: "tab-a" });

      expect(getSidebarNewWorkspaceDropIntent({
        draggingTabId: null,
        draggingGroupId: "group-1",
        draggingClosedTabIndex: null
      })).toEqual({ type: "group", groupId: "group-1" });

      // Workspace favorites drag with tab identity.
      expect(getSidebarNewWorkspaceDropIntent({
        draggingTabId: "fav-tab",
        draggingGroupId: null,
        draggingClosedTabIndex: null
      })).toEqual({ type: "tab", tabId: "fav-tab" });

      expect(getSidebarNewWorkspaceDropIntent({
        draggingTabId: null,
        draggingGroupId: null,
        draggingClosedTabIndex: 0
      })).toEqual({ type: "closedTab", closedTabIndex: 0 });

      const acceptEvent = {
        dataTransfer: tabTransfer("tab-a"),
        preventDefault: vi.fn()
      };
      expect(acceptSidebarNewWorkspaceDropTarget(acceptEvent, {
        draggingTabId: null,
        draggingGroupId: null,
        draggingClosedTabIndex: null
      })).not.toBeNull();
      expect(acceptEvent.preventDefault).toHaveBeenCalled();
      expect(acceptEvent.dataTransfer.dropEffect).toBe("move");

      const emptyEvent = {
        dataTransfer: buildTransfer(new Map()),
        preventDefault: vi.fn()
      };
      expect(acceptSidebarNewWorkspaceDropTarget(emptyEvent, {
        draggingTabId: null,
        draggingGroupId: null,
        draggingClosedTabIndex: null
      })).toBeNull();
      expect(emptyEvent.preventDefault).not.toHaveBeenCalled();
    });
  });

  describe("sidebar split drop target helper", () => {
    function buildSplitState(overrides: Partial<{
      activeTabId: string;
      closedTabs: ClosedTab[];
      essentials: Favorite[];
      favoriteTabs: BrowserTab[];
      tabs: BrowserTab[];
    }> = {}) {
      return {
        activeTabId: overrides.activeTabId ?? "active-tab",
        closedTabs: overrides.closedTabs ?? [],
        draggingClosedTabIndex: null,
        draggingEssentialId: null,
        draggingFavoriteId: null,
        draggingTabId: null,
        essentials: overrides.essentials ?? [],
        favoriteTabs: overrides.favoriteTabs ?? [
          { id: "fav-tab", url: "https://fav.test", title: "Favorite Tab", isFavorite: true } as BrowserTab
        ],
        tabs: overrides.tabs ?? [
          { id: "tab-a", url: "https://a.test", title: "Tab A", isFavorite: false } as BrowserTab,
          { id: "active-tab", url: "https://active.test", title: "Active", isFavorite: false } as BrowserTab
        ] as BrowserTab[]
      };
    }

    it("resolves non-active tabs by identity before falling back to URL entries", () => {
      const source = getSidebarSplitDropSource(buildSplitState(), (type) => tabTransfer("tab-a").getData(type));
      expect(source).toEqual({ type: "tab", tabId: "tab-a", title: "Tab A" });
    });

    it("ignores the active tab when resolving a split drop from its tab id", () => {
      const activeTabPayload = buildSplitState();
      const source = getSidebarSplitDropSource(activeTabPayload, (type) => tabTransfer("active-tab").getData(type));
      expect(source).toBeNull();
    });

    it("resolves essentials and closed tabs through URL sources, workspace favorites through tab identity", () => {
      const essentialState = buildSplitState({
        essentials: [{ id: "ess-1", url: "https://ess.test", title: "Essential" } as Favorite]
      });
      expect(getSidebarSplitDropSource(essentialState, (type) => type === SIDEBAR_DRAG_DATA.essentialId ? "ess-1" : "")).toEqual({
        type: "url",
        title: "Essential",
        url: "https://ess.test"
      });

      // Workspace favorites now drag with tab identity.
      const favoriteTabState = buildSplitState({
        tabs: [
          { id: "tab-a", url: "https://a.test", title: "Tab A", isFavorite: false } as BrowserTab,
          { id: "fav-1", url: "https://fav.test", title: "Favorite", isFavorite: true } as BrowserTab,
          { id: "active-tab", url: "https://active.test", title: "Active", isFavorite: false } as BrowserTab
        ]
      });
      expect(getSidebarSplitDropSource(favoriteTabState, (type) => tabTransfer("fav-1").getData(type))).toEqual({
        type: "tab",
        tabId: "fav-1",
        title: "Favorite"
      });

      const closedState = buildSplitState({
        closedTabs: [{ url: "https://closed.test", title: "Closed" } as ClosedTab]
      });
      expect(getSidebarSplitDropSource(closedState, (type) => type === SIDEBAR_DRAG_DATA.closedTabIndex ? "0" : "")).toEqual({
        type: "url",
        title: "Closed",
        url: "https://closed.test"
      });
    });

    it("accepts and resolves split drop events", () => {
      const state = buildSplitState();
      const acceptEvent = {
        dataTransfer: tabTransfer("tab-a"),
        preventDefault: vi.fn()
      };
      expect(acceptSidebarSplitDropTarget(acceptEvent, state)).not.toBeNull();
      expect(acceptEvent.preventDefault).toHaveBeenCalled();
      expect(acceptEvent.dataTransfer.dropEffect).toBe("move");

      const resolveEvent = {
        dataTransfer: tabTransfer("tab-a"),
        preventDefault: vi.fn()
      };
      expect(resolveSidebarSplitDrop(resolveEvent, state)).toEqual({
        type: "tab",
        tabId: "tab-a",
        title: "Tab A"
      });
    });

    it("returns null when no source matches", () => {
      const state = buildSplitState();
      expect(getSidebarSplitDropSource(state, () => "")).toBeNull();
    });
  });
});
