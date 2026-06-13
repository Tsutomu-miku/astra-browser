// ============================================================================
// BUILD.gn Linter
// ============================================================================
// Focused GN file linter for Astra BUILD.gn files.
//
// Checks performed:
//   1. sources arrays are sorted alphabetically
//   2. deps arrays are sorted (by path depth, then alphabetically)
//   3. public_deps comes before deps in each target
//   4. visibility is always specified on every target
//   5. testonly = true targets follow correct naming convention
//   6. public_deps arrays are sorted
//
// This is a best-effort regex-based linter.  It handles the common
// patterns used in Astra BUILD.gn files but may not catch every
// GN construct (conditionals, template expansions, etc.).
// ============================================================================

import { readdirSync, readFileSync, statSync, existsSync } from "node:fs";
import { join, relative, basename, dirname } from "node:path";
import { fileURLToPath } from "node:url";

const root = fileURLToPath(new URL("..", import.meta.url));
const astraDir = join(root, "chromium", "astra");

// ============================================================================
// Helpers
// ============================================================================

const errors = [];
const warnings = [];

function assert(condition, message) {
  if (!condition) {
    errors.push(message);
  }
}

function warn(condition, message) {
  if (!condition) {
    warnings.push(message);
  }
}

function walk(dir) {
  const fullDir = join(root, dir);
  if (!existsSync(fullDir)) {
    return [];
  }
  return readdirSync(fullDir).flatMap((entry) => {
    const fullPath = join(fullDir, entry);
    const stat = statSync(fullPath);
    return stat.isDirectory()
      ? walk(relative(root, fullPath))
      : [relative(root, fullPath)];
  });
}

// Extract entries from a GN list like `[ "a", "b", "c" ]`.
// Returns an array of { value, line } objects.
function extractListEntries(listBlock) {
  const entries = [];
  // Match quoted strings on their own lines (with optional trailing comma).
  const entryPattern = /^\s*"([^"]+)"\s*,?\s*$/gm;
  let match;
  while ((match = entryPattern.exec(listBlock)) !== null) {
    // Calculate line number by counting newlines before this match.
    const beforeMatch = listBlock.substring(0, match.index);
    const lineNum = (beforeMatch.match(/\n/g) || []).length + 1;
    entries.push({ value: match[1], line: lineNum });
  }
  return entries;
}

// Check if an array is sorted by the given comparator.
// Returns the first out-of-order pair or null.
function firstUnsortedPair(arr, compareFn) {
  for (let i = 1; i < arr.length; i++) {
    if (compareFn(arr[i - 1].value, arr[i].value) > 0) {
      return [arr[i - 1], arr[i]];
    }
  }
  return null;
}

// Alphabetical comparison (Chromium style: case-sensitive, ASCIIbetical).
function alphaCompare(a, b) {
  return a < b ? -1 : a > b ? 1 : 0;
}

// Depth-first comparison for deps: shallower paths come first,
// then alphabetical within the same depth.
function depthCompare(a, b) {
  // Count path separators to determine depth.
  // "//base" has depth 1, "//chrome/browser" has depth 2, etc.
  const depthA = a.split("/").filter((s) => s.length > 0).length;
  const depthB = b.split("/").filter((s) => s.length > 0).length;
  if (depthA !== depthB) return depthA - depthB;
  return alphaCompare(a, b);
}

// Extract all targets from a BUILD.gn file.
// Returns array of { name, type, body, startLine }.
function extractTargets(contents) {
  const targets = [];
  // Match target definitions like:
  //   source_set("name") {
  //   group("name") {
  //   component("name") {
  const targetPattern = /(source_set|group|component|static_library|shared_library|config)\("([^"]+)"\)\s*\{/g;
  let match;
  while ((match = targetPattern.exec(contents)) !== null) {
    const type = match[1];
    const name = match[2];
    const startIndex = match.index + match[0].length;

    // Find the matching closing brace by counting braces.
    let depth = 1;
    let i = startIndex;
    while (i < contents.length && depth > 0) {
      if (contents[i] === "{") depth++;
      else if (contents[i] === "}") depth--;
      i++;
    }

    const body = contents.substring(startIndex, i - 1);
    const startLine = (contents.substring(0, match.index).match(/\n/g) || []).length + 1;

    targets.push({ name, type, body, startLine });
  }
  return targets;
}

// Get a named list (sources, deps, public_deps, visibility) from a target body.
function getList(body, listName) {
  const pattern = new RegExp(
    `(?:^|\\n)\\s*${listName}\\s*=\\s*\\[([\\s\\S]*?)\\]`,
    "m"
  );
  const match = body.match(pattern);
  if (!match) return null;
  return match[1]; // The content inside [ ... ]
}

// Check if one section appears before another in a target body.
function sectionAppearsBefore(body, earlier, later) {
  const earlierMatch = body.match(new RegExp(`(?:^|\\n)\\s*${earlier}\\s*=\\s*`));
  const laterMatch = body.match(new RegExp(`(?:^|\\n)\\s*${later}\\s*=\\s*`));
  if (!earlierMatch || !laterMatch) return true; // both must exist for ordering to matter
  return earlierMatch.index < laterMatch.index;
}

// ============================================================================
// Check 1: sources arrays are sorted alphabetically
// ============================================================================

function checkSourcesSorted(buildGnFile, targets) {
  for (const target of targets) {
    const sourcesList = getList(target.body, "sources");
    if (!sourcesList) continue;

    const entries = extractListEntries(sourcesList);
    // Only check if there are 2+ entries and they all look like file paths.
    if (entries.length < 2) continue;

    const pair = firstUnsortedPair(entries, alphaCompare);
    if (pair) {
      assert(false,
             `${buildGnFile}:${target.startLine + pair[1].line - 1} ` +
             `target '${target.name}': sources not sorted — ` +
             `'${pair[0].value}' should come after '${pair[1].value}'`);
    }
  }
}

