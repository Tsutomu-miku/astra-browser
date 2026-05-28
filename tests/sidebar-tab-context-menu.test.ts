import { createElement } from "react";
import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, it, vi } from "vitest";

import { createTab } from "../src/renderer/domain/browser";
import { TabContextMenu } from "../src/renderer/surfaces/sidebar/components/tabs/TabContextMenu";

describe("sidebar tab context menu", () => {
  it("offers Sleep tab for awake tabs", () => {
    const html = renderMenu(false);

    expect(html).toContain("Sleep tab");
    expect(html).not.toContain("Wake tab");
  });

  it("offers Wake tab for sleeping tabs", () => {
    const html = renderMenu(true);

    expect(html).toContain("Wake tab");
    expect(html).not.toContain("Sleep tab");
  });
});

function renderMenu(isSleeping: boolean): string {
  return renderToStaticMarkup(createElement(TabContextMenu, {
    cleanupState: {
      canCloseOtherTabs: true,
      canCloseTabsToLeft: false,
      canCloseTabsToRight: false
    },
    groupMenuState: {
      canCreateGroup: false,
      canUngroup: false,
      moveGroupTargets: []
    },
    left: 10,
    moveWorkspaceTargets: [],
    onClose: vi.fn(),
    onCloseOtherTabs: vi.fn(),
    onCloseTab: vi.fn(),
    onCloseTabsToLeft: vi.fn(),
    onCloseTabsToRight: vi.fn(),
    onDuplicate: vi.fn(),
    onGroupTab: vi.fn(),
    onMoveToGroup: vi.fn(),
    onMoveToWorkspace: vi.fn(),
    onOpenGlance: vi.fn(),
    onOpenInSplit: vi.fn(),
    onSelect: vi.fn(),
    onSleepTab: vi.fn(),
    onToggleEssential: vi.fn(),
    onToggleFavorite: vi.fn(),
    onToggleMuted: vi.fn(),
    onTogglePinned: vi.fn(),
    onUngroupTab: vi.fn(),
    tab: {
      ...createTab("Docs", "https://docs.example"),
      isSleeping
    },
    tabIsEssential: false,
    tabIsFavorite: false,
    top: 20
  }));
}
