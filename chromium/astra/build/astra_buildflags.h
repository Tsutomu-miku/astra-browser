// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Astra build flags — compile-time configuration for the Astra product layer.
//
// Build flags vs feature flags:
//   Build flags (this file) are compile-time constants set by the GN build
//   system. They determine which code is compiled into the binary at all.
//   Use them for things that must not be present in certain build variants
//   (e.g., official vs developer builds, or code that only makes sense when
//   Astra branding is enabled).
//
//   Feature flags (astra_feature_list.h, via base::FeatureList) are runtime
//   toggles that can be changed via command line, field trials, or about:flags.
//   Use them for features that should be compiled in everywhere but may be
//   turned on or off at runtime.
//
// Chromium owns: buildflag.h macro system, GN build configuration,
// chrome_build.gni flag definitions.
// Astra owns: product-specific build flag names and default values.
//
// Naming follows Chromium buildflag conventions:
//   - IS_ASTRA_BRANDED   — "is" style boolean flag (branding on/off)
//   - ENABLE_ASTRA_*     — "enable" style feature compilation flags
//
// TODO(astra): Wire these flags into build/config/chrome_build.gni so
// the GN args like `is_astra_branded=true` set these build flags at
// compile time.
// Patch point: //build/config/chrome_build.gni
//
// TODO(astra): Generate this header from GN buildflag template like Chromium's
// other buildflag headers (e.g., chrome_buildflags.h, content_buildflags.h).
// Patch point: //build/buildflag_header.gni template via astra/build/BUILD.gn

#ifndef ASTRA_BUILD_ASTRA_BUILDFLAGS_H_
#define ASTRA_BUILD_ASTRA_BUILDFLAGS_H_

#include "build/buildflag.h"

// ---------------------------------------------------------------------------
// Branding
// ---------------------------------------------------------------------------

// Whether this build is an Astra-branded browser build.
// When enabled, Chromium UI surfaces show "Astra Browser" instead of
// "Chromium" or "Google Chrome", and Astra-specific UI surfaces are
// compiled in (sidebar, workspace switcher, etc.).
//
// Set by GN arg: is_astra_branded = true
// Default: disabled (plain Chromium build)
//
// TODO(astra): Generate from build/buildflag_header.gni template instead
// of hardcoding here.  See chrome_buildflags.h for the pattern.
#define BUILDFLAG_INTERNAL_IS_ASTRA_BRANDED() (0)

// ---------------------------------------------------------------------------
// Feature compilation flags
// ---------------------------------------------------------------------------
//
// Each feature can be independently enabled or disabled at build time.
// By default, all Astra features are enabled in a developer build.
// Features can be individually turned off for size-optimized builds.
//
// Set by GN args in astra/build/buildflags.gni.
// Access in C++ via BUILDFLAG(ENABLE_ASTRA_*).
//
// Usage pattern:
//   #if BUILDFLAG(ENABLE_ASTRA_SIDEBAR)
//     // Sidebar-specific code
//   #endif

// Whether the Astra sidebar UI surface is compiled into the binary.
// The sidebar is the primary Astra navigation surface (vertical tab strip
// + workspace switcher + favorites).
//
// When disabled at build time, sidebar code is not compiled at all and
// the browser uses Chromium's standard horizontal tab strip.
//
// Default: enabled
// Controlled by GN arg: enable_astra_sidebar
//
// TODO(astra): Wire to GN build flag.
// Patch point: //build/config/chrome_build.gni (add enable_astra_sidebar)
#define BUILDFLAG_INTERNAL_ENABLE_ASTRA_SIDEBAR() (1)

// Whether Astra split view / Glance presentation mode is compiled in.
// Split view allows two WebContents to be shown side-by-side in a single
// browser tab container, with a draggable divider.
//
// When disabled at build time, split view classes and UI surfaces are
// not compiled and tab containers always show a single WebContents.
//
// Default: enabled
// Controlled by GN arg: enable_astra_split_view
//
// TODO(astra): Wire to GN build flag.
// Patch point: //build/config/chrome_build.gni (add enable_astra_split_view)
#define BUILDFLAG_INTERNAL_ENABLE_ASTRA_SPLIT_VIEW() (1)

