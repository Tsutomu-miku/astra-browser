// ============================================================================
// Chromium Bootstrap — Diagnostic / Planning Script
// ============================================================================
// Checks the local environment for Astra Chromium development readiness.
// This is a diagnostic script — it does NOT download or build anything.
//
// What it checks:
//   1. Chromium source tree presence (looks for .chromium marker or env var)
//   2. Required build tools (gn, ninja, clang)
//   3. Depot tools availability (gclient, fetch)
//   4. Generates a sample args.gn for Astra builds
//   5. Outputs build instructions
//
// Usage:
//   node scripts/bootstrap-chromium.mjs
//
// Environment variables:
//   CHROMIUM_SRC   Path to Chromium src/ directory
//   DEPOT_TOOLS    Path to depot_tools directory
// ============================================================================

import { existsSync, readFileSync, statSync } from "node:fs";
import { join, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import { execSync } from "node:child_process";

const root = fileURLToPath(new URL("..", import.meta.url));

// ============================================================================
// Helpers
// ============================================================================

function checkCmd(cmd) {
  try {
    execSync(`which ${cmd} 2>/dev/null`, { encoding: "utf8" });
    return true;
  } catch {
    return false;
  }
}

function cmdPath(cmd) {
  try {
    return execSync(`which ${cmd} 2>/dev/null`, { encoding: "utf8" }).trim();
  } catch {
    return null;
  }
}

function cmdVersion(cmd, versionFlag = "--version") {
  try {
    return execSync(`${cmd} ${versionFlag} 2>&1 | head -1`, {
      encoding: "utf8",
    }).trim();
  } catch {
    return null;
  }
}

function dirExists(path) {
  try {
    return statSync(path).isDirectory();
  } catch {
    return false;
  }
}

// ============================================================================
// 1. Chromium source tree
// ============================================================================

function checkChromiumSource() {
  const result = { found: false, path: null, source: null };

  // Check CHROMIUM_SRC env var first.
  const envPath = process.env.CHROMIUM_SRC;
  if (envPath) {
    const fullPath = resolve(envPath);
    if (dirExists(fullPath)) {
      result.found = true;
      result.path = fullPath;
      result.source = "CHROMIUM_SRC environment variable";
      return result;
    }
  }

  // Check default location: chromium/src (relative to project root).
  const defaultPath = join(root, "chromium", "src");
  if (dirExists(defaultPath)) {
    result.found = true;
    result.path = defaultPath;
    result.source = "default location (chromium/src)";
    return result;
  }

  // Check for .chromium marker file in project root.
  const markerFile = join(root, ".chromium");
  if (existsSync(markerFile)) {
    try {
      const markerContent = readFileSync(markerFile, "utf8").trim();
      const markerPath = resolve(root, markerContent);
      if (dirExists(markerPath)) {
        result.found = true;
        result.path = markerPath;
        result.source = ".chromium marker file";
        return result;
      }
    } catch {
      // Marker file exists but points to invalid path.
    }
  }

  return result;
}

// ============================================================================
// 2. Build tools
// ============================================================================

function checkBuildTools() {
  const tools = [
    { name: "gn", desc: "GN build generator", version: "--version" },
    { name: "ninja", desc: "Ninja build tool", version: "--version" },
    { name: "clang++", desc: "Clang C++ compiler", version: "--version" },
    { name: "clang", desc: "Clang C compiler", version: "--version" },
    { name: "lld", desc: "LLVM linker (lld)", version: "--version" },
  ];

  return tools.map((tool) => {
    const path = cmdPath(tool.name);
    const version = path ? cmdVersion(tool.name, tool.version) : null;
    return {
      name: tool.name,
      desc: tool.desc,
      found: !!path,
      path,
      version,
    };
  });
}

// ============================================================================
// 3. Depot tools
// ============================================================================

function checkDepotTools() {
  const result = { found: false, path: null, source: null };

  // Check DEPOT_TOOLS env var.
  const envPath = process.env.DEPOT_TOOLS;
  if (envPath) {
    const fullPath = resolve(envPath);
    if (dirExists(fullPath)) {
      result.found = true;
      result.path = fullPath;
      result.source = "DEPOT_TOOLS environment variable";
      return result;
    }
  }

  // Check default location: third_party/depot_tools.
  const defaultPath = join(root, "third_party", "depot_tools");
  if (dirExists(defaultPath)) {
    result.found = true;
    result.path = defaultPath;
    result.source = "default location (third_party/depot_tools)";
    return result;
  }

  // Check if gclient is on PATH (indicates depot_tools is available).
  if (checkCmd("gclient")) {
    const gclientPath = cmdPath("gclient");
    result.found = true;
    result.path = gclientPath ? join(gclientPath, "..") : null;
    result.source = "gclient found on PATH";
    return result;
  }

  return result;
}

// ============================================================================
// 4. Sample args.gn
// ============================================================================

function generateSampleArgsGn() {
  return `# =============================================================================
# Astra Browser — sample args.gn
# =============================================================================
# Place this file in your Chromium out/ directory, e.g.:
#   chromium/src/out/Default/args.gn
#
# Generate the build directory with:
#   gn gen out/Default
#
# Build with:
#   ninja -C out/Default chrome
# =============================================================================

# -----------------------------------------------------------------------------
# Build type
# -----------------------------------------------------------------------------
is_debug = true
is_component_build = true
symbol_level = 2

# -----------------------------------------------------------------------------
# Astra branding
# -----------------------------------------------------------------------------
# Enables Astra-branded Chromium build.
# When true, Chromium uses Astra strings, icons, and product name.
# Patch point: build/config/chrome_build.gni
is_astra_branded = true

# -----------------------------------------------------------------------------
# Astra feature flags (build-time)
# -----------------------------------------------------------------------------
# These flags control which Astra features are compiled in.
# They correspond to BUILDFLAG values in astra/build/astra_buildflags.h.

# Sidebar projection and classification.
# Patch point: build/config/chrome_build.gni (add astra_enable_sidebar)
enable_astra_sidebar = true

# Workspace / spaces metadata service.
# Patch point: build/config/chrome_build.gni (add astra_enable_workspaces)
enable_astra_workspaces = true

# Split view / glance presentation.
# Patch point: build/config/chrome_build.gni (add astra_enable_split_view)
enable_astra_split_view = true

# Command palette (quick command access).
# Patch point: build/config/chrome_build.gni (add astra_enable_command_palette)
enable_astra_command_palette = true

# -----------------------------------------------------------------------------
# Target OS
# -----------------------------------------------------------------------------
# Set to your host OS. Uncomment one:
# target_os = "mac"
# target_os = "win"
# target_os = "linux"

# -----------------------------------------------------------------------------
# Target CPU
# -----------------------------------------------------------------------------
# Set to your host CPU architecture. Uncomment one:
# target_cpu = "arm64"   # Apple Silicon / modern ARM
# target_cpu = "x64"     # Intel / AMD

# -----------------------------------------------------------------------------
# Optional: enable test targets
# -----------------------------------------------------------------------------
# Set to true to build unit tests and browser tests.
enable_nacl = false
enable_iterator_debugging = false
`;
}

// ============================================================================
// 5. Build instructions
// ============================================================================

function buildInstructions(chromiumInfo) {
  const chromiumSrc = chromiumInfo.found
    ? chromiumInfo.path
    : "<path/to/chromium/src>";

  return `
# Build Instructions

## 1. Get the Chromium source

If you don't have a Chromium checkout yet:

    # Install depot_tools
    git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
    export PATH="\$PWD/depot_tools:\$PATH"

    # Fetch Chromium (this takes a while — 10+ GB download)
    mkdir -p chromium
    cd chromium
    fetch --nohooks chromium
    cd src

    # Sync dependencies
    gclient sync

## 2. Sync the Astra overlay

The Astra overlay lives at chromium/astra/ in this repo.
It needs to be copied or symlinked into the Chromium source tree:

    # From the project root:
    ./scripts/sync-chromium-overlay.sh

This copies chromium/astra/ to \${chromiumSrc}/astra/.

## 3. Apply Chromium patches

Astra requires a few small patches to the Chromium source tree.
All patches are described in chromium/astra/patches/.

Summary of required patches:
  - chrome/browser/chrome_browser_main.cc
    -> Register AstraBrowserMainExtraParts
  - chrome/browser/ui/views/frame/browser_view.cc
    -> Install AstraBrowserView after BrowserView construction
  - chrome/browser/ui/browser_command_controller.cc
    -> Forward Astra-only command IDs to AstraCommandDelegate
  - chrome/browser/BUILD.gn
    -> Add //astra:astra_browser dependency
  - build/config/chrome_build.gni
    -> Add is_astra_branded GN arg

Patches are intentionally minimal and delegate to //astra code.
Product logic must never live inside Chromium source files.

## 4. Configure the build

    cd ${chromiumSrc}

    # Create an output directory and generate args.gn
    gn gen out/Default

    # Edit args.gn to enable Astra features (see sample above)
    # Or generate with flags directly:
    gn args out/Default --args='is_debug=true is_astra_branded=true'

## 5. Build

    ninja -C out/Default chrome

## 6. Run

    out/Default/Chromium.app/Contents/MacOS/Chromium
    # (or equivalent for your OS)

## Useful build targets

    ninja -C out/Default chrome                    # Full browser
    ninja -C out/Default unit_tests               # Unit tests
    ninja -C out/Default browser_tests            # Browser tests
    ninja -C out/Default astra_browser             # Astra layer only
    ninja -C out/Default astra_tests              # Astra tests
`;
}

// ============================================================================
// Main
// ============================================================================

function main() {
  console.log("=".repeat(72));
  console.log("Astra Chromium Bootstrap — Diagnostic Report");
  console.log("=".repeat(72));
  console.log("");

  // 1. Chromium source
  console.log("Chromium source tree:");
  const chromiumInfo = checkChromiumSource();
  if (chromiumInfo.found) {
    console.log(`  ✓ Found at: ${chromiumInfo.path}`);
    console.log(`    (via ${chromiumInfo.source})`);
  } else {
    console.log("  ✗ Not found");
    console.log("    Set CHROMIUM_SRC environment variable or");
    console.log("    create a .chromium file with the path, or");
    console.log("    use chromium/src/ as the default location.");
  }
  console.log("");

  // 2. Depot tools
  console.log("Depot tools:");
  const depotToolsInfo = checkDepotTools();
  if (depotToolsInfo.found) {
    console.log(`  ✓ Found at: ${depotToolsInfo.path}`);
    console.log(`    (via ${depotToolsInfo.source})`);
  } else {
    console.log("  ✗ Not found on PATH");
    console.log("    Install depot_tools to fetch and sync Chromium.");
  }
  console.log("");

  // 3. Build tools
  console.log("Build tools:");
  const tools = checkBuildTools();
  let allToolsFound = true;
  for (const tool of tools) {
    if (tool.found) {
      console.log(`  ✓ ${tool.name.padEnd(10)} ${tool.desc}`);
      if (tool.version) {
        console.log(`    ${tool.version.split("\n")[0]}`);
      }
    } else {
      console.log(`  ✗ ${tool.name.padEnd(10)} ${tool.desc} (not found)`);
      allToolsFound = false;
    }
  }
  console.log("");

  // 4. Sample args.gn
  console.log("=".repeat(72));
  console.log("Sample args.gn for Astra builds");
  console.log("=".repeat(72));
  console.log("");
  console.log(generateSampleArgsGn());

  // 5. Build instructions
  console.log("=".repeat(72));
  console.log("Build Instructions");
  console.log("=".repeat(72));
  console.log(buildInstructions(chromiumInfo));

  // Summary
  console.log("=".repeat(72));
  console.log("Summary");
  console.log("=".repeat(72));
  console.log("");

  const chromiumOk = chromiumInfo.found;
  const depotOk = depotToolsInfo.found;

  if (chromiumOk && depotOk && allToolsFound) {
    console.log("  ✓ Environment is ready for Astra Chromium development!");
    console.log("");
    console.log("  Next steps:");
    console.log("    1. Sync Astra overlay: ./scripts/sync-chromium-overlay.sh");
    console.log("    2. Apply patches (see chromium/astra/patches/)");
    console.log("    3. Configure: gn gen out/Default");
    console.log("    4. Build:     ninja -C out/Default chrome");
    process.exit(0);
  } else {
    console.log("  ✗ Environment is not fully ready.");
    console.log("");
    if (!chromiumOk) {
      console.log("    - Chromium source tree not found");
    }
    if (!depotOk) {
      console.log("    - Depot tools not found");
    }
    if (!allToolsFound) {
      console.log("    - Some build tools missing");
    }
    console.log("");
    console.log("  See instructions above to set up your environment.");
    process.exit(1);
  }
}

main();
