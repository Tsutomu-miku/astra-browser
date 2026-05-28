import { readFileSync } from "node:fs";
import { join } from "node:path";
import { describe, expect, it } from "vitest";

const packageJson = JSON.parse(readFileSync(join(process.cwd(), "package.json"), "utf8"));

describe("package configuration", () => {
  it("builds macOS x64 and arm64 as separate artifacts", () => {
    expect(packageJson.scripts["package:mac:x64"]).toContain("--x64");
    expect(packageJson.scripts["package:mac:x64"]).not.toContain("--arm64");
    expect(packageJson.scripts["package:mac:arm64"]).toContain("--arm64");
    expect(packageJson.scripts["package:mac:arm64"]).not.toContain("--x64");
    expect(packageJson.scripts["package:all"]).toContain("electron-builder --mac dmg zip --x64");
    expect(packageJson.scripts["package:all"]).toContain("electron-builder --mac dmg zip --arm64");
    expect(packageJson.scripts["package:all"]).not.toContain("electron-builder --mac --win --linux");
    expect(packageJson.build.mac.artifactName).toContain("${arch}");
  });

  it("cleans unpacked package staging output after packaging", () => {
    expect(packageJson.scripts["clean:package-output"]).toBe("node scripts/clean-package-output.mjs");

    for (const scriptName of [
      "package",
      "package:all",
      "package:mac",
      "package:mac:arm64",
      "package:mac:x64",
      "package:mac:all",
      "package:linux",
      "package:win"
    ]) {
      expect(packageJson.scripts[scriptName]).toContain("pnpm clean:package-output");
    }
  });

  it("keeps publish disabled for local package scripts", () => {
    for (const scriptName of [
      "package",
      "package:all",
      "package:mac",
      "package:mac:arm64",
      "package:mac:x64",
      "package:mac:all",
      "package:linux",
      "package:win"
    ]) {
      expect(packageJson.scripts[scriptName]).toContain("--publish never");
    }
  });
});
