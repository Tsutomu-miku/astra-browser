import { readFileSync } from "node:fs";
import { join } from "node:path";
import { describe, expect, it } from "vitest";

const sidebarCss = readFileSync(join(__dirname, "../src/renderer/styles/sidebar.css"), "utf8");
const sidebarDropZonesCss = readFileSync(join(__dirname, "../src/renderer/styles/sidebar-drop-zones.css"), "utf8");
const workspaceCss = readFileSync(join(__dirname, "../src/renderer/styles/sidebar-workspaces.css"), "utf8");
const sidebarMenuCss = readFileSync(join(__dirname, "../src/renderer/styles/sidebar-menu.css"), "utf8");
const baseCss = readFileSync(join(__dirname, "../src/renderer/styles/base.css"), "utf8");

describe("sidebar selection styles", () => {
  it("keeps tab and favorite selected states neutral instead of border-heavy", () => {
    const activeTabBlock = getRuleBlock(sidebarCss, ".tab-row[aria-current=\"true\"]");
    const activeFavoriteBlock = getRuleBlock(sidebarCss, ".favorite-button[aria-current=\"true\"]");
    const searchSelectedBlock = getRuleBlock(sidebarCss, ".tab-row[aria-selected=\"true\"],\n.favorite-button[aria-selected=\"true\"]");

    for (const block of [activeTabBlock, activeFavoriteBlock, searchSelectedBlock]) {
      expect(block).toContain("border-color: transparent");
      expect(block).toContain("box-shadow: none");
      expect(block).not.toContain("inset");
      expect(block).not.toContain("var(--accent)");
    }
  });

  it("uses quiet whole-row focus states for sidebar items", () => {
    const tabFocusBlock = getRuleBlock(sidebarCss, ".tab-row:focus-within");
    const favoriteFocusBlock = getRuleBlock(sidebarCss, ".favorite-button:focus-visible");
    const pinnedFocusBlock = getRuleBlock(sidebarCss, "\n.pinned-tab-button:focus-visible {");
    const closedFocusBlock = getRuleBlock(sidebarCss, ".closed-tab-button:focus-visible");

    for (const block of [tabFocusBlock, favoriteFocusBlock, closedFocusBlock]) {
      expect(block).toContain("border-color: transparent");
      expect(block).toContain("background: var(--panel-soft)");
      expect(block).not.toContain("var(--accent)");
    }
    expect(favoriteFocusBlock).toContain("outline: none");
    expect(pinnedFocusBlock).toContain("outline: none");
    expect(closedFocusBlock).toContain("outline: none");
  });

  it("keeps sidebar hover states quiet and fill-based", () => {
    const tabHoverBlock = getRuleBlock(sidebarCss, ".tab-row:hover");
    const favoriteHoverBlock = getRuleBlock(sidebarCss, ".favorite-button:hover");
    const closedHoverBlock = getRuleBlock(sidebarCss, ".closed-tab-button:hover");
    const pinnedHoverBlock = getRuleBlock(sidebarCss, ".pinned-tab-button:hover,\n.pinned-tab-button:focus-visible,\n.pinned-tab-button[aria-current=\"true\"],\n.pinned-tab-button[aria-selected=\"true\"]");
    const pinnedIconHoverBlock = getRuleBlock(sidebarCss, ".pinned-tab-button:hover .pinned-tab-icon,\n.pinned-tab-button:focus-visible .pinned-tab-icon");
    const workspaceHoverBlock = getRuleBlock(workspaceCss, ".workspace-button:hover");

    for (const block of [tabHoverBlock, favoriteHoverBlock, closedHoverBlock]) {
      expect(block).toContain("background: var(--panel-soft)");
      expect(block).toContain("border-color: transparent");
      expect(block).not.toContain("var(--accent)");
    }
    for (const block of [pinnedHoverBlock, pinnedIconHoverBlock, workspaceHoverBlock]) {
      expect(block).not.toContain("transform:");
      expect(block).not.toContain("translateY");
    }
  });

  it("separates pressed feedback from dragging affordances", () => {
    const tabPressedBlock = getRuleBlock(sidebarCss, ".tab-row:active");
    const tabRowBlock = getRuleBlock(sidebarCss, "\n.tab-row {");
    const favoriteBlock = getRuleBlock(sidebarCss, "\n.favorite-button {");
    const closedBlock = getRuleBlock(sidebarCss, "\n.closed-tab-button {");
    const quickEntryPressedBlock = getRuleBlock(sidebarCss, ".pinned-tab-button:active,\n.favorite-button:active,\n.closed-tab-button:active");
    const tabDraggingBlock = getRuleBlock(sidebarCss, ".tab-row[data-dragging=\"true\"]");
    const pinnedDraggingBlock = getRuleBlock(sidebarCss, ".pinned-tab-button[data-dragging=\"true\"]");
    const closedDraggingBlock = getRuleBlock(sidebarCss, ".closed-tab-button[data-dragging=\"true\"]");
    const quickEntryDraggingBlock = getRuleBlock(sidebarDropZonesCss, ".favorite-button[data-dragging=\"true\"]");

    for (const block of [tabPressedBlock, quickEntryPressedBlock]) {
      expect(block).toContain("background: var(--sidebar-selected-bg)");
      expect(block).toContain("border-color: transparent");
      expect(block).not.toContain("cursor: grabbing");
      expect(block).not.toContain("var(--accent)");
    }
    for (const block of [tabRowBlock, favoriteBlock]) {
      expect(block).toContain("cursor: pointer");
      expect(block).not.toContain("cursor: grab");
    }
    for (const block of [favoriteBlock, closedBlock]) {
      expect(block).toContain("background: transparent");
    }
    for (const block of [tabDraggingBlock, pinnedDraggingBlock, closedDraggingBlock, quickEntryDraggingBlock]) {
      expect(block).toContain("cursor: grabbing");
      expect(block).not.toContain("transform");
    }
    expect(sidebarDropZonesCss).not.toContain('cursor: grab;');
    expect(sidebarDropZonesCss).not.toContain('.favorite-button[draggable="true"]:active');
  });

  it("keeps row status indicators as inline glyphs instead of badges", () => {
    const statusBlock = getRuleBlock(sidebarCss, "[data-sidebar-tab-status-badge=\"true\"]");

    expect(statusBlock).toContain("width: var(--sidebar-tab-status-badge-size, 14px)");
    expect(statusBlock).toContain("height: var(--sidebar-tab-status-badge-size, 14px)");
    expect(statusBlock).toContain("border: 0");
    expect(statusBlock).toContain("background: transparent");
    expect(statusBlock).not.toContain("border-radius");
    expect(sidebarCss).not.toContain(".tab-row.is-split-tab .tab-favicon::after");
  });

  it("keeps recently closed restore affordance icon-only", () => {
    const restoreBlock = getRuleBlock(sidebarCss, "\n.closed-tab-action {");
    const closedMainBlock = getRuleBlock(sidebarCss, ".closed-tab-main");

    expect(restoreBlock).toContain("width: 22px");
    expect(restoreBlock).toContain("display: inline-grid");
    expect(restoreBlock).toContain("background: transparent");
    expect(restoreBlock).not.toContain("font-weight");
    expect(closedMainBlock).toContain("display: block");
    expect(sidebarCss).not.toContain(".closed-tab-url");
  });

  it("keeps sidebar status and internal-page colors neutral", () => {
    const iconStatusBlock = getRuleBlock(sidebarCss, "\n.sidebar-item-icon-status {");
    const splitBlock = getRuleBlock(sidebarCss, "[data-sidebar-tab-status-badge=\"true\"].is-split");
    const loadingBlock = getRuleBlock(sidebarCss, "\n.sidebar-item-icon-status.is-loading");
    const internalBlock = getRuleBlock(sidebarCss, "\n.sidebar-item-icon.is-internal");

    expect(iconStatusBlock).toContain("border: 0");
    expect(iconStatusBlock).toContain("background: transparent");
    expect(iconStatusBlock).not.toContain("border-radius");
    for (const block of [splitBlock, loadingBlock, internalBlock]) {
      expect(block).toContain("var(--sidebar-status-strong)");
      expect(block).not.toContain("var(--accent)");
    }
    expect(sidebarCss).not.toContain("--sidebar-subtle-accent");
  });

  it("keeps workspace active state quiet and removes accent wash from the app background", () => {
    const activeWorkspaceBlock = getRuleBlock(workspaceCss, ".workspace-button[aria-current=\"true\"]");

    expect(activeWorkspaceBlock).toContain("border-color: transparent");
    expect(activeWorkspaceBlock).toContain("box-shadow: none");
    expect(activeWorkspaceBlock).not.toContain("var(--accent)");
    expect(baseCss).not.toContain("radial-gradient");
  });

  it("keeps primary sidebar chrome neutral by default", () => {
    const resizeBlock = getRuleBlock(sidebarCss, ".sidebar-resize-handle:hover::before,\n.sidebar-resize-handle:focus-visible::before,\n.sidebar-resize-handle[data-dragging=\"true\"]::before");
    const essentialBlock = getRuleBlock(sidebarCss, ".essentials .favorite-button");
    const essentialIconBlock = getRuleBlock(sidebarCss, ".essentials .favorite-icon");
    const newWorkspaceBlock = getRuleBlock(workspaceCss, ".workspace-new-button");
    const newWorkspaceHoverBlock = getRuleBlock(workspaceCss, ".workspace-new-button:hover");
    const sidebarMenuSurfaceBlock = getRuleBlock(sidebarMenuCss, ".sidebar-menu-surface");
    const workspaceLabelBlock = getRuleBlock(sidebarMenuCss, ".sidebar-menu-field span");
    const workspaceInputBlock = getRuleBlock(sidebarMenuCss, ".sidebar-menu-field input");
    const workspaceInputFocusBlock = getRuleBlock(sidebarMenuCss, ".sidebar-menu-field input:focus,\n.sidebar-menu-field input:focus-visible");

    for (const block of [
      resizeBlock,
      essentialBlock,
      essentialIconBlock,
      newWorkspaceHoverBlock,
      sidebarMenuSurfaceBlock,
      workspaceInputFocusBlock
    ]) {
      expect(block).not.toContain("var(--accent)");
    }
    expect(essentialBlock).toContain("border-color: transparent");
    expect(essentialIconBlock).toContain("background: var(--control)");
    expect(newWorkspaceBlock).toContain("border-color: transparent");
    expect(newWorkspaceBlock).toContain("background: transparent");
    expect(newWorkspaceHoverBlock).toContain("border-color: transparent");
    expect(sidebarMenuSurfaceBlock).toContain("position: fixed");
    expect(sidebarMenuSurfaceBlock).toContain("max-height: calc(100vh - 24px)");
    expect(sidebarMenuSurfaceBlock).toContain("overflow: auto");
    expect(workspaceLabelBlock).toContain("text-transform: none");
    expect(workspaceInputBlock).toContain("border: 1px solid transparent");
    expect(workspaceInputBlock).toContain("background: transparent");
    expect(workspaceInputFocusBlock).toContain("background: rgba(255, 255, 255, 0.06)");
    expect(workspaceInputFocusBlock).toContain("box-shadow: none");
  });

  it("keeps sidebar menu color pickers quiet while preserving selected state", () => {
    const groupInputBlock = getRuleBlock(sidebarMenuCss, ".sidebar-menu-field input");
    const groupInputFocusBlock = getRuleBlock(sidebarMenuCss, ".sidebar-menu-field input:focus,\n.sidebar-menu-field input:focus-visible");
    const groupSwatchBlock = getRuleBlock(sidebarMenuCss, ".sidebar-menu-surface .sidebar-menu-swatch");
    const groupSwatchHoverBlock = getRuleBlock(sidebarMenuCss, ".sidebar-menu-surface .sidebar-menu-swatch:hover,\n.sidebar-menu-surface .sidebar-menu-swatch:focus-visible");
    const groupSwatchSelectedBlock = getRuleBlock(sidebarMenuCss, ".sidebar-menu-surface .sidebar-menu-swatch[aria-pressed=\"true\"]");

    expect(groupInputBlock).toContain("border: 1px solid transparent");
    expect(groupInputBlock).toContain("background: transparent");
    expect(groupInputFocusBlock).toContain("border-color: var(--line-strong)");
    expect(groupInputFocusBlock).toContain("background: rgba(255, 255, 255, 0.06)");
    expect(groupInputFocusBlock).toContain("box-shadow: none");
    expect(groupInputFocusBlock).not.toContain("var(--group-color)");
    expect(groupSwatchBlock).toContain("border: 1px solid rgba(255, 255, 255, 0.14)");
    expect(groupSwatchBlock).toContain("border-radius: 6px");
    expect(groupSwatchBlock).not.toContain("border-radius: 999px");

    for (const block of [
      groupSwatchHoverBlock,
      groupSwatchSelectedBlock
    ]) {
      expect(block).toContain("box-shadow: inset");
      expect(block).not.toContain("color-mix");
      expect(block).not.toContain("var(--accent)");
      expect(block).not.toContain("border-color: white");
    }
  });
});

function getRuleBlock(css: string, selector: string): string {
  const start = css.indexOf(selector);
  expect(start).toBeGreaterThanOrEqual(0);
  const bodyStart = css.indexOf("{", start);
  const bodyEnd = css.indexOf("}", bodyStart);
  return css.slice(bodyStart + 1, bodyEnd);
}
