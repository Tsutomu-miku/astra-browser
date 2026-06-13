// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_APP_ASTRA_FEATURE_LIST_H_
#define ASTRA_APP_ASTRA_FEATURE_LIST_H_

#include <string>
#include <vector>

#include "base/feature_list.h"
#include "base/memory/raw_ptr.h"

class PrefService;

namespace astra {

// ---------------------------------------------------------------------------
// Build flags vs feature flags
// ---------------------------------------------------------------------------
//
// Build flags (astra/build/astra_buildflags.h, BUILDFLAG() macro) are
// compile-time constants set by GN.  They determine which code is compiled
// into the binary at all.  Use build flags for code that must not be present
// in certain build variants (e.g., code that only makes sense with Astra
// branding).
//
// Feature flags (this file, base::FeatureList) are runtime toggles that can
// be changed via command line (--enable-features=X), field trials, or
// about:flags.  Use feature flags for features that should be compiled in
// everywhere but may be turned on or off at runtime for gradual rollout,
// A/B testing, or developer experimentation.
//
// Rule of thumb:
//   - If it depends on IS_ASTRA_BRANDED at compile time: use BUILDFLAG().
//   - If it can be toggled without rebuilding: use base::Feature.
//   - Prefer feature flags for most product features — they give us more
//     flexibility for gradual rollout and emergency kill switches.

// ---------------------------------------------------------------------------
// Core product features (default-enabled in Astra builds)
// ---------------------------------------------------------------------------

// Master flag for all Astra-branded behavior.
// This is always enabled in Astra-branded builds (IS_ASTRA_BRANDED) and
// controls UI surfaces that only make sense for the Astra product identity.
//
// Chromium owns: Browser, TabStripModel, WebContents.
// This feature only gates Astra-specific UI projections and metadata.
BASE_DECLARE_FEATURE(kAstraBrandedBuild);

// Enables the Astra vertical sidebar UI surface.
// Chromium owns TabStripModel and Browser; this only toggles the Astra sidebar
// projection layer registered through BrowserView patch points.
BASE_DECLARE_FEATURE(kAstraSidebar);

// Enables Astra workspace / Spaces metadata and tab projection.
// Chromium continues to own WebContents, TabStripModel, and session restore.
// This feature only toggles the Astra workspace service that stores metadata.
BASE_DECLARE_FEATURE(kAstraWorkspaces);

// Enables Astra split view / Glance presentation mode.
// Split view is a Views-level layout of Chromium-owned WebContents; it does
// not replace or duplicate WebContents ownership.
BASE_DECLARE_FEATURE(kAstraSplitView);

// ---------------------------------------------------------------------------
// Individual feature flags (default-disabled, experimental / planned)
// ---------------------------------------------------------------------------

// Enables the Astra command palette — a keyboard-triggered command search
// surface that surfaces both Chromium commands and Astra-only commands.
//
// Chromium owns: command infrastructure (chrome_command_ids,
// BrowserCommandController).
// Astra owns: command palette UI, search/ranking, Astra-only command IDs.
//
// TODO(astra): Build command palette Views UI and wire to command controller.
// Patch point: //chrome/browser/ui/browser_command_controller.cc
BASE_DECLARE_FEATURE(kAstraCommandPalette);

// Enables Astra favorites — a curated set of tabs / links that appear in
// the sidebar as a persistent collection.
//
// Chromium owns: bookmarks, bookmark model.
// Astra owns: favorite folder projection, sidebar presentation.
// Favorites are a lightweight layer on top of Chromium bookmarks — they
// tag certain bookmark entries as "Astra favorites" for sidebar display.
//
// TODO(astra): Implement favorite service on top of Chromium bookmarks model,
// or as Astra-specific tab metadata (AstraTabFeatures).
// Patch point: //chrome/browser/bookmarks (reuse BookmarkModel)
BASE_DECLARE_FEATURE(kAstraFavorites);

// Enables the Astra tab search surface — a quick find for open tabs across
// all windows and workspaces.
//
// Chromium owns: TabStripModel, existing tab search (chrome/browser/ui/tabs).
// Astra owns: tab search UI presentation, workspace-aware filtering.
//
// TODO(astra): Evaluate whether we need a custom tab search or can reuse
// Chromium's built-in tab search with Astra styling.
// Patch point: //chrome/browser/ui/tabs/tab_search (reuse if possible)
BASE_DECLARE_FEATURE(kAstraTabSearch);

// Enables the Astra Glance / Peek feature — a quick preview mode that
// shows a tab's content in a sidebar panel without switching tabs.
//
// Chromium owns: WebContents, rendering.
// Astra owns: Glance presentation, sidebar preview panel.
//
// Glance is a presentation-layer feature that shows a mini WebContents
// view in the sidebar for quick previewing.
BASE_DECLARE_FEATURE(kAstraGlance);

// Enables Astra focus mode — a distraction-free browsing mode that hides
// non-essential UI elements and optionally blocks distracting websites.
//
// Chromium owns: tab management, content settings.
// Astra owns: focus mode UI, distraction block list, session tracking.
BASE_DECLARE_FEATURE(kAstraFocusMode);

// Enables Astra reading list sidebar integration.
//
// Chromium owns: ReadingListModel (components/reading_list).
// Astra owns: sidebar reading list presentation and management UI.
BASE_DECLARE_FEATURE(kAstraReadingList);

// Enables Astra notes feature — quick notes attached to tabs and workspaces.
//
// Chromium owns: data persistence (via PrefService or bookmarks).
// Astra owns: notes service, sidebar notes UI.
BASE_DECLARE_FEATURE(kAstraNotes);

// Enables Astra memory saver feature — automatically suspends inactive tabs
// to free memory.
//
// Chromium owns: tab discarding (chrome/browser/resource_coordinator).
// Astra owns: memory saver UI, configuration, and sidebar presentation.
//
// TODO(astra): Evaluate reuse of Chromium's built-in tab discarding / memory
// saver before implementing a custom version.
BASE_DECLARE_FEATURE(kAstraMemorySaver);

// Enables Astra screenshot feature — capture visible area, full page, or
// selected region, with annotation tools.
//
// Chromium owns: screenshot capture infrastructure (extensions API,
// chrome/browser/screenshots).
// Astra owns: screenshot UI, annotation tools, sidebar integration.
BASE_DECLARE_FEATURE(kAstraScreenshot);

// Enables Astra Picture-in-Picture (PiP) extensions and custom PiP UI.
//
// Chromium owns: core PiP functionality (media/Video).
// Astra owns: PiP enhancements, workspace-aware PiP positioning.
BASE_DECLARE_FEATURE(kAstraPip);

// Enables Astra DevTools integration — custom Astra panels in DevTools
// for workspace management, tab stack inspection, etc.
//
// Chromium owns: DevTools infrastructure (third_party/devtools-frontend).
// Astra owns: custom DevTools panels and extensions.
BASE_DECLARE_FEATURE(kAstraDevTools);

// Enables Astra accessibility enhancements — improved screen reader support,
// keyboard navigation, and accessibility features beyond Chromium's defaults.
//
// Chromium owns: core accessibility (ui/accessibility, content/browser/accessibility).
// Astra owns: product-specific accessibility improvements and UI adjustments.
BASE_DECLARE_FEATURE(kAstraAccessibility);

// ---------------------------------------------------------------------------
// Feature list helpers
// ---------------------------------------------------------------------------

// Initializes Astra feature overrides from user preferences.
//
// Some features may have user-preference toggles in addition to the
// base::Feature flag. This function applies those pref-based overrides
// to the effective feature state.
//
// This should be called during browser startup, after the initial profile
// is created but before feature-dependent code runs.
//
// TODO(astra): Implement pref-based feature overrides. For now, feature
//   state is controlled entirely by base::FeatureList (command-line flags
//   and field trials).
//   Chromium owner: PrefService + base::FeatureList
void InitializeAstraFeaturesFromPrefs(PrefService* prefs);

// Returns true if the given Astra feature is enabled.
//
// This is a convenience wrapper around base::FeatureList::IsEnabled()
// that also checks the master kAstraBrandedBuild flag for features that
// only make sense in Astra-branded builds.
//
// Parameters:
//   feature - The feature to check.
//   requires_branding - If true, the feature is only considered enabled
//                       if kAstraBrandedBuild is also enabled.
//
// Returns true if the feature is enabled (and branding is satisfied).
bool IsAstraFeatureEnabled(const base::Feature& feature,
                           bool requires_branding = true);

// Returns a list of all Astra feature names and their current states.
//
// Useful for debugging, about:flags integration, and logging.
//
// Returns a vector of (feature_name, is_enabled) pairs.
struct AstraFeatureInfo {
  std::string name;
  bool enabled;
  std::string description;
};

std::vector<AstraFeatureInfo> GetAllAstraFeatures();

// Returns the number of enabled Astra features.
//
// Useful for metrics and telemetry.
int GetEnabledAstraFeatureCount();

// Resets Astra feature state for testing.
//
// This clears any pref-based overrides and resets to base::FeatureList
// defaults. Only for use in tests.
void ResetAstraFeatureOverridesForTesting();

// ---------------------------------------------------------------------------
// Feature state observer
// ---------------------------------------------------------------------------
//
// TODO(astra): Add a feature observer interface for code that needs to
//   react to feature state changes (e.g., dynamic about:flags toggles).
//   For now, features are fixed at startup and can't change at runtime
//   (which is the standard Chromium pattern for most features).

}  // namespace astra

#endif  // ASTRA_APP_ASTRA_FEATURE_LIST_H_
