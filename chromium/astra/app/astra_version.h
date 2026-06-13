// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Astra product version information.
//
// Astra has its own product version that is independent of the Chromium
// engine version.  The Chromium engine has its own version (e.g., "131.0.6778.85")
// which reflects the Blink / V8 / network stack versions.  Astra's version
// is a product-level version that tracks Astra-specific features and
// branding changes on top of the Chromium engine.
//
// Version format: MAJOR.MINOR.PATCH
//   - MAJOR: Incremented for breaking changes or major product releases.
//   - MINOR: Incremented for new features in backwards-compatible ways.
//   - PATCH: Incremented for bug fixes and minor updates.
//
// This file provides both:
//   - Macro-style defines (ASTRA_VERSION_MAJOR, etc.) for preprocessor use
//     and build-system integration.
//   - C++ constexpr constants (kAstraVersionString, etc.) for type-safe
//     usage in C++ code.
//
// Chromium owns: engine version (chrome/version, chrome_version_string),
// version generation infrastructure (chrome/VERSION file, version.py).
// Astra owns: product version string, milestone/build number semantics.
//
// Display convention (about dialog, --version output):
//   Astra Browser 0.1.0 (Chromium 131.0.6778.85)
//
// How version is determined:
//   The version is defined in this header as compile-time constants.
//   For official CI builds, the build system can override these values
//   via compiler defines (-DASTRA_VERSION_MAJOR=1 etc.).
//   TODO(astra): Auto-generate version numbers from build system / git tags
//   instead of hardcoding.  Follow Chromium's version.py pattern but for the
//   Astra product layer.
//   Patch point: //chrome/VERSION (engine) — Astra version is in astra layer,
//   not in Chromium's VERSION file.
//
// TODO(astra): Add a build-time version header generation step that reads
// from a VERSION file in astra/ (similar to chrome/VERSION) and generates
// this header.  This would allow version bumps without editing C++ code.
// Patch point: //tools/version/version.py (Chromium version generator)

#ifndef ASTRA_APP_ASTRA_VERSION_H_
#define ASTRA_APP_ASTRA_VERSION_H_

// ---------------------------------------------------------------------------
// Macro-style version defines
// ---------------------------------------------------------------------------
//
// These are #define macros so they can be used by the preprocessor,
// stringified in build scripts, and compared with #if directives.
//
// For C++ code, prefer the constexpr constants below.
//
// To override at build time:
//   gn gen out/Default --args='astra_version_major=1'
// or via compiler flags:
//   -DASTRA_VERSION_MAJOR=1

#ifndef ASTRA_VERSION_MAJOR
// Major version component.  Increment for breaking changes or major releases.
#define ASTRA_VERSION_MAJOR 0
#endif

#ifndef ASTRA_VERSION_MINOR
// Minor version component.  Increment for new feature releases.
#define ASTRA_VERSION_MINOR 1
#endif

#ifndef ASTRA_VERSION_PATCH
// Patch version component.  Increment for bug fix releases.
#define ASTRA_VERSION_PATCH 0
#endif

#ifndef ASTRA_VERSION_BUILD
// Build number component.
// In official CI builds, this is set from the build number / commit position.
// Not included in the standard MAJOR.MINOR.PATCH display string.
#define ASTRA_VERSION_BUILD 1
#endif

// Full version string in MAJOR.MINOR.PATCH format.
//
// This macro uses token-pasting to construct the version string from the
// individual component macros.  It is re-evaluated when any of the
// component macros are redefined (e.g., by build overrides).
//
// Implementation note: The double-macro expansion trick is needed so that
// the macro values (not names) get stringified.
#define ASTRA_VERSION_STRINGIFY(x) #x
#define ASTRA_VERSION_STRINGIFY2(x) ASTRA_VERSION_STRINGIFY(x)
#define ASTRA_VERSION \
  ASTRA_VERSION_STRINGIFY2(ASTRA_VERSION_MAJOR) \
  "." ASTRA_VERSION_STRINGIFY2(ASTRA_VERSION_MINOR) \
  "." ASTRA_VERSION_STRINGIFY2(ASTRA_VERSION_PATCH)

// Full version with build number: MAJOR.MINOR.PATCH.BUILD
#define ASTRA_FULL_VERSION \
  ASTRA_VERSION "." ASTRA_VERSION_STRINGIFY2(ASTRA_VERSION_BUILD)

// ---------------------------------------------------------------------------
// C++ constexpr version constants
// ---------------------------------------------------------------------------
//
// These are type-safe C++ constants derived from the macros above.
// Use these in C++ code instead of the macros.

namespace astra {

// Full human-readable version string.
// Format: MAJOR.MINOR.PATCH (semver-like).
// Example: "0.1.0"
inline constexpr char kAstraVersionString[] = ASTRA_VERSION;

// Full version string with build number.
// Format: MAJOR.MINOR.PATCH.BUILD
// Example: "0.1.0.1"
inline constexpr char kAstraFullVersionString[] = ASTRA_FULL_VERSION;

// Individual version components.
inline constexpr int kAstraMajorVersion = ASTRA_VERSION_MAJOR;
inline constexpr int kAstraMinorVersion = ASTRA_VERSION_MINOR;
inline constexpr int kAstraPatchVersion = ASTRA_VERSION_PATCH;

// ---------------------------------------------------------------------------
// Milestone / build number
// ---------------------------------------------------------------------------

// Build number — monotonically increasing per build.
// Used by update / crash reporting to identify exact builds.
// In CI builds, this should be set from the build number / commit position.
// TODO(astra): Wire build number into CI / build system.
inline constexpr int kAstraBuildNumber = ASTRA_VERSION_BUILD;

// Milestone version — the Astra product milestone.
// This increments with each major release and is used for feature gating,
// deprecation checks, and migration logic.
//
// The milestone version follows the major version for now.  In the future,
// milestones may be independent (like Chromium milestones vs Chrome version).
// TODO(astra): Define milestone cadence once release process is established.
inline constexpr int kAstraMilestoneVersion = ASTRA_VERSION_MAJOR;

// ---------------------------------------------------------------------------
// Version comparison helpers
// ---------------------------------------------------------------------------
//
// These helpers provide compile-time version comparison.
// They are useful for feature gating and migration logic.

// Returns true if the current version is at least (major, minor, patch).
constexpr bool IsAstraVersionAtLeast(int major, int minor, int patch) {
  return kAstraMajorVersion > major ||
         (kAstraMajorVersion == major && kAstraMinorVersion > minor) ||
         (kAstraMajorVersion == major && kAstraMinorVersion == minor &&
          kAstraPatchVersion >= patch);
}

// ---------------------------------------------------------------------------
// Version display helper
// ---------------------------------------------------------------------------
//
// Use kAstraVersionString for display purposes.
// Use individual components for version comparisons and migration logic.
//
// For the Chromium engine version, use:
//   chrome/version.h -> chrome_version_string()
//   base/version.h  -> base::Version
//
// For a combined Astra + Chromium version string, use:
//   astra_brand.h -> GetAstraFullVersionString()
//
// TODO(astra): Add a function like GetAstraFullVersionString() that combines
// Astra product version with Chromium engine version for display.
// This would live in astra_brand.cc or a separate version_util.cc.

}  // namespace astra

#endif  // ASTRA_APP_ASTRA_VERSION_H_
