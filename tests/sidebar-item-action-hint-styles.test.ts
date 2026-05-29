import { readFileSync } from "node:fs";
import { join } from "node:path";
import { describe, expect, it } from "vitest";

const sidebarCss = readFileSync(join(__dirname, "../src/renderer/styles/sidebar-action-hints.css"), "utf8");
const sidebarLayoutCss = readFileSync(join(__dirname, "../src/renderer/styles/sidebar.css"), "utf8");

describe("sidebar item action hint styles", () => {
  it("reveals row action hints from stable in-flow slots on hover and keyboard focus", () => {
    expect(sidebarCss).not.toContain("position: absolute");
    expect(sidebarCss).toContain("width: 46px");
    expect(sidebarCss).toContain("min-width: 46px");
    expect(sidebarCss).not.toContain("max-width");
    expect(sidebarCss).toContain(".tab-row:hover .sidebar-item-action-hints");
    expect(sidebarCss).toContain(".tab-row:focus-within .sidebar-item-action-hints");
    expect(sidebarCss).toContain(".favorite-button:hover .sidebar-item-action-hints");
    expect(sidebarCss).toContain(".favorite-button:focus-visible .sidebar-item-action-hints");
    expect(sidebarCss).toContain(".closed-tab-button:hover .sidebar-item-action-hints");
    expect(sidebarCss).toContain(".closed-tab-button:focus-visible .sidebar-item-action-hints");
  });

  it("reveals tab close controls on keyboard row focus", () => {
    expect(sidebarLayoutCss).toContain(".tab-row:focus-within .tab-close");
    expect(sidebarLayoutCss).toContain(".tab-close:focus-visible");
  });
});
