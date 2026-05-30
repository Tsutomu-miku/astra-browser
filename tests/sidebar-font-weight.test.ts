import { readFileSync } from "node:fs";
import { join } from "node:path";
import { describe, expect, it } from "vitest";

const sidebarCss = [
  "sidebar.css",
  "sidebar-action-hints.css",
  "sidebar-context-menu.css",
  "sidebar-groups.css",
  "sidebar-search.css",
  "sidebar-workspaces.css"
].map((file) => readFileSync(join(__dirname, `../src/renderer/styles/${file}`), "utf8")).join("\n");

describe("sidebar font weight", () => {
  it("keeps sidebar typography restrained", () => {
    expect(sidebarCss).not.toMatch(/font-weight:\s*(6|7|8|9)\d{2}/);
  });
});
