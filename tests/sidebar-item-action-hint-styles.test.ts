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

  it("keeps action hints as quiet inline glyphs instead of keycaps", () => {
    const hintBlock = getRuleBlock(sidebarCss, "\n.sidebar-item-action-hint {");

    expect(hintBlock).toContain("width: 16px");
    expect(hintBlock).toContain("height: 16px");
    expect(hintBlock).toContain("border: 0");
    expect(hintBlock).toContain("background: transparent");
    expect(hintBlock).not.toContain("border-radius");
    expect(hintBlock).not.toContain("var(--accent)");
  });

  it("reveals tab close controls on keyboard row focus", () => {
    expect(sidebarLayoutCss).toContain(".tab-row:focus-within .tab-close");
    expect(sidebarLayoutCss).toContain(".tab-close:focus-visible");
  });
});

function getRuleBlock(css: string, selector: string): string {
  const start = css.indexOf(selector);
  expect(start).toBeGreaterThanOrEqual(0);
  const bodyStart = css.indexOf("{", start);
  const bodyEnd = css.indexOf("}", bodyStart);
  return css.slice(bodyStart + 1, bodyEnd);
}
