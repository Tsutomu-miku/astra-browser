import { readFileSync } from "node:fs";
import { join } from "node:path";

import { describe, expect, it } from "vitest";

const sidebarCss = readFileSync(join(__dirname, "../src/renderer/styles/sidebar.css"), "utf8");

describe("sidebar compact focus styles", () => {
  it("reveals collapsed sidebar content on keyboard focus", () => {
    expect(sidebarCss).toContain(".sidebar.is-collapsed:focus-within");
    expect(sidebarCss).toContain(".sidebar.is-collapsed:focus-within .tab-stack");
  });

  it("keeps sidebar sections in one scrollable area above the footer", () => {
    expect(sidebarCss).toContain("height: 100vh");
    expect(sidebarCss).toContain("overflow: hidden");
    expect(sidebarCss).toContain("grid-template-rows: 58px auto minmax(0, 1fr)");
    expect(sidebarCss).toContain(".sidebar-scroll-area");
    expect(sidebarCss).toContain("overflow-y: auto");
    expect(sidebarCss).toContain("overflow-x: hidden");
    expect(sidebarCss).toContain("scrollbar-width: thin");
  });
});
