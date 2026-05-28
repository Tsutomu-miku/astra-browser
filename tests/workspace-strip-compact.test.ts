import { createElement } from "react";
import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, it, vi } from "vitest";

import { createDefaultState } from "../src/renderer/domain/browser";
import { getActiveWorkspace } from "../src/renderer/domain/browser/selectors";
import { WorkspaceStrip } from "../src/renderer/surfaces/sidebar/components/workspaces/WorkspaceStrip";

describe("workspace strip compact controls", () => {
  it("uses the always-visible sidebar toggle as a compact floating-sidebar pin", () => {
    const normal = renderStrip({ compactMode: false, floatingSidebarOpen: false, sidebarCollapsed: false });
    expect(normal).toContain('aria-label="Collapse sidebar"');

    const compact = renderStrip({ compactMode: true, floatingSidebarOpen: false, sidebarCollapsed: true });
    expect(compact).toContain('aria-label="Pin floating sidebar"');
    expect(compact).toContain('aria-pressed="false"');

    const pinned = renderStrip({ compactMode: true, floatingSidebarOpen: true, sidebarCollapsed: true });
    expect(pinned).toContain('aria-label="Unpin floating sidebar"');
    expect(pinned).toContain('aria-pressed="true"');
  });

  it("marks other Spaces as drop targets while dragging a tab group", () => {
    const html = renderStrip({
      compactMode: false,
      draggingGroupId: "group",
      floatingSidebarOpen: false,
      sidebarCollapsed: false
    });

    expect(html).toContain('data-drop-target="true"');
  });

  it("marks New Space as a drop target while dragging tabs or groups", () => {
    const tabHtml = renderStrip({
      compactMode: false,
      draggingTabId: "tab",
      floatingSidebarOpen: false,
      sidebarCollapsed: false
    });
    const groupHtml = renderStrip({
      compactMode: false,
      draggingGroupId: "group",
      floatingSidebarOpen: false,
      sidebarCollapsed: false
    });

    expect(tabHtml).toContain('class="workspace-button workspace-new-button"');
    expect(tabHtml).toContain('aria-label="New Space" data-drop-target="true"');
    expect(groupHtml).toContain('aria-label="New Space" data-drop-target="true"');
  });
});

function renderStrip({
  compactMode,
  draggingGroupId = null,
  draggingTabId = null,
  floatingSidebarOpen,
  sidebarCollapsed
}: {
  compactMode: boolean;
  draggingGroupId?: string | null;
  draggingTabId?: string | null;
  floatingSidebarOpen: boolean;
  sidebarCollapsed: boolean;
}) {
  const state = createDefaultState();
  const activeWorkspace = getActiveWorkspace(state);

  return renderToStaticMarkup(createElement(WorkspaceStrip, {
    activeWorkspaceId: activeWorkspace.id,
    compactMode,
    draggingGroupId,
    draggingTabId,
    draggingWorkspaceId: null,
    floatingSidebarOpen,
    onDragEnd: vi.fn(),
    onDragOver: vi.fn(),
    onDragStart: vi.fn(),
    onDrop: vi.fn(),
    onDeleteWorkspace: vi.fn(),
    onNewWorkspace: vi.fn(),
    onNewWorkspaceDrop: vi.fn(),
    onSelect: vi.fn(),
    onToggleSidebar: vi.fn(),
    onUpdateWorkspace: vi.fn(),
    sidebarCollapsed,
    workspaces: state.workspaces
  }));
}