// ============================================================================
// Check 2: deps arrays are sorted (by depth, then alphabetically)
// ============================================================================

function checkDepsSorted(buildGnFile, targets) {
  for (const target of targets) {
    const depsList = getList(target.body, "deps");
    if (!depsList) continue;

    const entries = extractListEntries(depsList);
    if (entries.length < 2) continue;

    const pair = firstUnsortedPair(entries, depthCompare);
    if (pair) {
      warn(false,
           `${buildGnFile}:${target.startLine + pair[1].line - 1} ` +
           `target '${target.name}': deps not sorted by depth/alpha — ` +
           `'${pair[0].value}' should come after '${pair[1].value}'`);
    }
  }
}

// ============================================================================
// Check 3: public_deps arrays are sorted
// ============================================================================

function checkPublicDepsSorted(buildGnFile, targets) {
  for (const target of targets) {
    const depsList = getList(target.body, "public_deps");
    if (!depsList) continue;

    const entries = extractListEntries(depsList);
    if (entries.length < 2) continue;

    const pair = firstUnsortedPair(entries, depthCompare);
    if (pair) {
      warn(false,
           `${buildGnFile}:${target.startLine + pair[1].line - 1} ` +
           `target '${target.name}': public_deps not sorted by depth/alpha — ` +
           `'${pair[0].value}' should come after '${pair[1].value}'`);
    }
  }
}

// ============================================================================
// Check 4: public_deps comes before deps
// ============================================================================

function checkPublicDepsBeforeDeps(buildGnFile, targets) {
  for (const target of targets) {
    const hasPublicDeps = /(?:^|\n)\s*public_deps\s*=\s*\[/.test(target.body);
    const hasDeps = /(?:^|\n)\s*deps\s*=\s*\[/.test(target.body);

    if (hasPublicDeps && hasDeps) {
      assert(sectionAppearsBefore(target.body, "public_deps", "deps"),
             `${buildGnFile}: target '${target.name}': ` +
             `public_deps must come before deps`);
    }
  }
}

// ============================================================================
// Check 5: visibility is always specified
// ============================================================================

function checkVisibilitySpecified(buildGnFile, targets) {
  for (const target of targets) {
    // config targets and test-only targets may not need visibility,
    // but source_set, group, and component should have it.
    if (target.type === "config") continue;

    const hasVisibility = /(?:^|\n)\s*visibility\s*=\s*\[/.test(target.body);
    const isTestOnly = /testonly\s*=\s*true/.test(target.body);

    // Test-only targets in subdirectories may have narrower visibility,
    // but they should still specify it.
    if (!hasVisibility) {
      warn(false,
           `${buildGnFile}: target '${target.name}' (${target.type}): ` +
           `visibility is not specified — ` +
           `consider adding visibility = [ "//astra/*" ] or similar`);
    }
  }
}

// ============================================================================
// Check 6: testonly = true targets have correct naming convention
// ============================================================================

function checkTestTargetNaming(buildGnFile, targets) {
  for (const target of targets) {
    const isTestOnly = /testonly\s*=\s*true/.test(target.body);
    if (!isTestOnly) continue;

    // Test targets should have "test" in their name.
    const nameHasTest = /test/i.test(target.name);
    assert(nameHasTest,
           `${buildGnFile}: target '${target.name}' has testonly = true ` +
           `but name does not contain 'test'`);

    // Test targets should follow Chromium naming:
    //   *_unittests  — for unit test targets
    //   *_browsertests — for browser test targets
    //   *_tests      — general test aggregate targets
    // This is a warning, not an error, since naming varies.
    const hasStandardSuffix =
      target.name.endsWith("_unittests") ||
      target.name.endsWith("_browsertests") ||
      target.name.endsWith("_tests") ||
      target.name.endsWith("_test_support");
    warn(hasStandardSuffix,
         `${buildGnFile}: test target '${target.name}' does not follow ` +
         `Chromium naming convention (*_unittests, *_browsertests, *_tests)`);
  }
}

// ============================================================================
// Main
// ============================================================================

function main() {
  if (!existsSync(astraDir)) {
    console.error("chromium/astra/ directory not found.");
    process.exit(1);
  }

  const astraFiles = walk("chromium/astra");
  const buildGnFiles = astraFiles.filter((f) => basename(f) === "BUILD.gn");

  console.log(`Linting ${buildGnFiles.length} BUILD.gn file(s)...`);
  console.log("");

  for (const gnFile of buildGnFiles) {
    const contents = readFileSync(join(root, gnFile), "utf8");
    const targets = extractTargets(contents);

    checkSourcesSorted(gnFile, targets);
    checkDepsSorted(gnFile, targets);
    checkPublicDepsSorted(gnFile, targets);
    checkPublicDepsBeforeDeps(gnFile, targets);
    checkVisibilitySpecified(gnFile, targets);
    checkTestTargetNaming(gnFile, targets);
  }

  // Results
  if (warnings.length > 0) {
    console.warn("BUILD.gn lint warnings:");
    for (const w of warnings) {
      console.warn(`  ! ${w}`);
    }
    console.warn("");
  }

  if (errors.length > 0) {
    console.error("BUILD.gn lint FAILED:");
    for (const error of errors) {
      console.error(`  ✗ ${error}`);
    }
    console.error("");
    console.error(`${errors.length} error(s), ${warnings.length} warning(s).`);
    process.exit(1);
  }

  console.log(`BUILD.gn lint passed.`);
  console.log(`  Files checked: ${buildGnFiles.length}`);
  console.log(`  Rules enforced: 6`);
  console.log(`  Warnings: ${warnings.length}`);
}

main();
