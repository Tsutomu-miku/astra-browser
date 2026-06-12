import { describe, expect, it } from "vitest";

import { fillSplitView, openTabInSplit, openUrlInActiveWorkspace, sleepInactiveTabs, toggleSplitMode } from "../src/renderer/domain/actions";
import { createDefaultState, isInternalNewTabUrl } from "../src/renderer/domain/browser";
import { getActiveWorkspace } from "../src/renderer/domain/browser/selectors";
import {
  canResizeSplitLayout,
  getKeepAliveWebviewTabs,
  getSplitRatioFromPoint,
  normalizeSplitRatio
} from "../src/renderer/surfaces/webview/webviewLayout";

describe("webview layout", () => {
  it("clamps resizable split ratios for usable panes", () => {
    expect(normalizeSplitRatio(Number.NaN)).toBe(0.5);
    expect(normalizeSplitRatio(0.1)).toBe(0.25);
    expect(normalizeSplitRatio(0.8)).toBe(0.75);
    expect(normalizeSplitRatio(0.6)).toBe(0.6);
  });

  it("supports horizontal and vertical two-pane split resizing", () => {
    const rect = { height: 400, left: 100, top: 40, width: 800 };

    expect(canResizeSplitLayout("horizontal", true)).toBe(true);
    expect(canResizeSplitLayout("vertical", true)).toBe(true);
    expect(canResizeSplitLayout("grid", true)).toBe(false);
    expect(canResizeSplitLayout("horizontal", false)).toBe(false);
    expect(getSplitRatioFromPoint("horizontal", { x: 500, y: 70 }, rect)).toBe(0.5);
    expect(getSplitRatioFromPoint("vertical", { x: 120, y: 240 }, rect)).toBe(0.5);
    expect(getSplitRatioFromPoint("vertical", { x: 120, y: 20 }, rect)).toBe(0.25);
    expect(getSplitRatioFromPoint("horizontal", { x: 900, y: 20 }, rect)).toBe(0.75);
  });

  it("keeps inactive workspace tabs mounted after the active webview", () => {
    const base = createDefaultState();
    const personalWs = base.workspaces.find((ws) => ws.id === "personal")!;
    personalWs.tabs = personalWs.tabs.filter((tab) => !tab.isFavorite);
    personalWs.favoriteOrder = [];
    const first = openUrlInActiveWorkspace(base, "first.test", "First");
    const second = openUrlInActiveWorkspace(first, "second.test", "Second");
    const workspace = getActiveWorkspace(second);
    const layout = getKeepAliveWebviewTabs(workspace);

    expect(layout.map((entry) => entry.tab.title)).toEqual(["Second", "New Tab", "First"]);
    expect(layout.map((entry) => entry.isVisible)).toEqual([true, false, false]);
    expect(isInternalNewTabUrl(layout[1].tab.url)).toBe(true);
  });

  it("renders active and split webviews first while keeping the rest alive", () => {
    const base = createDefaultState();
    const personalWs = base.workspaces.find((ws) => ws.id === "personal")!;
    personalWs.tabs = personalWs.tabs.filter((tab) => !tab.isFavorite);
    personalWs.favoriteOrder = [];
    const first = openUrlInActiveWorkspace(base, "first.test", "First");
    const second = openUrlInActiveWorkspace(first, "second.test", "Second");
    const split = toggleSplitMode(second);
    const workspace = getActiveWorkspace(split);
    const layout = getKeepAliveWebviewTabs(workspace);

    expect(layout.map((entry) => entry.isVisible)).toEqual([true, true, false]);
    expect(layout[0].tab.id).toBe(workspace.activeTabId);
    expect(layout[1].tab.id).toBe(workspace.activeAncillaryTabId);
  });

  it("keeps split tabs visible and other tabs alive in background (entity model)", () => {
    const base = createDefaultState();
    const personalWs = base.workspaces.find((ws) => ws.id === "personal")!;
    personalWs.tabs = personalWs.tabs.filter((tab) => !tab.isFavorite);
    personalWs.favoriteOrder = [];
    const first = openUrlInActiveWorkspace(base, "first.test", "First");
    const second = openUrlInActiveWorkspace(first, "second.test", "Second");
    const third = openUrlInActiveWorkspace(second, "third.test", "Third");
    const workspace = getActiveWorkspace(third);
    const firstTab = workspace.tabs.find((tab) => tab.title === "First")!;
    const secondTab = workspace.tabs.find((tab) => tab.title === "Second")!;
    const newTab = workspace.tabs.find((tab) => tab.title === "New Tab")!;
    // Entity model: openTabInSplit replaces the secondary tab each time.
    // After 3 calls, the split has exactly 2 tabs: primary (active) + secondary (last added).
    const splitOnce = openTabInSplit(third, firstTab.id);
    const splitTwice = openTabInSplit(splitOnce, newTab.id);
    const splitThrice = openTabInSplit(splitTwice, secondTab.id);
    const finalWs = getActiveWorkspace(splitThrice);
    const layout = getKeepAliveWebviewTabs(finalWs);

    // All 4 tabs kept alive (1 primary + 1 secondary in split + 2 background tabs)
    expect(layout).toHaveLength(4);
    // 2 visible: active primary + active secondary (horizontal mode = single ancillary visible)
    const visibleIds = layout.filter((e) => e.isVisible).map((e) => e.tab.id);
    expect(visibleIds).toHaveLength(2);
    expect(visibleIds).toContain(finalWs.activeTabId);
    expect(visibleIds).toContain(secondTab.id); // last added = secondary of split
    // Order: active primary first, then active ancillary, then other alive tabs
    expect(layout[0].tab.id).toBe(finalWs.activeTabId);
    expect(layout[0].pane).toBe("primary");
    expect(layout[1].tab.id).toBe(secondTab.id);
    expect(layout[1].pane).toBe("ancillary");
    // Background tabs have pane "primary" (they are not in the split)
    expect(layout[2].pane).toBe("primary");
    expect(layout[3].pane).toBe("primary");
    // Background tabs are not visible
    expect(layout[2].isVisible).toBe(false);
    expect(layout[3].isVisible).toBe(false);
  });

  it("shows all ancillary tabs in grid layout mode", () => {
    const base = createDefaultState();
    const personalWs = base.workspaces.find((ws) => ws.id === "personal")!;
    personalWs.tabs = personalWs.tabs.filter((tab) => !tab.isFavorite);
    personalWs.favoriteOrder = [];
    const first = openUrlInActiveWorkspace(base, "first.test", "First");
    const second = openUrlInActiveWorkspace(first, "second.test", "Second");
    const third = openUrlInActiveWorkspace(second, "third.test", "Third");
    const workspace = getActiveWorkspace(third);
    const firstTab = workspace.tabs.find((tab) => tab.title === "First")!;
    const secondTab = workspace.tabs.find((tab) => tab.title === "Second")!;
    const newTab = workspace.tabs.find((tab) => tab.title === "New Tab")!;
    const withGrid = fillSplitView(third);
    const layout = getKeepAliveWebviewTabs(getActiveWorkspace(withGrid));

    // All tabs visible in grid mode
    expect(layout.filter((entry) => entry.isVisible)).toHaveLength(4);
    expect(layout[0].pane).toBe("primary");
    expect(layout.slice(1).every((entry) => entry.pane === "ancillary")).toBe(true);
  });

  it("omits sleeping background tabs from mounted webviews", () => {
    const first = openUrlInActiveWorkspace(createDefaultState(), "first.test", "First");
    const second = openUrlInActiveWorkspace(first, "second.test", "Second");
    const slept = sleepInactiveTabs(second);
    const workspace = getActiveWorkspace(slept);
    const layout = getKeepAliveWebviewTabs(workspace);

    expect(layout).toHaveLength(1);
    expect(layout[0].tab.title).toBe("Second");
  });
});
