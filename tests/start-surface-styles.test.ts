import { readFileSync } from "node:fs";
import { join } from "node:path";
import { describe, expect, it } from "vitest";

const startCss = readFileSync(join(__dirname, "../src/renderer/styles/start-page.css"), "utf8");

describe("start surface styles", () => {
  it("keeps new tab typography restrained", () => {
    expect(startCss).not.toMatch(/font-weight:\s*(6|7|8|9)\d{2}/);
  });

  it("uses quiet fill-based hover states for new tab entries", () => {
    const hoverBlock = getRuleBlock(startCss, ".start-tile:hover,\n.start-tile:focus-visible,\n.start-history-item:hover,\n.start-history-item:focus-visible");

    expect(hoverBlock).toContain("border-color: transparent");
    expect(hoverBlock).toContain("background: var(--panel-soft)");
    expect(hoverBlock).not.toContain("var(--start-accent)");
  });

  it("applies accent-based highlight to active search suggestions", () => {
    const shellBlock = getRuleBlock(startCss, ".start-page-shell");
    const dotBlock = getRuleBlock(startCss, ".start-space-dot");
    const suggestionBlock = getRuleBlock(startCss, ".start-search-suggestion[aria-selected=\"true\"],\n.start-search-suggestion:hover");
    const activeBlock = getLastRuleBlock(startCss, '\n.start-search-suggestion[aria-selected="true"]');

    expect(shellBlock).toContain("var(--start-accent) 9%");
    expect(dotBlock).toContain("var(--start-accent) 12%");
    expect(suggestionBlock).toContain("background: var(--theme-accent-highlight)");
    expect(activeBlock).toContain("var(--start-accent)");
    expect(activeBlock).toContain("box-shadow:");
  });

  it("uses compact glyph action hints in start search suggestions", () => {
    const hintBlock = getRuleBlock(startCss, "\n.start-search-action-hint {");

    expect(hintBlock).toContain("inline-grid");
    expect(startCss).not.toContain(".start-search-action-hint kbd");
  });
});

function getRuleBlock(css: string, selector: string): string {
  const start = css.indexOf(selector);
  expect(start).toBeGreaterThanOrEqual(0);
  const bodyStart = css.indexOf("{", start);
  const bodyEnd = css.indexOf("}", bodyStart);
  return css.slice(bodyStart + 1, bodyEnd);
}

function getLastRuleBlock(css: string, selector: string): string {
  const start = css.lastIndexOf(selector);
  expect(start).toBeGreaterThanOrEqual(0);
  const bodyStart = css.indexOf("{", start);
  const bodyEnd = css.indexOf("}", bodyStart);
  return css.slice(bodyStart + 1, bodyEnd);
}
