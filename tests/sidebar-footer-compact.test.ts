import { createElement } from "react";
import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, it, vi } from "vitest";

import type { BrowserController } from "../src/renderer/app/controller/types";
import { SidebarFooter } from "../src/renderer/surfaces/sidebar/components/chrome/SidebarFooter";

describe("sidebar footer compact controls", () => {
  it("turns the sidebar toggle into a floating-sidebar pin control in compact mode", () => {
    expect(renderFooter({ compactMode: false, floatingSidebarOpen: false })).toContain('aria-label="Focus sidebar"');

    const unpinned = renderFooter({ compactMode: true, floatingSidebarOpen: false });
    expect(unpinned).toContain('aria-label="Pin floating sidebar"');
    expect(unpinned).toContain('aria-pressed="false"');

    const pinned = renderFooter({ compactMode: true, floatingSidebarOpen: true });
    expect(pinned).toContain('aria-label="Unpin floating sidebar"');
    expect(pinned).toContain('aria-pressed="true"');
  });
});

function renderFooter({
  compactMode,
  floatingSidebarOpen
}: {
  compactMode: boolean;
  floatingSidebarOpen: boolean;
}) {
  return renderToStaticMarkup(createElement(SidebarFooter, {
    actions: {
      setSplitLayout: vi.fn(),
      toggleCompactMode: vi.fn(),
      toggleSidebar: vi.fn(),
      toggleSplitMode: vi.fn()
    } as unknown as BrowserController["actions"],
    compactMode,
    floatingSidebarOpen,
    setPanel: vi.fn(),
    splitLayout: "horizontal",
    splitMode: false
  }));
}
