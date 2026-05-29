import { readFileSync } from "node:fs";
import { join } from "node:path";
import { describe, expect, it } from "vitest";

const sidebarCss = readFileSync(join(__dirname, "../src/renderer/styles/sidebar.css"), "utf8");
const workspaceCss = readFileSync(join(__dirname, "../src/renderer/styles/sidebar-workspaces.css"), "utf8");
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

  it("keeps workspace active state quiet and removes accent wash from the app background", () => {
    const activeWorkspaceBlock = getRuleBlock(workspaceCss, ".workspace-button[aria-current=\"true\"]");

    expect(activeWorkspaceBlock).toContain("border-color: transparent");
    expect(activeWorkspaceBlock).toContain("box-shadow: none");
    expect(activeWorkspaceBlock).not.toContain("var(--accent)");
    expect(baseCss).not.toContain("radial-gradient");
  });
});

function getRuleBlock(css: string, selector: string): string {
  const start = css.indexOf(selector);
  expect(start).toBeGreaterThanOrEqual(0);
  const bodyStart = css.indexOf("{", start);
  const bodyEnd = css.indexOf("}", bodyStart);
  return css.slice(bodyStart + 1, bodyEnd);
}
