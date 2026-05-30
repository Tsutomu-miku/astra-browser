import { createElement } from "react";
import { act } from "react";
import { createRoot } from "react-dom/client";
import { describe, expect, it, vi } from "vitest";

import type { BrowserController } from "../src/renderer/app/controller/types";
import { SIDEBAR_TAB_DRAG_TYPE } from "../src/renderer/common/drag-drop/sidebarDragPayload";
import { createDefaultState, createFavorite, createTab, type BrowserState, type Workspace } from "../src/renderer/domain/browser";
import { useSidebarQuickEntryDrag } from "../src/renderer/surfaces/sidebar/hooks/useSidebarQuickEntryDrag";
import { FavoriteButton } from "../src/renderer/surfaces/sidebar/components/tabs/SidebarItems";

describe("useSidebarQuickEntryDrag", () => {
  it("keeps legacy Favorite drags on the quick-entry payload", () => {
    const tab = createTab("Docs", "https://docs.example");
    const favorite = createFavorite("Docs", tab.url);
    const workspace = createWorkspace(tab, favorite);
    const data = new Map<string, string>();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(FavoriteDragHarness, {
        activeWorkspace: workspace,
        favorite,
        state: createDefaultState()
      }));
    });

    const event = new Event("dragstart", { bubbles: true });
    const dataTransfer = {
      effectAllowed: "",
      getData: (type: string) => data.get(type) ?? "",
      setData: (type: string, value: string) => data.set(type, value)
    };
    Object.defineProperty(event, "dataTransfer", { value: dataTransfer });
    container.querySelector(".favorite-button")?.dispatchEvent(event);

    expect(dataTransfer.effectAllowed).toBe("move");
    expect(data.get("text/favorite-id")).toBe(favorite.id);
    expect(data.get(SIDEBAR_TAB_DRAG_TYPE)).toBeUndefined();
    expect(data.get("text/plain")).toBeUndefined();

    act(() => root.unmount());
  });

  it("reorders payload-backed Favorites through the shared quick-entry drop resolver", () => {
    const first = createFavorite("First", "https://first.example");
    const second = createFavorite("Second", "https://second.example");
    const tab = createTab("Docs", "https://docs.example");
    const actions = createActions();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(FavoriteReorderHarness, {
        actions,
        activeWorkspace: createWorkspace(tab, second),
        favorite: second,
        state: createDefaultState()
      }));
    });

    const button = container.querySelector<HTMLElement>(".favorite-button")!;
    Object.defineProperty(button, "getBoundingClientRect", {
      value: () => ({
        bottom: 40,
        height: 40,
        left: 0,
        right: 240,
        top: 0,
        width: 240,
        x: 0,
        y: 0,
        toJSON: () => undefined
      })
    });

    const dragData = { "text/favorite-id": first.id };
    button.dispatchEvent(createDragEvent("dragover", { clientY: 36 }, dragData));
    button.dispatchEvent(createDragEvent("drop", { clientY: 36 }, dragData));

    expect(actions.reorderWorkspaceFavorite).toHaveBeenCalledWith(first.id, second.id, "after");

    act(() => root.unmount());
  });
});

function FavoriteDragHarness({
  activeWorkspace,
  favorite,
  state
}: {
  activeWorkspace: Workspace;
  favorite: Workspace["favorites"][number];
  state: BrowserState;
}) {
  const drag = useSidebarQuickEntryDrag({
    actions: createActions(),
    activeWorkspace,
    draggingTabId: null,
    setDraggingTabId: vi.fn(),
    state
  });

  return createElement(FavoriteButton, {
    draggable: true,
    draggingQuickEntryId: drag.draggingFavoriteId,
    favorite,
    onDragEnd: () => drag.setDraggingFavoriteId(null),
    onDragStart: drag.handleFavoriteDragStart,
    onOpen: vi.fn(),
    onOpenInSplit: vi.fn(),
    onPreview: vi.fn()
  });
}

function FavoriteReorderHarness({
  actions,
  activeWorkspace,
  favorite,
  state
}: {
  actions: BrowserController["actions"];
  activeWorkspace: Workspace;
  favorite: Workspace["favorites"][number];
  state: BrowserState;
}) {
  const drag = useSidebarQuickEntryDrag({
    actions,
    activeWorkspace,
    draggingTabId: null,
    setDraggingTabId: vi.fn(),
    state
  });

  return createElement(FavoriteButton, {
    draggable: true,
    draggingQuickEntryId: null,
    favorite,
    kind: "favorite",
    onDragEnd: () => drag.setDraggingFavoriteId(null),
    onDragStart: drag.handleFavoriteDragStart,
    onDrop: drag.handleFavoriteReorderDrop,
    onOpen: vi.fn(),
    onOpenInSplit: vi.fn(),
    onPreview: vi.fn()
  });
}

function createWorkspace(tab: Workspace["tabs"][number], favorite: Workspace["favorites"][number]): Workspace {
  return {
    ...createDefaultState().workspaces[0],
    activeTabId: tab.id,
    favorites: [favorite],
    tabs: [tab]
  };
}

function createActions() {
  return {
    addTabToFavorites: vi.fn(),
    moveTabToFolderEnd: vi.fn(),
    reorderEssential: vi.fn(),
    reorderWorkspaceFavorite: vi.fn(),
    toggleTabEssential: vi.fn()
  } as unknown as BrowserController["actions"];
}

function createDragEvent(
  type: string,
  pointer: { clientX?: number; clientY?: number } = {},
  dragData: Record<string, string> = {}
) {
  const event = new Event(type, { bubbles: true, cancelable: true });
  Object.defineProperty(event, "clientX", { value: pointer.clientX ?? 0 });
  Object.defineProperty(event, "clientY", { value: pointer.clientY ?? 0 });
  Object.defineProperty(event, "dataTransfer", {
    value: {
      dropEffect: "none",
      effectAllowed: "all",
      getData: vi.fn((type: string) => dragData[type] ?? ""),
      setData: vi.fn((type: string, value: string) => {
        dragData[type] = value;
      })
    }
  });
  return event;
}
