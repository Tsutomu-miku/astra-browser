import { readFileSync } from "node:fs";
import { join } from "node:path";
import { describe, expect, it } from "vitest";

const sidebarCss = readFileSync(join(__dirname, "../src/renderer/styles/sidebar.css"), "utf8");
const workspaceCss = readFileSync(join(__dirname, "../src/renderer/styles/sidebar-workspaces.css"), "utf8");
const contextMenuCss = readFileSync(join(__dirname, "../src/renderer/styles/sidebar-context-menu.css"), "utf8");
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

    for (const block of [tabHoverBlock, favoriteHoverBlock, closedHoverBlock]) {
      expect(block).toContain("background: var(--panel-soft)");
      expect(block).toContain("border-color: transparent");
      expect(block).not.toContain("var(--accent)");
    }
  });

  it("keeps row status indicators as inline glyphs instead of badges", () => {
    const statusBlock = getRuleBlock(sidebarCss, "\n.tab-status-badge {");

    expect(statusBlock).toContain("width: 14px");
    expect(statusBlock).toContain("height: 14px");
    expect(statusBlock).toContain("border: 0");
    expect(statusBlock).toContain("background: transparent");
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
    const rowSplitBlock = getRuleBlock(sidebarCss, "\n.tab-status-badge.is-split");
    const pinnedSplitBlock = getRuleBlock(sidebarCss, "\n.pinned-tab-status-badge.is-split");
    const loadingBlock = getRuleBlock(sidebarCss, "\n.sidebar-item-icon-status.is-loading");
    const internalBlock = getRuleBlock(sidebarCss, "\n.sidebar-item-icon.is-internal");

    for (const block of [rowSplitBlock, pinnedSplitBlock, loadingBlock, internalBlock]) {
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
    const newWorkspaceHoverBlock = getRuleBlock(workspaceCss, ".workspace-new-button:hover");
    const workspaceMenuBlock = getRuleBlock(workspaceCss, ".workspace-context-menu");
    const workspaceInputFocusBlock = getRuleBlock(workspaceCss, ".workspace-menu-field input:focus");

    for (const block of [
      resizeBlock,
      essentialBlock,
      essentialIconBlock,
      newWorkspaceHoverBlock,
      workspaceMenuBlock,
      workspaceInputFocusBlock
    ]) {
      expect(block).not.toContain("var(--accent)");
    }
    expect(essentialBlock).toContain("border-color: transparent");
    expect(essentialIconBlock).toContain("background: var(--control)");
    expect(newWorkspaceHoverBlock).toContain("border-color: transparent");
    expect(workspaceInputFocusBlock).toContain("box-shadow: none");
  });

  it("keeps sidebar menu color pickers quiet while preserving selected state", () => {
    const groupInputFocusBlock = getRuleBlock(contextMenuCss, ".tab-group-menu-field input:focus-visible");
    const groupSwatchBlock = getRuleBlock(contextMenuCss, ".tab-context-menu .tab-group-menu-swatch");
    const groupSwatchHoverBlock = getRuleBlock(contextMenuCss, ".tab-context-menu .tab-group-menu-swatch:hover,\n.tab-context-menu .tab-group-menu-swatch:focus-visible");
    const groupSwatchSelectedBlock = getRuleBlock(contextMenuCss, ".tab-context-menu .tab-group-menu-swatch[aria-pressed=\"true\"]");
    const workspaceSwatchHoverBlock = getRuleBlock(workspaceCss, ".workspace-context-menu .workspace-menu-swatch:hover,\n.workspace-context-menu .workspace-menu-swatch:focus-visible");
    const workspaceSwatchSelectedBlock = getRuleBlock(workspaceCss, ".workspace-context-menu .workspace-menu-swatch[aria-pressed=\"true\"]");

    expect(groupInputFocusBlock).toContain("border-color: var(--line-strong)");
    expect(groupInputFocusBlock).toContain("box-shadow: none");
    expect(groupInputFocusBlock).not.toContain("var(--group-color)");
    expect(groupSwatchBlock).toContain("border: 1px solid rgba(255, 255, 255, 0.14)");

    for (const block of [
      groupSwatchHoverBlock,
      groupSwatchSelectedBlock,
      workspaceSwatchHoverBlock,
      workspaceSwatchSelectedBlock
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
