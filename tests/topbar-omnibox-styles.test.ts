import { readFileSync } from "node:fs";
import { join } from "node:path";

import { describe, expect, it } from "vitest";

const topbarCss = readFileSync(join(__dirname, "../src/renderer/styles/topbar.css"), "utf8");

describe("topbar omnibox styles", () => {
  it("keeps suggestion hints quiet and glyph-only", () => {
    const hintBlock = getRuleBlock(topbarCss, "\n.omnibox-action-hint {");

    expect(hintBlock).toContain("width: 17px");
    expect(hintBlock).toContain("height: 17px");
    expect(hintBlock).not.toContain("border:");
    expect(topbarCss).not.toContain("kbd");
  });

  it("uses neutral selected suggestions without accent stripes or heavy titles", () => {
    const selectedBlock = getRuleBlock(topbarCss, '.omnibox-suggestion[aria-selected="true"]');
    const titleBlock = getRuleBlock(topbarCss, ".suggestion-title");

    expect(selectedBlock).toContain("box-shadow: none");
    expect(selectedBlock).not.toContain("var(--accent)");
    expect(titleBlock).toContain("font-weight: 500");
  });
});

function getRuleBlock(css: string, selector: string): string {
  const start = css.indexOf(selector);
  expect(start).toBeGreaterThanOrEqual(0);
  const open = css.indexOf("{", start);
  const close = css.indexOf("}", open);
  return css.slice(open + 1, close);
}
