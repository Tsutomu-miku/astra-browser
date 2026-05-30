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

  it("copies tab page details from the sidebar context menu", () => {
    const onClose = vi.fn();
    const onCopyText = vi.fn();
    const tab = createTab("Docs", "https://docs.example");
    const menu = createElement(TabContextMenu, {
      ...menuProps(false),
      onClose,
      onCopyText,
      tab
    });
    menu.props.onCopyText(tab.url);
    menu.props.onCopyText(tab.title || tab.url);

    const html = renderToStaticMarkup(menu);
    expect(html).toContain("Copy URL");
    expect(html).toContain("Copy title");
    expect(onCopyText).toHaveBeenCalledWith("https://docs.example");
    expect(onCopyText).toHaveBeenCalledWith("Docs");
  });

  it("renders quiet icon menu items without native tooltips", () => {
    const html = renderMenu(false);

    expect(html).toContain('class="sidebar-menu-item"');
    expect(html).toContain('class="sidebar-menu-item-icon" aria-hidden="true"');
    expect(html).toContain('class="sidebar-menu-item-label"');
    expect(html).not.toContain('title="');
  });
});

function renderMenu(isSleeping: boolean): string {
  return renderToStaticMarkup(createElement(TabContextMenu, menuProps(isSleeping)));
}

function menuProps(isSleeping: boolean) {
  return {
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
    onCopyText: vi.fn(),
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
  };
}
