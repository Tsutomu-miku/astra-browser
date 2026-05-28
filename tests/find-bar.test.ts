import { createElement } from "react";
import { renderToStaticMarkup } from "react-dom/server";
import { readFileSync } from "node:fs";
import { join } from "node:path";
import { describe, expect, it, vi } from "vitest";

import type { BrowserController } from "../src/renderer/app/controller/types";
import { FindBar } from "../src/renderer/surfaces/find/FindBar";
import { getFindStatusLabel } from "../src/renderer/surfaces/find/findStatus";

const browserCss = readFileSync(join(__dirname, "../src/renderer/styles/browser.css"), "utf8");

describe("find bar", () => {
  it("formats Chromium find result status", () => {
    expect(getFindStatusLabel("", null)).toBe("Ready");
    expect(getFindStatusLabel("docs", null)).toBe("Searching");
    expect(getFindStatusLabel("docs", { activeMatchOrdinal: 2, finalUpdate: true, matches: 5 })).toBe("2 / 5");
    expect(getFindStatusLabel("missing", { activeMatchOrdinal: 0, finalUpdate: true, matches: 0 })).toBe("No matches");
  });

  it("renders match status inside the find surface", () => {
    const html = renderToStaticMarkup(createElement(FindBar, {
      controller: createController({
        activeMatchOrdinal: 3,
        finalUpdate: true,
        matches: 8
      })
    }));

    expect(html).toContain('aria-label="Find in page"');
    expect(html).toContain('aria-live="polite"');
    expect(html).toContain("3 / 8");
  });

  it("styles match counts and no-match feedback", () => {
    expect(browserCss).toContain(".find-status");
    expect(browserCss).toContain(".find-status.is-empty");
    expect(browserCss).toContain("minmax(58px, auto)");
  });
});

function createController(findResult: BrowserController["findResult"]): BrowserController {
  const noop = vi.fn();

  return {
    actions: {
      closeFind: noop,
      findInPage: noop
    },
    findQuery: "docs",
    findResult
  } as unknown as BrowserController;
}
