# Patch 0005: Astra Branding Constants

**Patch ID:** 0005
**File:** Multiple files (see below)
**Size estimate:** ~20 lines total across 4-5 files
**Status:** planned
**Astra component:** `astra/app/astra_brand.*`, `astra/app/astra_version.h`

## Context

Chromium's product identity — the name that appears in window titles, about
dialogs, menus, crash reports, installer, and OS-level app metadata — is
defined across several files in the Chromium source tree.  For Astra to show
"Astra Browser" instead of "Chromium" / "Google Chrome", we need to patch
those branding points.

This cannot be done from `//astra` alone because Chromium's core UI and
system integration code directly references Chromium product name constants.
Small patches at the definition sites replace those constants with Astra
values when `BUILDFLAG(IS_ASTRA_BRANDED)` is true.

Key Chromium subsystems involved:
- **chrome/common/chrome_constants.cc** — `kBrowser`, `kBrowserProcessExecutableName`, etc.
- **chrome/app/theme/** — app icons, color palette, product logo.
- **chrome/installer/** — installer branding (Windows).
- **chrome/browser/about_flags.cc** — about:flags page title.
- **chrome/browser/resources/settings/** — settings page branding.

## Change

### 1. chrome/common/chrome_constants.cc — product name constants

Replace the hardcoded "Chromium" / "Google Chrome" strings with
Astra-branded values when the build flag is set.

**Before:**

```cpp
const char kBrowserProcessExecutableName[] = "Chromium";
const char kBrowserFrameName[] = "Chromium";
// ...
const char kShortProductName[] = "Chromium";
```

**After:**

```cpp
#include "astra/build/astra_buildflags.h"
#include "astra/app/astra_brand.h"

#if BUILDFLAG(IS_ASTRA_BRANDED)
const char kBrowserProcessExecutableName[] = astra::kAstraExecutableName;
const char kBrowserFrameName[] = astra::kAstraProductName;
// ...
const char kShortProductName[] = astra::kAstraShortProductName;
#else
const char kBrowserProcessExecutableName[] = "Chromium";
const char kBrowserFrameName[] = "Chromium";
// ...
const char kShortProductName[] = "Chromium";
#endif
```

TODO(astra): Identify the complete set of constants in chrome_constants.cc
that need Astra values.  This is an estimate of the key ones.

### 2. chrome/app/theme/chromium/ — app icons

Replace Chromium's default product icons with Astra icons.

**Before:**

```gn
# chrome/app/theme/chromium/BUILD.gn
copy("theme_resources") {
  sources = [
    "product_logo_16.png",
    "product_logo_32.png",
    # ...
  ]
```

**After:**

```gn
if (is_astra_branded) {
  sources += [ "//astra/app/theme/product_logo_16.png" ]
  # override with Astra icons
} else {
  sources += [ "product_logo_16.png" ]
}
```

TODO(astra): Determine the cleanest way to replace icons — either by
overriding the theme directory for Astra builds or by patching the
icon file paths in the GN build.
Patch point: `//chrome/app/theme/BUILD.gn`

### 3. chrome/installer/ — installer branding (Windows)

Replace installer strings and icons with Astra branding.

**Before:**

```rc
// chrome/installer/setup/setup.rc
#define IDS_PROJNAME "Chromium"
```

**After:**

```rc
#if defined(IS_ASTRA_BRANDED)
#define IDS_PROJNAME "Astra Browser"
#else
#define IDS_PROJNAME "Chromium"
#endif
```

TODO(astra): Scope installer patches once we target Windows builds.
Patch point: `//chrome/installer/`

### 4. build/config/chrome_build.gni — is_astra_branded GN flag

Add the `is_astra_branded` build flag that controls all other Astra
compilation.

**Before:**

```gn
# build/config/chrome_build.gni
declare_args() {
  is_chrome_branded = false
  is_chromecast = false
  # ...
```

**After:**

```gn
declare_args() {
  is_astra_branded = false
  is_chrome_branded = false
  # ...
```

This is the master GN flag.  When `is_astra_branded=true` is passed to `gn
gen`, all Astra-specific code paths and resources are compiled in.

### 5. build/buildflag / astra buildflags header

Generate the `astra_buildflags.h` header from GN so `BUILDFLAG(IS_ASTRA_BRANDED)`
works at compile time.

This follows the same pattern as `chrome_buildflags.h` / `content_buildflags.h`.

TODO(astra): Set up `buildflag_header` target for Astra build flags.
Patch point: `//build/buildflag_header.gni` — template to use.

## Rationale

Branding must be wired into Chromium's core constant definitions because the
entire browser UI, system integration, and crash reporting stack references
those constants.  It would be impractical (and a much larger patch) to
override every display site individually.

The approach is:
1. Define Astra string constants in `//astra/app/astra_brand.h`.
2. Patch Chromium's constant definition files to conditionally use Astra
   values when `BUILDFLAG(IS_ASTRA_BRANDED)`.
3. Patch resource files to use Astra icons and theme assets.

This keeps patches minimal and centralized.  All product-specific values
live in `//astra`, and Chromium patches just reference them.

## Build Flag

- Gate: `BUILDFLAG(IS_ASTRA_BRANDED)` (compile-time)
- Build flag defined in: `build/config/chrome_build.gni` → `is_astra_branded` arg
- Generated buildflag header: `astra/build/astra_buildflags.h`
- Default: `false` (plain Chromium build)

## Alternatives Considered

1. **Override at every display site** — e.g., patch every `title.SetText(...)`
   call to use Astra branding.  Rejected: far too many patches, high
   maintenance cost, fragile.

2. **Replace the entire chrome/app/theme directory** — wholesale replacement
   of branding files.  Rejected: too invasive, makes rebasing harder, and
   we want to reuse as much of Chromium as possible.

3. **Use a separate branding config file** — e.g., a JSON or GYP file with
   brand strings, similar to how Chrome for Android does branding.
   Considered: this is actually how Chromium works internally (chrome/brand).
   We may move to this approach later.  For now, string constants in a header
   are simpler and follow the patch + delegate pattern.

4. **Resource-only branding (no code patches)** — use only `.grd` string
   overrides and icon replacement.  Rejected: many Chromium constants are
   C++ string literals, not loaded from resource files, so this wouldn't
   cover window titles, crash reports, process names, etc.

## Risks & Rebase Concerns

- **chrome_constants.cc changes rarely** — it's a stable file, so the patch
  should rebase cleanly.
- **Installer patches** are Windows-specific and can be deferred until we
  target Windows builds.
- **Icon/theme files** change with major Chromium milestones but the build
  pattern is stable.
- **Risk of missing a branding site** — there may be hardcoded "Chromium"
  strings in unexpected places.  We'll find these during QA and add them
  to the patch set.  This is an iterative process.

## Related

- Astra source: `astra/app/astra_brand.h`
- Astra source: `astra/app/astra_version.h`
- Astra source: `astra/build/astra_buildflags.h`
- Astra source: `astra/app/resources/README.md`
- Related patch: 0003 (build flag configuration — may be the same patch)
- ADR: `docs/adr/0009-direct-chromium-architecture.md`
