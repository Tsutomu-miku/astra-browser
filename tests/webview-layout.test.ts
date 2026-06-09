import { describe, expect, it } from "vitest";

import { openTabInSplit, openUrlInActiveWorkspace, sleepInactiveTabs, toggleSplitMode } from "../src/renderer/domain/actions";
import { createDefaultState, isInternalNewTabUrl } from "../src/renderer/domain/browser";
import { getActiveTab, getActiveWorkspace } from "../src/renderer/domain/browser/selectors";
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

    expect(canResizeSplitLayout("horizontal", 2)).toBe(true);
    expect(canResizeSplitLayout("vertical", 2)).toBe(true);
    expect(canResizeSplitLayout("grid", 2)).toBe(false);
    expect(canResizeSplitLayout("horizontal", 3)).toBe(false);
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
    const layout = getKeepAliveWebviewTabs(workspace, getActiveTab(workspace), second);

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
    const layout = getKeepAliveWebviewTabs(workspace, getActiveTab(workspace), split);

    expect(layout.map((entry) => entry.isVisible)).toEqual([true, true, false]);
    expect(layout[0].tab.id).toBe(workspace.activeTabId);
    expect(layout[1].tab.id).toBe(split.splitTabId);
  });

  it("renders up to four split webviews before background tabs", () => {
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
    const splitOnce = openTabInSplit(third, firstTab.id);
    const splitTwice = openTabInSplit(splitOnce, newTab.id);
    const splitThrice = openTabInSplit(splitTwice, secondTab.id);
    const layout = getKeepAliveWebviewTabs(getActiveWorkspace(splitThrice), getActiveTab(getActiveWorkspace(splitThrice)), splitThrice);

    expect(layout.map((entry) => entry.isVisible)).toEqual([true, true, true, true]);
    expect(layout.map((entry) => entry.tab.id)).toEqual([
      getActiveWorkspace(splitThrice).activeTabId,
      firstTab.id,
      newTab.id,
      secondTab.id
    ]);
  });

  it("omits sleeping background tabs from mounted webviews", () => {
    const first = openUrlInActiveWorkspace(createDefaultState(), "first.test", "First");
    const second = openUrlInActiveWorkspace(first, "second.test", "Second");
    const slept = sleepInactiveTabs(second);
    const workspace = getActiveWorkspace(slept);
    const layout = getKeepAliveWebviewTabs(workspace, getActiveTab(workspace), slept);

    expect(layout).toHaveLength(1);
    expect(layout[0].tab.title).toBe("Second");
  });
});
