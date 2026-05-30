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
    const weights = [...sidebarCss.matchAll(/font-weight:\s*(\d+)/g)].map((match) => Number(match[1]));

    expect(weights.length).toBeGreaterThan(0);
    expect(Math.max(...weights)).toBeLessThanOrEqual(500);
    expect(sidebarCss).not.toMatch(/font-weight:\s*(bold|bolder|[6-9]\d{2})/);
  });
});
