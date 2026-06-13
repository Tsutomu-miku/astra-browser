// LEGACY SCRIPT — Electron era
//
// This script checks the legacy Electron / Vite frontend source tree (src/).
// It is preserved for reference and for the remaining Electron build pipeline.
//
// DO NOT extend this script with new architecture checks.
// New Astra / direct-Chromium architecture checks belong in check-architecture.mjs.
//
// Per AGENTS.md: "Legacy src/ is migration reference only.
// Do not add new architecture there."
//
// What this script does (legacy):
//   - Verifies required Electron/Vite files exist
//   - Checks that source files contain no bare TODO comments
//   - Syntax-checks JS files with node --check
//   - Verifies HTML element ids referenced from React actually exist

import { readFileSync, readdirSync, statSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { join } from "node:path";
import { spawnSync } from "node:child_process";

const root = fileURLToPath(new URL("..", import.meta.url));

// --- Legacy Electron source files that must exist ---------------------------
const requiredFiles = [
  "package.json",
  "tsconfig.json",
  "vite.config.ts",
  "src/main/main.js",
  "src/main/preload.js",
  "src/renderer/index.html",
  "src/renderer/main.tsx",
  "src/renderer/app/App.tsx",
  "src/renderer/domain/browser/index.ts",
  "src/renderer/styles.css",
  "docs/CHROMIUM_DIRECT_REFACTOR_PLAN.md",
  "docs/ENGINEERING_STANDARDS.md",
  "docs/adr/0009-direct-chromium-architecture.md"
];

const missing = requiredFiles.filter((file) => {
  try {
    statSync(join(root, file));
    return false;
  } catch {
    return true;
  }
});

if (missing.length > 0) {
  throw new Error(`Missing required files: ${missing.join(", ")}`);
}

// --- Walk the legacy src/ directory ----------------------------------------
function walk(dir) {
  return readdirSync(dir).flatMap((entry) => {
    const path = join(dir, entry);
    const stat = statSync(path);
    return stat.isDirectory() ? walk(path) : [path];
  });
}

if (!statSync(join(root, "src")).isDirectory()) {
  console.log("Legacy src/ not found — skipping legacy source checks.");
  process.exit(0);
}

const sourceFiles = walk(join(root, "src")).filter((file) =>
  /\.(html|css|js|mjs|ts|tsx)$/.test(file)
);
const jsFiles = sourceFiles.filter((file) => /\.(js|mjs)$/.test(file));

// --- Check for bare TODOs ---------------------------------------------------
// Note: in the Electron src tree, TODOs are not required to have an owner tag.
// This check flags ALL TODOs — they should be resolved before migrating to
// the direct-Chromium architecture.
for (const file of sourceFiles) {
  const contents = readFileSync(file, "utf8");
  if (contents.includes("TODO")) {
    throw new Error(`Unexpected TODO in ${file}`);
  }
}

// --- JS syntax check --------------------------------------------------------
for (const file of jsFiles) {
  const result = spawnSync(process.execPath, ["--check", file], {
    encoding: "utf8"
  });

  if (result.status !== 0) {
    throw new Error(`Syntax check failed for ${file}\n${result.stderr}`);
  }
}

// --- HTML <-> React id consistency ------------------------------------------
// Verifies that document.querySelector("#...") calls in App.tsx reference
// ids that actually exist in index.html.
const renderer = readFileSync(join(root, "src/renderer/app/App.tsx"), "utf8");
const html = readFileSync(join(root, "src/renderer/index.html"), "utf8");
const referencedIds = [...renderer.matchAll(/document\.querySelector\("#([^"]+)"\)/g)]
  .map((match) => match[1])
  .sort();
const htmlIds = new Set([...html.matchAll(/\sid="([^"]+)"/g)].map((match) => match[1]));
const missingIds = referencedIds.filter((id) => !htmlIds.has(id));

if (missingIds.length > 0) {
  throw new Error(`Renderer references missing HTML ids: ${missingIds.join(", ")}`);
}

console.log(`Checked ${sourceFiles.length} legacy source files (Electron era).`);
