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
