import { readFileSync } from "node:fs";
import { join } from "node:path";
import { describe, expect, it } from "vitest";

const sidebarCss = readFileSync(join(__dirname, "../src/renderer/styles/sidebar-action-hints.css"), "utf8");
const sidebarLayoutCss = readFileSync(join(__dirname, "../src/renderer/styles/sidebar.css"), "utf8");

describe("sidebar item action hint styles", () => {
  it("reveals row action hints from stable in-flow slots on hover and keyboard focus", () => {
    expect(sidebarCss).not.toContain(".tab-row .sidebar-item-action-hints {\n  position: absolute");
    expect(sidebarCss).not.toContain(".favorite-button .sidebar-item-action-hints {\n  position: absolute");
    expect(sidebarCss).not.toContain(".closed-tab-button .sidebar-item-action-hints {\n  position: absolute");
    expect(sidebarCss).toContain("width: 38px");
    expect(sidebarCss).toContain("min-width: 38px");
    expect(sidebarCss).not.toContain("max-width");
    expect(sidebarCss).toContain(".tab-row:hover .sidebar-item-action-hints");
    expect(sidebarCss).toContain(".tab-row:focus-within .sidebar-item-action-hints");
    expect(sidebarCss).toContain(".favorite-button:hover .sidebar-item-action-hints");
    expect(sidebarCss).toContain(".favorite-button:focus-visible .sidebar-item-action-hints");
    expect(sidebarCss).toContain(".closed-tab-button:hover .sidebar-item-action-hints");
    expect(sidebarCss).toContain(".closed-tab-button:focus-visible .sidebar-item-action-hints");
    expect(sidebarCss).toContain(".pinned-tab-button:hover .sidebar-item-action-hints");
    expect(sidebarCss).toContain(".pinned-tab-button:focus-visible .sidebar-item-action-hints");
  });

  it("fades action hints in without hover slide motion", () => {
    const rowHintsBlock = getRuleBlock(sidebarCss, ".sidebar-item-action-hints");
    const rowRevealBlock = getRuleBlock(sidebarCss, ".tab-row:hover .sidebar-item-action-hints,\n.tab-row:focus-within .sidebar-item-action-hints,\n.favorite-button:hover .sidebar-item-action-hints,\n.favorite-button:focus-visible .sidebar-item-action-hints,\n.closed-tab-button:hover .sidebar-item-action-hints,\n.closed-tab-button:focus-visible .sidebar-item-action-hints");
    const pinnedHintsBlock = getRuleBlock(sidebarCss, ".pinned-tab-button .sidebar-item-action-hints");
    const pinnedRevealBlock = getRuleBlock(sidebarCss, ".pinned-tab-button:hover .sidebar-item-action-hints,\n.pinned-tab-button:focus-visible .sidebar-item-action-hints");

    expect(rowHintsBlock).toContain("transition: opacity 120ms ease");
    expect(rowHintsBlock).not.toContain("transform:");
    expect(rowRevealBlock).not.toContain("transform:");
    expect(pinnedHintsBlock).toContain("transform: translateX(-50%)");
    expect(pinnedHintsBlock).not.toContain("translate(-50%, 3px)");
    expect(pinnedRevealBlock).not.toContain("transform:");
  });

  it("keeps action hints as quiet inline glyphs instead of keycaps", () => {
    const hintBlock = getRuleBlock(sidebarCss, "[data-sidebar-modifier-hint=\"true\"]");
    const legacyHintBlock = getRuleBlock(sidebarCss, "\n.sidebar-item-action-hint {");

    expect(hintBlock).toContain("width: 16px");
    expect(hintBlock).toContain("height: 16px");
    expect(legacyHintBlock).toContain("border: 0");
    expect(legacyHintBlock).toContain("background: transparent");
    expect(hintBlock).not.toContain("border-radius");
    expect(hintBlock).not.toContain("var(--accent)");
  });

  it("reveals tab close controls on keyboard row focus", () => {
    expect(sidebarLayoutCss).toContain(".tab-row:focus-within .tab-close");
    expect(sidebarLayoutCss).toContain(".tab-close:focus-visible");
  });

  it("keeps tab close controls visually available without adding hidden Tab stops", () => {
    expect(sidebarLayoutCss).toContain("opacity: 0");
    expect(sidebarLayoutCss).toContain(".tab-row:hover .tab-close");
    expect(sidebarLayoutCss).toContain(".tab-row:focus-within .tab-close");
  });

  it("keeps tab row hints and close controls in one stable action rail", () => {
    const rowBlock = getRuleBlock(sidebarLayoutCss, ".tab-row");
    const buttonBlock = getRuleBlock(sidebarLayoutCss, "\n.tab-button {");
    const railBlock = getRuleBlock(sidebarLayoutCss, "\n.tab-row-actions");
    const railHintBlock = getRuleBlock(sidebarLayoutCss, "\n.tab-row-actions .sidebar-item-action-hints");

    expect(rowBlock).toContain("grid-template-columns: minmax(0, 1fr) 56px");
    expect(buttonBlock).toContain("grid-template-columns: 24px minmax(0, 1fr)");
    expect(buttonBlock).not.toContain("38px");
    expect(railBlock).toContain("width: 56px");
    expect(railBlock).toContain("justify-content: flex-end");
    expect(railHintBlock).toContain("width: 30px");
  });
});

function getRuleBlock(css: string, selector: string): string {
  const start = css.indexOf(selector);
  expect(start).toBeGreaterThanOrEqual(0);
  const bodyStart = css.indexOf("{", start);
  const bodyEnd = css.indexOf("}", bodyStart);
  return css.slice(bodyStart + 1, bodyEnd);
}
