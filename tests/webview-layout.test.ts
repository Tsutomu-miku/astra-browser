import { describe, expect, it } from "vitest";

import { openUrlInActiveWorkspace, sleepInactiveTabs, toggleSplitMode } from "../src/renderer/domain/browser-actions";
import { createDefaultState, isInternalNewTabUrl } from "../src/renderer/domain/browser-core";
import { getActiveTab, getActiveWorkspace } from "../src/renderer/domain/selectors";
import { getKeepAliveWebviewTabs } from "../src/renderer/surfaces/webview/webviewLayout";

describe("webview layout", () => {
  it("keeps inactive workspace tabs mounted after the active webview", () => {
    const first = openUrlInActiveWorkspace(createDefaultState(), "first.test", "First");
    const second = openUrlInActiveWorkspace(first, "second.test", "Second");
    const workspace = getActiveWorkspace(second);
    const layout = getKeepAliveWebviewTabs(workspace, getActiveTab(workspace), second);

    expect(layout.map((entry) => entry.tab.title)).toEqual(["Second", "New Tab", "First"]);
    expect(layout.map((entry) => entry.isVisible)).toEqual([true, false, false]);
    expect(isInternalNewTabUrl(layout[1].tab.url)).toBe(true);
  });

  it("renders active and split webviews first while keeping the rest alive", () => {
    const first = openUrlInActiveWorkspace(createDefaultState(), "first.test", "First");
    const second = openUrlInActiveWorkspace(first, "second.test", "Second");
    const split = toggleSplitMode(second);
    const workspace = getActiveWorkspace(split);
    const layout = getKeepAliveWebviewTabs(workspace, getActiveTab(workspace), split);

    expect(layout.map((entry) => entry.isVisible)).toEqual([true, true, false]);
    expect(layout[0].tab.id).toBe(workspace.activeTabId);
    expect(layout[1].tab.id).toBe(split.splitTabId);
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
