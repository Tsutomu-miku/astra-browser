import { createElement } from "react";
import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, it, vi } from "vitest";

import type { BrowserController } from "../src/renderer/app/controller/types";
import { createDefaultState } from "../src/renderer/domain/browser";
import { getActiveTab, getActiveWorkspace } from "../src/renderer/domain/browser/selectors";
import { Topbar } from "../src/renderer/surfaces/topbar/Topbar";

describe("compact toolbar controls", () => {
  it("shows a pressed floating-toolbar pin control only in compact mode", () => {
    expect(renderTopbar({ compactMode: false, floatingToolbarOpen: false })).not.toContain("Pin floating toolbar");

    const unpinned = renderTopbar({ compactMode: true, floatingToolbarOpen: false });
    expect(unpinned).toContain('aria-label="Pin floating toolbar"');
    expect(unpinned).toContain('aria-pressed="false"');

    const pinned = renderTopbar({ compactMode: true, floatingToolbarOpen: true });
    expect(pinned).toContain('aria-label="Unpin floating toolbar"');
    expect(pinned).toContain('aria-pressed="true"');
  });
});

function renderTopbar({
  compactMode,
  floatingToolbarOpen
}: {
  compactMode: boolean;
  floatingToolbarOpen: boolean;
}) {
  return renderToStaticMarkup(createElement(Topbar, {
    controller: createController({ compactMode, floatingToolbarOpen })
  }));
}

function createController({
  compactMode,
  floatingToolbarOpen
}: {
  compactMode: boolean;
  floatingToolbarOpen: boolean;
}): BrowserController {
  const state = createDefaultState();
  const activeWorkspace = getActiveWorkspace(state);
  const activeTab = getActiveTab(activeWorkspace);
  const noop = vi.fn();

  return {
    actions: {
      closeActiveTab: noop,
      navigateActiveTab: noop,
      openTabInSplit: noop,
      openUrlInSplit: noop,
      resetActiveTabZoom: noop,
      runWebviewAction: noop,
      selectTab: noop,
      setSplitLayout: noop,
      toggleActiveTabEssential: noop,
      toggleActiveTabFavorite: noop,
      toggleActiveTabMuted: noop,
      toggleActiveTabPinned: noop,
      toggleFloatingToolbar: noop,
      zoomIn: noop,
      zoomOut: noop
    },
    activeTab,
    activeWebview: undefined,
    activeWorkspace,
    addressValue: activeTab.url,
    compactMode,
    floatingToolbarOpen,
    setAddressValue: noop,
    setPanel: noop,
    state
  } as unknown as BrowserController;
}
