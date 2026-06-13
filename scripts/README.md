# Astra Scripts

This directory contains build, check, and bootstrap scripts for the Astra
Chromium overlay project.

## Table of Contents

- [Architecture Checks](#architecture-checks)
- [BUILD.gn Linting](#buildgn-linting)
- [Bootstrap & Setup](#bootstrap--setup)
- [Patch Point Management](#patch-point-management)
- [Build & Sync](#build--sync)
- [Release Utilities](#release-utilities)
- [Legacy Scripts](#legacy-scripts)

---

## Architecture Checks

### `check-architecture.mjs`

**Purpose:** Validates that the direct Chromium architecture is followed
throughout the codebase.  This is the primary quality gate for Astra's
layered architecture.

**How to run:**
```bash
node scripts/check-architecture.mjs
# or via package.json:
pnpm check:architecture
```

**Checks performed (14 total):**

1.  **Forbidden legacy paths** — No Electron/CEF/CMake residue
2.  **Required active files** — Architecture docs and entry points exist
3.  **Active docs — no stale terminology** — No legacy architecture phrases
4.  **Forbidden runtime patterns** — No CEF/Electron/AppKit references in source
5.  **No Chromium service redefinitions** — Astra doesn't redefine Chromium-owned services
6.  **Class/struct Astra prefix** — Classes use the `Astra` prefix (warning level)
7.  **Source file prefix** — Files start with `astra_`
8.  **Header guards** — Chromium-style `#ifndef` / `#define` / `#endif` guards
9.  **TODO ownership** — All TODOs use `TODO(astra):`
10. **Legacy src/ isolation** — No new C++/GN in `src/`
11. **BUILD.gn source completeness** — Every .cc/.h file on disk is in a sources list
12. **Dependency direction** — Lower layers never include from higher layers
    - `common/` must not include from `browser/` or `ui/`
    - `browser/` must not include from `ui/views/` or `app/`
    - `ui/views/` must not include from `app/`
13. **Public API surface** — `//astra:public` target only exposes buildflags and headers
14. **Test file naming** — Test files use `_unittest.cc` / `_browsertest.cc` and have matching test targets

**Exit code:** 0 on pass, 1 on failure.

---

## BUILD.gn Linting

### `check-build-gn.mjs`

**Purpose:** Focused linter for BUILD.gn files under `chromium/astra/`.
Ensures consistent formatting and best practices.

**How to run:**
```bash
node scripts/check-build-gn.mjs
```

**Checks performed (6 total):**

1.  **sources sorted** — Alphabetical order within `sources = [...]`
2.  **deps sorted** — By path depth, then alphabetically (warning level)
3.  **public_deps sorted** — Same depth/alpha ordering (warning level)
4.  **public_deps before deps** — `public_deps` block appears before `deps`
5.  **visibility specified** — Every target declares visibility (warning level)
6.  **testonly naming** — `testonly = true` targets have `test` in their name

**Exit code:** 0 on pass, 1 on error-level failures.

---

## Bootstrap & Setup

### `bootstrap-chromium.mjs`

**Purpose:** Diagnostic / planning script that checks the local environment
for Astra Chromium development readiness.  It does **not** download or build
anything — it only reports what's available and what's missing.

**How to run:**
```bash
node scripts/bootstrap-chromium.mjs
```

**What it checks:**

- Chromium source tree presence (via `CHROMIUM_SRC` env var, `.chromium`
  marker file, or default `chromium/src/` location)
- Required build tools: `gn`, `ninja`, `clang`, `lld`
- Depot tools availability (`gclient`, `fetch`)

**What it outputs:**

- Status report of each dependency
- Sample `args.gn` for Astra builds (with all feature flags documented)
- Step-by-step build instructions

**Environment variables:**
- `CHROMIUM_SRC` — Path to Chromium src/ directory
- `DEPOT_TOOLS` — Path to depot_tools directory

**Exit code:** 0 if all prerequisites found, 1 otherwise.

### `chromium-bootstrap.sh` (shell)

**Purpose:** Full bootstrap script that actually downloads and sets up a
Chromium checkout.  This is the real bootstrap — it clones depot_tools,
fetches Chromium, and syncs the Astra overlay.

**How to run:**
```bash
./scripts/chromium-bootstrap.sh
```

**What it does:**

1. Clones or updates `depot_tools`
2. Clones or syncs a Chromium checkout
3. Syncs the Astra overlay into the Chromium tree
4. Prints patch point instructions

**Environment variables:**
- `DEPOT_TOOLS_DIR` — Path to depot_tools (default: `./third_party/depot_tools`)
- `CHROMIUM_SRC` — Path to Chromium checkout (default: `./chromium/src`)
- `CHROMIUM_BRANCH` — Branch to check out (default: `main`)

> **Note:** This script can take a very long time (hours) on first run
> because Chromium is a large repository.

### `sync-chromium-overlay.sh` (shell)

**Purpose:** Syncs the Astra overlay from `chromium/astra/` into the
Chromium source tree at `$CHROMIUM_SRC/astra/`.

**How to run:**
```bash
./scripts/sync-chromium-overlay.sh
```

### `build-chromium.sh` (shell)

**Purpose:** Builds Chromium with the Astra overlay.

**How to run:**
```bash
./scripts/build-chromium.sh Debug
# or
./scripts/build-chromium.sh Release
```

---

## Patch Point Management

### `list-patch-points.mjs`

**Purpose:** Scans all source files under `chromium/astra/` for patch point
comments and generates a consolidated report.

**How to run:**
```bash
# Default: group by file
node scripts/list-patch-points.mjs

# Group by architectural layer
node scripts/list-patch-points.mjs --by-layer

# Group by category (best-effort heuristic)
node scripts/list-patch-points.mjs --by-category

# JSON output (for scripting / tooling)
node scripts/list-patch-points.mjs --json
```

**Patch point formats detected:**
```c++
// Patch point: chrome/browser/ui/browser.h — add Astra widget
// Chromium patch point: //chrome/test/BUILD.gn test suite
```

**Output modes:**
- `--by-file` — Group results by source file (default)
- `--by-layer` — Group by architectural layer (build, common, browser, etc.)
- `--by-category` — Group by patch point category (UI, commands, profile, etc.)
- `--json` — Machine-readable JSON output

**Informational entries:** Patch points marked as "None needed" or "no patch
needed" are flagged with `i` marker and counted as informational.

---

## Build & Sync

### `sync-chromium-overlay.sh`

See [Bootstrap & Setup](#bootstrap--setup).

### `build-chromium.sh`

See [Bootstrap & Setup](#bootstrap--setup).

---

## Release Utilities

### `clean-release.mjs`

**Purpose:** Cleans release build artifacts.

**How to run:**
```bash
node scripts/clean-release.mjs
```

### `clean-package-output.mjs`

**Purpose:** Cleans package output from electron-builder.

**How to run:**
```bash
node scripts/clean-package-output.mjs
```

### `notarize.js`

**Purpose:** macOS notarization for signed builds.

### `sign-windows.js`

**Purpose:** Windows code signing for builds.

---

## Legacy Scripts

### `check-sources.mjs`

**Status:** LEGACY — Electron era

**Purpose:** Checks the legacy Electron/Vite frontend source tree (`src/`).
Preserved for reference and for the remaining Electron build pipeline.

> Per AGENTS.md: *"Legacy src/ is migration reference only.
> Do not add new architecture there."*

Do **not** extend this script with new architecture checks.
New Astra / direct-Chromium architecture checks belong in
`check-architecture.mjs`.

---

## Running All Checks

To run the full architecture quality gate:

```bash
node scripts/check-architecture.mjs
node scripts/check-build-gn.mjs
```

Or via `pnpm` where configured:
```bash
pnpm check:architecture
```
