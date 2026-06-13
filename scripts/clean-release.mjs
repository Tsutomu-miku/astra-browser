// LEGACY SCRIPT — Electron era
//
// Cleans and recreates the release/ output directory used by electron-builder.
//
// This is part of the legacy Electron build pipeline.
// The direct-Chromium build uses ninja out/ directories instead.

import { mkdirSync, rmSync } from "node:fs";
import { join } from "node:path";
import { fileURLToPath } from "node:url";

const root = fileURLToPath(new URL("..", import.meta.url));
const releaseDir = join(root, "release");

rmSync(releaseDir, { force: true, recursive: true });
mkdirSync(releaseDir, { recursive: true });

console.log("Cleaned release artifacts.");
