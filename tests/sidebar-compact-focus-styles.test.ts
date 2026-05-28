import { readFileSync } from "node:fs";
import { join } from "node:path";

import { describe, expect, it } from "vitest";

const sidebarCss = readFileSync(join(__dirname, "../src/renderer/styles/sidebar.css"), "utf8");

describe("sidebar compact focus styles", () => {
  it("reveals collapsed sidebar content on keyboard focus", () => {
    expect(sidebarCss).toContain(".sidebar.is-collapsed:focus-within");
    expect(sidebarCss).toContain(".sidebar.is-collapsed:focus-within .tab-stack");
    expect(sidebarCss).toContain(".sidebar.is-collapsed:focus-within .sidebar-footer");
  });
});