// Whether Astra workspaces are compiled into the binary.
// Workspaces let users organize tabs into named, switchable groups with
// their own metadata, favorites, and session state.
//
// When disabled at build time, workspace metadata services and UI
// surfaces are not compiled.  The browser behaves like a standard
// single-workspace Chromium window.
//
// Default: enabled
// Controlled by GN arg: enable_astra_workspaces
//
// TODO(astra): Wire to GN build flag.
// Patch point: //build/config/chrome_build.gni (add enable_astra_workspaces)
#define BUILDFLAG_INTERNAL_ENABLE_ASTRA_WORKSPACES() (1)

// Whether the Astra command palette is compiled into the binary.
// The command palette provides quick-access command search (Cmd/Ctrl+K)
// for navigating the browser and triggering Astra-specific commands.
//
// When disabled at build time, the command palette UI and command
// registry are not compiled.
//
// Default: enabled
// Controlled by GN arg: enable_astra_command_palette
//
// TODO(astra): Wire to GN build flag.
// Patch point: //build/config/chrome_build.gni (add enable_astra_command_palette)
#define BUILDFLAG_INTERNAL_ENABLE_ASTRA_COMMAND_PALETTE() (1)

// Whether Astra focus mode is compiled into the binary.
// Focus mode provides distraction-free browsing by hiding UI chrome,
// dimming secondary tabs, suppressing notifications, and presenting
// a streamlined single-tab view.
//
// When disabled at build time, focus mode UI and state management
// are not compiled.
//
// Default: enabled
// Controlled by GN arg: enable_astra_focus_mode
//
// TODO(astra): Wire to GN build flag.
// Patch point: //build/config/chrome_build.gni (add enable_astra_focus_mode)
#define BUILDFLAG_INTERNAL_ENABLE_ASTRA_FOCUS_MODE() (1)

// Whether the Astra DevTools panel is compiled into the binary.
// The Astra DevTools panel adds an Astra-specific tab to the Chromium
// DevTools window for inspecting Astra metadata (workspaces, sidebar
// state, split view configuration, tab metadata).
//
// When disabled at build time, the DevTools panel extension and
// associated inspection bridge are not compiled.
//
// Default: enabled
// Controlled by GN arg: enable_astra_devtools_panel
//
// TODO(astra): Wire to GN build flag.
// Patch point: //build/config/chrome_build.gni (add enable_astra_devtools_panel)
#define BUILDFLAG_INTERNAL_ENABLE_ASTRA_DEVTOOLS_PANEL() (1)

// Whether Astra WebUI pages are compiled into the binary.
// Astra WebUI pages are chrome://astra-* pages for settings, workspace
// management, and other Astra-specific surfaces.
//
// When disabled (the default), Astra uses native Views UI instead of
// WebUI for its surfaces, which is preferred for performance and
// integration with Chromium's Views framework.
//
// Default: disabled (prefer native Views)
// Controlled by GN arg: enable_astra_webui
//
// TODO(astra): Wire to GN build flag.
// Patch point: //build/config/chrome_build.gni (add enable_astra_webui)
#define BUILDFLAG_INTERNAL_ENABLE_ASTRA_WEBUI() (0)

// ---------------------------------------------------------------------------
// Build flag usage pattern
// ---------------------------------------------------------------------------
//
// Use BUILDFLAG() macro (from build/buildflag.h) to check flags:
//
//   #if BUILDFLAG(IS_ASTRA_BRANDED)
//     astra::AstraBrand::GetProductName();
//   #else
//     "Chromium";
//   #endif
//
// Build flags are evaluated at compile time, so #if is preferred over
// `if (BUILDFLAG(...))` for code that should be conditionally compiled.
//
// For runtime feature toggles (not compile-time), use base::Feature
// and AstraFeatureList instead.

#endif  // ASTRA_BUILD_ASTRA_BUILDFLAGS_H_
