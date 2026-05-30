import { readFileSync } from "node:fs";
import { join } from "node:path";
import { describe, expect, it } from "vitest";

const startSurfaceFiles = [
  "../src/renderer/surfaces/start/StartPage.tsx",
  "../src/renderer/surfaces/start/components/StartSearch.tsx",
  "../src/renderer/surfaces/start/components/StartTileGrid.tsx"
];

describe("start surface native tooltips", () => {
  it("keeps start entries and suggestions free of native title tooltips", () => {
    for (const file of startSurfaceFiles) {
      const source = readFileSync(join(__dirname, file), "utf8");
      expect(source).not.toContain("title=");
    }
  });
});
