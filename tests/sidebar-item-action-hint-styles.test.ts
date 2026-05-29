import { readFileSync } from "node:fs";
import { join } from "node:path";
import { describe, expect, it } from "vitest";

const sidebarCss = readFileSync(join(__dirname, "../src/renderer/styles/sidebar-action-hints.css"), "utf8");

describe("sidebar item action hint styles", () => {
  it("reveals row action hints in flow on hover and keyboard focus", () => {
    expect(sidebarCss).not.toContain("position: absolute");
    expect(sidebarCss).toContain("max-width: 0");
    expect(sidebarCss).toContain("max-width: 46px");
    expect(sidebarCss).toContain(".tab-row:hover .sidebar-item-action-hints");
    expect(sidebarCss).toContain(".tab-row:focus-within .sidebar-item-action-hints");
    expect(sidebarCss).toContain(".favorite-button:hover .sidebar-item-action-hints");
    expect(sidebarCss).toContain(".favorite-button:focus-visible .sidebar-item-action-hints");
    expect(sidebarCss).toContain(".closed-tab-button:hover .sidebar-item-action-hints");
    expect(sidebarCss).toContain(".closed-tab-button:focus-visible .sidebar-item-action-hints");
  });
});
