// ============================================================================
// Astra Architecture Guard
// ============================================================================
// Ensures the direct Chromium architecture is followed.
// See AGENTS.md for the full architecture contract.
//
// Checks performed:
//   1. Forbidden legacy paths do not exist (Electron/CEF/CMake residue)
//   2. Required active architecture files exist
//   3. Active documentation contains no stale terminology
//   4. Astra source files contain no forbidden runtime patterns
//      (CEF, Electron, AppKit, CMake, libcef, electron-builder)
//   5. Astra source does not redefine Chromium-owned service classes
//   6. All class/struct definitions in Astra source use the Astra prefix
//   7. All .cc / .h source files use the astra_ filename prefix
//   8. All .h files have proper Chromium-style header guards
//   9. All TODO comments carry a TODO(astra) owner tag
//  10. Legacy src/ directory contains no new architecture files
//  11. BUILD.gn source lists are complete (all .h/.cc files on disk
//      are referenced in sources = [...])
//  12. Dependency direction: lower layers never include from higher layers
//      (e.g. common/ must not include from browser/ or ui/)
//  13. Public API surface (//astra:public) only exposes headers/buildflags,
//      not implementation details
//  14. Test file naming: test files end with _unittest.cc / _browsertest.cc
//      and have matching test targets in BUILD.gn
// ============================================================================

import { existsSync, readdirSync, readFileSync, statSync } from "node:fs";
import { join, relative, basename, dirname } from "node:path";
import { fileURLToPath } from "node:url";

const root = fileURLToPath(new URL("..", import.meta.url));
const astraDir = join(root, "chromium", "astra");

// ============================================================================
// Configuration
// ============================================================================

// Paths that must not exist — legacy Electron / CEF / CMake architecture.
const forbiddenPaths = [
  "CMakeLists.txt",
  "browser/chromium",
  "browser/core",
  "native",
  "scripts/build-native.sh",
  "scripts/setup-cef.sh"
];

// Files that must exist for the active direct-Chromium architecture.
const activeRequiredFiles = [
  "AGENTS.md",
  "README.md",
  "docs/ARCHITECTURE.md",
  "docs/CODE_STRUCTURE.md",
  "docs/ENGINEERING_STANDARDS.md",
  "docs/ROADMAP.md",
  "docs/CHROMIUM_DIRECT_REFACTOR_PLAN.md",
  "docs/adr/0009-direct-chromium-architecture.md",
  "chromium/astra/BUILD.gn"
];

// Doc files scanned for stale terminology.
const activeDocFiles = [
  "README.md",
  "AGENTS.md",
  "docs/ARCHITECTURE.md",
  "docs/CODE_STRUCTURE.md",
  "docs/ENGINEERING_STANDARDS.md",
  "docs/ROADMAP.md",
  "docs/CHROMIUM_DIRECT_REFACTOR_PLAN.md",
  "docs/adr/README.md",
  "docs/adr/0009-direct-chromium-architecture.md"
];

// Phrases that must not appear in active architecture docs.
// Each represents a legacy architecture concept that has been superseded.
const activeDocForbiddenPhrases = [
  "Electron is acceptable",
  "Chromium 原生 CEF",
  "CEF 架构",
  "CEF 工程",
  "setup-cef",
  "build-native",
  "electron-builder artifacts"
];

// Forbidden runtime patterns in Astra source files (.cc, .h, .gn, .gni, .md).
// These indicate the code is trying to use a non-Chromium runtime framework.
const astraSourceForbidden = [
  /\bCEF\b/i,
  /\bCef[A-Z_a-z0-9]*/,
  /\bElectron\b/i,
  /\bAppKit\b/,
  /\bCMake\b/i,
  /\blibcef\b/i,
  /\belectron-builder\b/i
];

