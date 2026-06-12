import { createElement } from "react";
import { act } from "react";
import { createRoot } from "react-dom/client";
import { describe, expect, it, vi } from "vitest";

import {
  createDefaultState,
  createTab
} from "../src/renderer/domain/browser";
import type { BrowserTab } from "../src/renderer/domain/browser";
import { getActiveWorkspace } from "../src/renderer/domain/browser/selectors";
import type { BrowserController } from "../src/renderer/app/controller/types";
import { SidebarSections } from "../src/renderer/surfaces/sidebar/components/tabs/SidebarSections";
import { buildOmniboxSuggestions } from "../src/renderer/common/omnibox/omniboxSuggestions";
import { getNumberShortcutTabs } from "../src/renderer/common/shortcuts/numberShortcutTargets";
import { isSidebarFavoriteActive } from "../src/renderer/surfaces/sidebar/model/sidebarItemState";
import { filterSidebarItems, getSidebarSearchTargets } from "../src/renderer/surfaces/sidebar/sidebarFiltering";
import { buildContentCommands } from "../src/renderer/surfaces/command/model/commandContentEntries";

describe("sidebar favorites with duplicate URLs across tabs", () => {
  function makeSameUrlTabs() {
    // Two tabs with the same URL but different ids — legitimate per user req.
    // Both are favorites (new model: tab.isFavorite = true, ordered via favoriteOrder).
    const tabA = { ...createTab("Tab A", "https://same.example"), id: "tab-a", isFavorite: true };
    const tabB = { ...createTab("Tab B", "https://same.example"), id: "tab-b", isFavorite: true };
    const tabC = { ...createTab("Tab C", "https://other.example"), id: "tab-c", isFavorite: false };
    return { tabA, tabB, tabC };
  }

  function createActions() {
    return {
      assignTabToGroup: vi.fn(),
      closeTab: vi.fn(),
      groupTab: vi.fn(),
      navigateActiveTab: vi.fn(),
      openGlance: vi.fn(),
      openTabInSplit: vi.fn(),
      openUrlInActiveWorkspace: vi.fn(),
      openUrlInSplit: vi.fn(),
      restoreClosedTab: vi.fn(),
      selectTab: vi.fn(),
      toggleTabGroupCollapsed: vi.fn(),
      updateTabGroup: vi.fn()
    } as unknown as BrowserController["actions"];
  }

  it("resolves two favorite tabs with identical backing-tab URLs to distinct tabs", () => {
    const { tabA, tabB, tabC } = makeSameUrlTabs();
    const actions = createActions();
    const container = document.createElement("div");
    const root = createRoot(container);

    act(() => {
      root.render(createElement(SidebarSections, {
        actions,
        activeTab: tabC,
        draggingEssentialId: null,
        draggingFavoriteId: null,
        draggingGroupId: null,
        draggingTabId: null,
        filteredItems: {
          essentials: [],
          favorites: [{ kind: "tab", tab: tabA }, { kind: "tab", tab: tabB }],
          groupedTabs: [],
          hasMatches: true,
          isFiltering: false,
          pinnedTabs: [],
          regularTabs: []
        },
        onEssentialDragStart: vi.fn(),
        onEssentialDrop: vi.fn(),
        onEssentialReorderDrop: vi.fn(),
        onFavoriteDragStart: vi.fn(),
        onFavoriteDrop: vi.fn(),
        onFavoriteReorderDrop: vi.fn(),
        onFavoriteTabDrop: vi.fn(),
        onTabGroupContextMenu: vi.fn(),
        onQuickEntryContextMenu: vi.fn(),
        onTabContextMenu: vi.fn(),
        onTabDrop: vi.fn(),
        onTabsDrop: vi.fn(),
        onRenameTab: vi.fn(),
        onToggleSection: vi.fn(),
        setDraggingEssentialId: vi.fn(),
        setDraggingFavoriteId: vi.fn(),
        setDraggingGroupId: vi.fn(),
        setDraggingTabId: vi.fn(),
        splitTabs: [],
        activeSplitId: null,
        workspaceTabs: [tabA, tabB, tabC]
      }));
    });

    const buttons = container.querySelectorAll(".favorites .tab-button");
    expect(buttons).toHaveLength(2);
    expect(buttons[0]?.getAttribute("aria-label")).toContain("Tab A");
    expect(buttons[1]?.getAttribute("aria-label")).toContain("Tab B");

    act(() => {
      buttons[0]?.dispatchEvent(new MouseEvent("click", { bubbles: true }));
    });
    expect(actions.selectTab).toHaveBeenLastCalledWith("tab-a");

    act(() => {
      buttons[1]?.dispatchEvent(new MouseEvent("click", { bubbles: true }));
    });
    expect(actions.selectTab).toHaveBeenLastCalledWith("tab-b");
    expect(actions.selectTab).toHaveBeenCalledTimes(2);
    expect(actions.navigateActiveTab).not.toHaveBeenCalled();

    act(() => root.unmount());
  });

  it("highlights active state for the correct favorite tab when URLs collide", () => {
    const { tabA, tabB } = makeSameUrlTabs();

    // tabA is active → tabA should be active, tabB must NOT be.
    expect(isSidebarFavoriteActive(tabA, tabA)).toBe(true);
    expect(isSidebarFavoriteActive(tabA, tabB)).toBe(false);

    // tabB is active → exactly the reverse.
    expect(isSidebarFavoriteActive(tabB, tabA)).toBe(false);
    expect(isSidebarFavoriteActive(tabB, tabB)).toBe(true);
  });

  it("never treats a non-favorite tab as the active favorite even when URLs match", () => {
    const { tabA, tabC } = makeSameUrlTabs();
    // tabC has a different URL in this fixture, but the point is:
    // isSidebarFavoriteActive compares by tab id, never by URL.
    const sameUrlNonFav: BrowserTab = { ...createTab("Same URL non-fav", tabA.url), id: "non-fav", isFavorite: false };
    expect(isSidebarFavoriteActive(tabA, sameUrlNonFav)).toBe(false);
    expect(isSidebarFavoriteActive(sameUrlNonFav, tabA)).toBe(false);
  });

  it("orders number-shortcut tabs by explicit tabId via favoriteOrder, never by URL match", () => {
    const { tabA, tabB, tabC } = makeSameUrlTabs();
    // Only tabB is a favorite. tabA has the same URL but is NOT a favorite —
    // it must remain in the regular tab order.
    tabA.isFavorite = false; // ensure tabA is not a favorite
    const workspace = {
      tabGroups: [],
      tabs: [tabC, tabA, tabB],
      favoriteOrder: [tabB.id]
    };

    const ordered = getNumberShortcutTabs(workspace).map((t) => t.id);
    // tabB must come first because it is in favoriteOrder and isFavorite.
    // tabA must NOT be promoted into the favorites slot just because it
    // shares tabB's URL.
    expect(ordered[0]).toBe("tab-b");
    // Explicit assertion: tabA appears AFTER tabB.
    expect(ordered.indexOf("tab-a")).toBeGreaterThan(ordered.indexOf("tab-b"));
  });

  it("keeps two favorite tabs with duplicate URLs distinct in omnibox suggestions", () => {
    const { tabA, tabB } = makeSameUrlTabs();
    const state = createDefaultState();
    const workspace = getActiveWorkspace(state);
    // Replace the default tabs with our two same-url test tabs.
    workspace.tabs = [tabA, tabB];
    workspace.activeTabId = tabA.id;
    workspace.favoriteOrder = [tabA.id, tabB.id];

    const suggestions = buildOmniboxSuggestions(state, "same");
    const favSuggestions = suggestions.filter((s) => s.type === "favorite");
    expect(favSuggestions).toHaveLength(2);

    const titles = favSuggestions.map((s) => s.title).sort();
    expect(titles).toEqual(["Tab A", "Tab B"]);

    // Each suggestion must carry its own backing tab id, not bleed across.
    const byTitle = Object.fromEntries(favSuggestions.map((s) => [s.title, s]));
    expect(byTitle["Tab A"].tabId).toBe("tab-a");
    expect(byTitle["Tab B"].tabId).toBe("tab-b");
  });

  it("keeps two favorite tabs with duplicate URLs distinct in command palette", () => {
    const { tabA, tabB, tabC } = makeSameUrlTabs();
    const state = createDefaultState();
    const workspace = { ...getActiveWorkspace(state), tabs: [tabA, tabB, tabC], favoriteOrder: [tabA.id, tabB.id] };
    const actions = createActions();

    const commands = buildContentCommands(state, workspace, actions);
    const favCommands = commands.filter((c) => c.subtitle.startsWith("Favorite tab"));
    expect(favCommands).toHaveLength(2);
    expect(favCommands.map((c) => c.title).sort()).toEqual(["Tab A", "Tab B"]);

    // Run each one and confirm it selects the correct tab id.
    favCommands.find((c) => c.title === "Tab A")?.run();
    expect(actions.selectTab).toHaveBeenLastCalledWith("tab-a");

    favCommands.find((c) => c.title === "Tab B")?.run();
    expect(actions.selectTab).toHaveBeenLastCalledWith("tab-b");

    expect(actions.selectTab).toHaveBeenCalledTimes(2);
    expect(actions.openUrlInActiveWorkspace).not.toHaveBeenCalled();
  });

  it("keeps two favorite tabs with duplicate URLs distinct in sidebar filtering and search targets", () => {
    const { tabA, tabB } = makeSameUrlTabs();
    const filtered = filterSidebarItems({
      essentials: [],
      favorites: [{ kind: "tab", tab: tabA }, { kind: "tab", tab: tabB }],
      groupedTabs: [],
      pinnedTabs: [],
      regularTabs: [],
      workspaceTabs: [tabA, tabB]
    }, "");

    expect(filtered.favorites).toHaveLength(2);

    // Search targets are the fully-hydrated view model consumed by keyboard
    // activation; each favorite tab's metadata must reflect its OWN backing
    // tab, never the URL-duplicate sibling.
    const targets = getSidebarSearchTargets(filtered);
    const favoriteTargets = targets.filter((t) => t.type === "favorite");
    expect(favoriteTargets).toHaveLength(2);
    expect(favoriteTargets[0].tabId).toBe("tab-a");
    expect(favoriteTargets[0].title).toBe("Tab A");
    expect(favoriteTargets[1].tabId).toBe("tab-b");
    expect(favoriteTargets[1].title).toBe("Tab B");
  });
});