// Chromium-owned service class names.  Astra must not redefine these.
// These correspond to subsystems listed in AGENTS.md as "Never reimplement".
const duplicateChromiumServiceNames = [
  "ProfileManager",
  "DownloadManager",
  "DownloadService",
  "PermissionManager",
  "PermissionService",
  "HistoryService",
  "ExtensionService",
  "ExtensionSystem",
  "PasswordManager",
  "PasswordService",
  "AutofillService",
  "AutofillManager",
  "SafeBrowsingService",
  "SafeBrowsingDatabaseManager",
  "DevToolsManager",
  "DevToolsService",
  "NavigationController",
  "TabStripModel",
  "WebContentsImpl",
  "UpdateService",
  "PolicyService"
];

// File extensions checked for Astra source rules.
const astraSourceExtensions = /\.(cc|h|gn|gni|mm)$/;

// File extensions checked for source-level naming rules (class prefix, etc.).
const astraCodeExtensions = /\.(cc|h|mm)$/;

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

function read(path) {
  return readFileSync(join(root, path), "utf8");
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

// Walk a directory and return paths relative to that directory.
function walkRel(baseDir) {
  const fullBase = join(root, baseDir);
  if (!existsSync(fullBase)) return [];

  function walkInner(dir) {
    const fullDir = join(fullBase, dir);
    return readdirSync(fullDir).flatMap((entry) => {
      const fullPath = join(fullDir, entry);
      const relPath = join(dir, entry);
      const stat = statSync(fullPath);
      return stat.isDirectory() ? walkInner(relPath) : [relPath];
    });
  }
  return walkInner("");
}

// Compute the expected Chromium-style header guard for a header file
// relative to chromium/astra/.  The guard follows Chromium convention:
// full path from //src root → uppercase → non-alnum → underscore → _H_ suffix.
// Since the Astra overlay lives at //astra in the Chromium tree, the path
// is prefixed with "astra/".
function expectedHeaderGuard(relPathFromAstra) {
  const fullPath = "astra/" + relPathFromAstra;
  return fullPath
    .toUpperCase()
    .replace(/[^A-Z0-9]/g, "_")
    .replace(/_H$/, "_H_");
}

// ============================================================================
// Check 1: Forbidden legacy paths
// ============================================================================
// Ensures no residue of previous architectures (Electron, CEF, CMake native)
// exists in the repository.

for (const path of forbiddenPaths) {
  assert(!existsSync(join(root, path)),
         `Forbidden legacy path exists: ${path}`);
}

// ============================================================================
// Check 2: Required active architecture files
// ============================================================================
// Ensures the direct-Chromium architecture documents and entry points exist.

for (const path of activeRequiredFiles) {
  assert(existsSync(join(root, path)),
         `Missing active architecture file: ${path}`);
}

// ============================================================================
// Check 3: Active docs — no stale terminology
// ============================================================================
// Ensures architecture docs reflect the current direct-Chromium direction,
// not previous Electron or CEF approaches.

for (const path of activeDocFiles) {
  if (!existsSync(join(root, path))) continue;
  const contents = read(path);
  for (const phrase of activeDocForbiddenPhrases) {
    assert(!contents.includes(phrase),
           `Active doc ${path} contains stale phrase: ${phrase}`);
  }
}

// ============================================================================
// Check 4: Forbidden runtime patterns in Astra source
// ============================================================================
// Ensures Astra source code references only Chromium-native frameworks.
// Applies to .cc, .h, .gn, .gni, .mm, and .md files under chromium/astra/.

const astraFiles = walk("chromium/astra");

for (const file of astraFiles) {
  if (!/\.(cc|h|gn|gni|md|mm)$/.test(file)) continue;
  const contents = read(file);

  for (const pattern of astraSourceForbidden) {
    assert(!pattern.test(contents),
           `${file} references forbidden runtime pattern ${pattern}`);
  }
}

// ============================================================================
// Check 5: No Chromium-owned service redefinitions
// ============================================================================
// Ensures Astra does not redefine services that Chromium already owns.
// Only catches class definitions (with body or inheritance), not forward
// declarations — forward declarations of Chromium types are expected and fine.

for (const file of astraFiles) {
  if (!astraCodeExtensions.test(file)) continue;
  const contents = read(file);

  for (const serviceName of duplicateChromiumServiceNames) {
    const re = new RegExp(`class\\s+${serviceName}\\s*[:{]`);
    assert(!re.test(contents),
           `${file} declares Chromium-owned service class ${serviceName}`);
  }
}

// ============================================================================
// Check 6: Class / struct prefix (Astra*) — warning level
// ============================================================================
// Per AGENTS.md: "Prefix classes with Astra."
// Also applies to top-level structs.
//
// This is a warning-level check (not error) because regex-based class
// detection is inherently heuristic:
//   - Nested / inner classes (e.g. Observer, Delegate) are already scoped
//     inside an Astra class and do not need the prefix.
//   - Test fixtures and test helpers follow different naming conventions.
//   - Anonymous-namespace implementation details may use shorter names.
//   - enum class / enum struct are excluded (they are enums, not classes).
//
// The check still catches obvious violations: a new top-level class at
// namespace scope without the Astra prefix will be flagged.

for (const file of astraFiles) {
  if (!astraCodeExtensions.test(file)) continue;
  // Skip test files — test fixtures and helpers follow different
  // naming conventions (e.g. XxxTest, TestXxxObserver).
  if (/_unittest\.cc$|_browsertest\.cc$|_test\.h$/.test(file)) continue;
  const contents = read(file);

  // Match class/struct definitions with a body or inheritance.
  // Forward declarations (ending in ';') are excluded.
  // Uses negative lookbehind to exclude "enum class" / "enum struct".
  const classDefPattern = /(?<!enum\s)(?:class|struct)\s+([A-Z][a-zA-Z0-9_]*)\s*[:{]/g;

  let match;
  while ((match = classDefPattern.exec(contents)) !== null) {
    const className = match[1];
    // Skip very short class names — likely inner types like Mode, Type,
    // Edge, etc. that are already scoped inside an Astra class.
    if (className.length < 6) continue;

    warn(className.startsWith("Astra"),
         `${file} defines class/struct '${className}' without Astra prefix ` +
         `(may be an inner class — review manually)`);
  }
}

// ============================================================================
// Check 7: Source file prefix (astra_*)
// ============================================================================
// Per AGENTS.md: "Prefix source files with astra_."
// Applies to .cc, .h, and .mm files.
// BUILD.gn, .gni, and README.md files are exempt.

for (const file of astraFiles) {
  if (!astraCodeExtensions.test(file)) continue;
  const name = basename(file);
  // Skip test-only helper files that might follow different conventions.
  if (name.endsWith("_unittest.cc") || name.endsWith("_browsertest.cc")) {
    // Test files should still have the astra_ prefix.
  }
  assert(name.startsWith("astra_"),
         `${file} source file does not start with 'astra_' prefix`);
}

// ============================================================================
// Check 8: Header guards
// ============================================================================
// Ensures all .h files use Chromium-style header guards.
// Guard format: ASTRA_PATH_TO_FILE_H_ (full path from //src, uppercase).
// Checks for both #ifndef / #define pair and closing #endif comment.

for (const file of astraFiles) {
  if (!/\.h$/.test(file)) continue;

  const contents = read(file);
  const relFromAstra = relative(astraDir, join(root, file));
  const expectedGuard = expectedHeaderGuard(relFromAstra);

  // Check #ifndef guard
  const ifndefMatch = contents.match(/^#ifndef\s+(\S+)/m);
  assert(ifndefMatch !== null,
         `${file} missing #ifndef header guard`);

  if (ifndefMatch) {
    assert(ifndefMatch[1] === expectedGuard,
           `${file} header guard mismatch: expected '${expectedGuard}', got '${ifndefMatch[1]}'`);
  }

  // Check #define guard (must match #ifndef)
  const defineMatch = contents.match(/^#define\s+(\S+)/m);
  if (ifndefMatch && defineMatch) {
    assert(defineMatch[1] === ifndefMatch[1],
           `${file} #define guard does not match #ifndef`);
  }

  // Check closing #endif comment (Chromium style)
  const endifMatch = contents.match(/^#endif\s*\/\/\s*(\S+)\s*$/m);
  if (endifMatch && ifndefMatch) {
    assert(endifMatch[1] === ifndefMatch[1],
           `${file} closing #endif comment does not match guard name`);
  }
}

// ============================================================================
// Check 9: TODO ownership — TODO(astra)
// ============================================================================
// Per AGENTS.md: "TODOs must use TODO(astra): and name the Chromium owner
// or patch point."
//
// Regex notes:
//   - \bTODO\b ensures we match "TODO" as a whole word, not "TODOs",
//     "TODO_LIST", "todo" (case-sensitive), or other identifiers.
//   - (?!\s*\(astra\)) is a negative lookahead that permits optional
//     whitespace before the (astra) tag.
//   - Case-sensitive: lowercase "todo" in prose/comments is not flagged.

const todoPattern = /\bTODO\b(?!\s*\(astra\))/g;

for (const file of astraFiles) {
  if (!astraSourceExtensions.test(file) && !/\.md$/.test(file)) continue;
  const contents = read(file);

  const vagueTodos = [...contents.matchAll(todoPattern)];
  assert(vagueTodos.length === 0,
         `${file} contains ${vagueTodos.length} TODO(s) without TODO(astra) owner`);
}

// ============================================================================
// Check 10: Legacy src/ directory — no new architecture
// ============================================================================
// Per AGENTS.md: "Legacy src/ is migration reference only.
// Do not add new architecture there."
//
// We verify this by checking that src/ contains only Electron-era file types
// (JS, TS, HTML, CSS) and no C++ / GN / Chromium architecture files.

const srcFiles = existsSync(join(root, "src")) ? walk("src") : [];
const cppOrGnInSrc = srcFiles.filter((f) =>
  /\.(cc|h|gn|gni|mm|proto)$/.test(f)
);
assert(cppOrGnInSrc.length === 0,
       `Legacy src/ contains C++/GN files — new architecture ` +
       `should go in chromium/astra/: ${cppOrGnInSrc.join(", ")}`);

// ============================================================================
// Check 11: BUILD.gn source list completeness
// ============================================================================
// Cross-references files on disk against sources lists in BUILD.gn files.
// Every .cc / .h / .mm file on disk must appear in at least one BUILD.gn
// sources list, and every file listed in sources must exist on disk.
//
// Note: this is a best-effort regex-based parse.  It handles the common
// pattern of `sources = [ "file.cc", "file.h", ... ]` but may not catch
// every GN construct (e.g., generated sources, conditionally included
// files, or files in sub-targets).

(function checkBuildGnSources() {
  if (!existsSync(astraDir)) return;

  // Collect all .cc/.h files on disk (relative to chromium/astra/).
  const diskFiles = new Set(
    walkRel("chromium/astra").filter((f) => astraCodeExtensions.test(f))
  );

  // Collect all files referenced in BUILD.gn sources lists.
  const gnSourceFiles = new Set();
  const buildGnFiles = astraFiles.filter((f) => basename(f) === "BUILD.gn");

  for (const gnFile of buildGnFiles) {
    const contents = read(gnFile);
    const gnDir = dirname(gnFile); // relative to root, e.g. "chromium/astra/browser"
    const gnDirRelFromAstra = relative(astraDir, join(root, gnDir)); // e.g. "browser"

    // Extract everything inside `sources = [ ... ]` blocks.
    // Handles multi-line lists with quoted file paths.
    const sourcesBlockPattern = /sources\s*=\s*\[([\s\S]*?)\]/g;
    let blockMatch;
    while ((blockMatch = sourcesBlockPattern.exec(contents)) !== null) {
      const block = blockMatch[1];
      // Extract quoted file paths (may include subdirectories).
      const filePattern = /"([^"]+\.(?:cc|h|mm))"/g;
      let fileMatch;
      while ((fileMatch = filePattern.exec(block)) !== null) {
        const filePath = fileMatch[1];
        // Resolve relative to the BUILD.gn directory.
        const fullPath = join(gnDirRelFromAstra, filePath);
        gnSourceFiles.add(fullPath);
      }
    }
  }

  // Files on disk that are not in any BUILD.gn sources list.
  for (const file of diskFiles) {
    assert(gnSourceFiles.has(file),
           `BUILD.gn completeness: ${file} exists on disk but is not ` +
           `listed in any BUILD.gn sources list`);
  }

  // Files in BUILD.gn sources that don't exist on disk.
  for (const file of gnSourceFiles) {
    assert(diskFiles.has(file),
           `BUILD.gn completeness: ${file} is listed in BUILD.gn ` +
           `but does not exist on disk`);
  }
})();

// ============================================================================
// Check 12: Dependency direction — layered architecture
// ============================================================================
// Ensures that files in lower-level directories never include headers from
// higher-level directories.  This enforces the layered architecture:
//
//   build/  (lowest — buildflags only)
//     |
//   common/  (shared types, no business logic)
//     |
//   browser/  (product metadata and services)
//   /     \
// app/   ui/views/  (top layers — startup hooks, UI surfaces)
//
// Rules enforced:
//   - common/  must not include from browser/ or ui/
//   - browser/ must not include from ui/views/ or app/
//   - ui/views/ must not include from app/
//   - ui/color/ must not include from browser/
//   - ui/accessibility/ must not include from browser/
//
// Detection uses #include patterns like `#include "astra/browser/...`.

(function checkDependencyDirection() {
  if (!existsSync(astraDir)) return;

  // Layer definitions: directory path (relative to chromium/astra/) ->
  // list of include prefixes that this layer must NOT use.
  const layerRules = [
    {
      layer: "common",
      dir: "common",
      forbiddenIncludes: [
        '"astra/browser/',
        '"astra/ui/',
        '"astra/app/',
      ],
    },
    {
      layer: "browser",
      dir: "browser",
      forbiddenIncludes: [
        '"astra/ui/views/',
        '"astra/app/',
      ],
    },
    {
      layer: "ui/views",
      dir: "ui/views",
      forbiddenIncludes: [
        '"astra/app/',
      ],
    },
    {
      layer: "ui/color",
      dir: "ui/color",
      forbiddenIncludes: [
        '"astra/browser/',
        '"astra/app/',
        '"astra/ui/views/',
      ],
    },
    {
      layer: "ui/accessibility",
      dir: "ui/accessibility",
      forbiddenIncludes: [
        '"astra/browser/',
        '"astra/app/',
        '"astra/ui/views/',
      ],
    },
  ];

  for (const rule of layerRules) {
    const layerDir = join(astraDir, rule.dir);
    if (!existsSync(layerDir)) continue;

    const layerFiles = walk(join("chromium/astra", rule.dir));

    for (const file of layerFiles) {
      if (!astraCodeExtensions.test(file)) continue;
      const contents = read(file);

      for (const forbidden of rule.forbiddenIncludes) {
        const pattern = new RegExp(`#include\\s+${forbidden}`, "g");
        const matches = [...contents.matchAll(pattern)];
        for (const match of matches) {
          // Extract the full include line for the error message.
          const lineMatch = match[0];
          assert(false,
                 `Dependency direction: ${rule.layer}/ must not include ` +
                 `from higher layer: ${lineMatch.trim()}`);
        }
      }
    }
  }
})();

// ============================================================================
// Check 13: Public API surface
// ============================================================================
// Verifies that the //astra:public target only exposes header files and
// buildflags — never implementation details (.cc files, non-public deps).
//
// The public target is what Chromium patch points depend on.  It must be
// a narrow, stable surface: only buildflags and forward-declared types.
//
// What this check verifies:
//   - The "public" target exists in chromium/astra/BUILD.gn
//   - It uses public_deps (not regular deps) for its dependencies
//   - Its dependencies are only buildflags and other header-only targets
//   - It has a visibility restriction (not globally visible)
//   - It does not list any .cc source files directly

(function checkPublicApiSurface() {
  const rootBuildGn = join(root, "chromium", "astra", "BUILD.gn");
  if (!existsSync(rootBuildGn)) return;

  const contents = read("chromium/astra/BUILD.gn");

  // Find the "public" target definition.
  const publicTargetPattern = /group\("public"\)\s*\{([\s\S]*?)\n\}/;
  const publicMatch = contents.match(publicTargetPattern);

  assert(publicMatch !== null,
         "Public API: //astra:public target not found in chromium/astra/BUILD.gn");

  if (!publicMatch) return;

  const targetBody = publicMatch[1];

  // Must use public_deps, not deps.
  assert(!/(?:^|\n)\s*deps\s*=\s*\[/.test(targetBody),
         "Public API: //astra:public uses 'deps' instead of 'public_deps' — " +
         "all public target dependencies must be public_deps");

  // Must have public_deps.
  assert(/(?:^|\n)\s*public_deps\s*=\s*\[/.test(targetBody),
         "Public API: //astra:public has no public_deps");

  // Must have visibility restriction.
  assert(/(?:^|\n)\s*visibility\s*=\s*\[/.test(targetBody),
         "Public API: //astra:public has no visibility restriction — " +
         "should be limited to //chrome/* or similar");

  // Must not list any .cc source files (headers-only surface).
  const sourcesInPublic = targetBody.match(/sources\s*=\s*\[([\s\S]*?)\]/);
  if (sourcesInPublic) {
    const sourcesBlock = sourcesInPublic[1];
    const ccFiles = sourcesBlock.match(/[^"]+\.cc"/g) || [];
    assert(ccFiles.length === 0,
           `Public API: //astra:public lists ${ccFiles.length} .cc file(s) ` +
           `in sources — public surface must be header-only`);
  }

  // Extract public_deps list and verify they point to safe targets.
  const publicDepsBlock = targetBody.match(/public_deps\s*=\s*\[([\s\S]*?)\]/);
  if (publicDepsBlock) {
    const depsBlock = publicDepsBlock[1];
    const depPattern = /"([^"]+)"/g;
    let depMatch;
    while ((depMatch = depPattern.exec(depsBlock)) !== null) {
      const dep = depMatch[1];
      // Safe deps: buildflags targets.
      // Unsafe deps: implementation targets like //astra/browser, //astra/ui/views, etc.
      const unsafePatterns = [
        /\/\/astra\/browser[^/]*$/,
        /\/\/astra\/app[^/]*$/,
        /\/\/astra\/ui\/views/,
        /\/\/astra\/ui\/color[^/]*$/,
        /\/\/astra\/ui\/accessibility[^/]*$/,
      ];
      for (const unsafe of unsafePatterns) {
        assert(!unsafe.test(dep),
               `Public API: //astra:public depends on ${dep} — ` +
               `implementation deps must not be in the public surface`);
      }
    }
  }
})();

// ============================================================================
// Check 14: Test file naming and target matching
// ============================================================================
// Verifies that:
//   - Test source files follow Chromium naming conventions:
//     * Unit tests end with _unittest.cc
//     * Browser tests end with _browsertest.cc
//   - Each test file on disk has a corresponding test target in BUILD.gn
//   - Each test target (testonly = true) has a meaningful name
//
// Files that are clearly test helpers (ending in _test.h, or in test/
// directories) are exempt from the naming rule.

(function checkTestFiles() {
  if (!existsSync(astraDir)) return;

  const allCodeFiles = walkRel("chromium/astra").filter((f) =>
    astraCodeExtensions.test(f)
  );

  // Identify test files by naming convention.
  const testFiles = allCodeFiles.filter((f) =>
    /_(unittest|browsertest)\.(cc|h|mm)$/.test(f) ||
    /_test\.(cc|h|mm)$/.test(f)
  );

  // Rule 1: Test .cc files must use proper Chromium suffixes.
  // _unittest.cc or _browsertest.cc are valid; _test.cc is not.
  for (const file of testFiles) {
    if (!file.endsWith(".cc")) continue;
    const hasValidSuffix =
      file.endsWith("_unittest.cc") || file.endsWith("_browsertest.cc");
    assert(hasValidSuffix,
           `Test naming: ${file} does not follow Chromium test naming ` +
           `convention — use _unittest.cc or _browsertest.cc`);
  }

  // Rule 2: Each test file must be listed in a BUILD.gn test target.
  // We collect all files from testonly = true targets.
  const buildGnFiles = astraFiles.filter((f) => basename(f) === "BUILD.gn");
  const testTargetSources = new Set();

  for (const gnFile of buildGnFiles) {
    const contents = read(gnFile);
    const gnDir = dirname(gnFile);
    const gnDirRelFromAstra = relative(astraDir, join(root, gnDir));

    // Find all testonly targets and extract their sources.
    // Strategy: find each target block, check if it has testonly = true,
    // then extract sources from that block.
    // Regex approach: match each source_set/group/etc block and check for testonly.
    const targetPattern = /(?:source_set|group|component|static_library)\("([^"]+)"\)\s*\{([\s\S]*?)\n\}/g;
    let targetMatch;
    while ((targetMatch = targetPattern.exec(contents)) !== null) {
      const targetName = targetMatch[1];
      const targetBody = targetMatch[2];

      const isTestOnly = /testonly\s*=\s*true/.test(targetBody);
      if (!isTestOnly) continue;

      // Extract sources from this test-only target.
      const sourcesMatch = targetBody.match(/sources\s*=\s*\[([\s\S]*?)\]/);
      if (sourcesMatch) {
        const sourcesBlock = sourcesMatch[1];
        const filePattern = /"([^"]+\.(?:cc|h|mm))"/g;
        let fileMatch;
        while ((fileMatch = filePattern.exec(sourcesBlock)) !== null) {
          const filePath = fileMatch[1];
          const fullPath = join(gnDirRelFromAstra, filePath);
          testTargetSources.add(fullPath);
        }
      }

      // Rule 3: testonly targets should have "test" in their name.
      const hasTestInName = /test/i.test(targetName);
      assert(hasTestInName,
             `Test naming: target '${targetName}' in ${gnFile} has ` +
             `testonly = true but name does not contain 'test'`);
    }
  }

  // Verify each test file on disk is in a test target.
  for (const file of testFiles) {
    if (!file.endsWith(".cc")) continue; // .h test helpers don't need targets
    assert(testTargetSources.has(file),
           `Test naming: ${file} is a test file but is not listed in ` +
           `any testonly = true BUILD.gn target`);
  }
})();

// ============================================================================
// Results
// ============================================================================

if (warnings.length > 0) {
  console.warn("Architecture guard warnings:");
  for (const w of warnings) {
    console.warn(`  ! ${w}`);
  }
  console.warn("");
}

if (errors.length > 0) {
  console.error("Architecture guard FAILED:");
  for (const error of errors) {
    console.error(`  ✗ ${error}`);
  }
  console.error("");
  console.error(`${errors.length} error(s), ${warnings.length} warning(s).`);
  process.exit(1);
}

const checkedFiles = astraFiles.length + srcFiles.length + activeDocFiles.length;
console.log(`Architecture guard passed.`);
console.log(`  Files checked: ${checkedFiles}`);
console.log(`  Rules enforced: 14`);
console.log(`  Warnings: ${warnings.length}`);
