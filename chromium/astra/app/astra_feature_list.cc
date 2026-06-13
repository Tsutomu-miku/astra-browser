// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/app/astra_feature_list.h"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "astra/build/astra_buildflags.h"
#include "base/feature_list.h"
#include "base/feature_list_buildflags.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "components/prefs/pref_service.h"

namespace astra {

namespace {

// ---------------------------------------------------------------------------
// Feature metadata table
// ---------------------------------------------------------------------------
//
// Stores human-readable descriptions for each Astra feature. This is used
// by GetAllAstraFeatures() to return feature info for UI display and
// debugging.
//
// TODO(astra): Consider using the Chromium about_flags infrastructure
//   for feature descriptions instead of a custom table.
//   Chromium owner: chrome/browser/about_flags.cc
struct AstraFeatureMetadata {
  raw_ptr<const base::Feature> feature;
  const char* description;
};

// Descriptions for each Astra feature.
// Keep in sync with the feature declarations in the header.
constexpr AstraFeatureMetadata kAstraFeatureMetadata[] = {
    {&kAstraBrandedBuild,
     "Master Astra branding flag — enables all Astra-specific UI surfaces "
     "and metadata services."},
    {&kAstraSidebar,
     "Vertical sidebar with tabs, workspaces, bookmarks, and history."},
    {&kAstraWorkspaces,
     "Workspace / Spaces feature — organize tabs into named workspaces."},
    {&kAstraSplitView,
     "Split view — show two tabs side by side in the same browser window."},
    {&kAstraCommandPalette,
     "Command palette — quick keyboard-driven command search and execution."},
    {&kAstraFavorites,
     "Favorites — curated tab collections displayed in the sidebar."},
    {&kAstraTabSearch,
     "Workspace-aware tab search across all open tabs and windows."},
    {&kAstraGlance,
     "Glance / Peek — quick tab preview in the sidebar panel."},
    {&kAstraFocusMode,
     "Focus mode — distraction-free browsing with site blocking."},
    {&kAstraReadingList,
     "Reading list sidebar integration with Chromium ReadingListModel."},
    {&kAstraNotes,
     "Notes — quick notes attached to tabs and workspaces."},
    {&kAstraMemorySaver,
     "Memory saver — automatically suspend inactive tabs to free memory."},
    {&kAstraScreenshot,
     "Screenshot — capture and annotate visible area, full page, or region."},
    {&kAstraPip,
     "Picture-in-Picture enhancements and workspace-aware PiP positioning."},
    {&kAstraDevTools,
     "Astra DevTools integration — custom panels for workspace inspection."},
    {&kAstraAccessibility,
     "Astra accessibility enhancements beyond Chromium defaults."},
};

// Total number of Astra features (for pre-allocating vectors).
constexpr size_t kAstraFeatureCount = std::size(kAstraFeatureMetadata);

// ---------------------------------------------------------------------------
// Pref-based feature override map
// ---------------------------------------------------------------------------
//
// User preferences can override the default feature state.  The override map
// stores per-feature override state that is applied on top of base::FeatureList
// (which handles command-line flags and field trials).
//
// Override priority (highest to lowest):
//   1. Pref-based forced override (enabled or disabled)
//   2. base::FeatureList (command-line, field trials, default state)
//
// TODO(astra): Wire this up to a preferences UI.  For now, the override map
//   is populated by InitializeAstraFeaturesFromPrefs() from user prefs.

// Override state for a single feature.
enum class FeatureOverrideState {
  kDefault,   // Use base::FeatureList state (no override)
  kEnabled,   // Force enabled (overrides base::FeatureList)
  kDisabled,  // Force disabled (overrides base::FeatureList)
};

// Map of feature name -> override state.
using FeatureOverrideMap = std::map<std::string, FeatureOverrideState>;

FeatureOverrideMap& GetFeatureOverrideMap() {
  static base::NoDestructor<FeatureOverrideMap> override_map;
  return *override_map;
}

// Returns the override state for a feature, or kDefault if not set.
FeatureOverrideState GetOverrideStateForFeature(const std::string& feature_name) {
  const auto& map = GetFeatureOverrideMap();
  auto it = map.find(feature_name);
  if (it != map.end()) {
    return it->second;
  }
  return FeatureOverrideState::kDefault;
}

// Gets the effective enabled state of a feature, considering both
// base::FeatureList and pref-based overrides.
bool GetEffectiveFeatureEnabled(const base::Feature& feature) {
  FeatureOverrideState override = GetOverrideStateForFeature(feature.name);

  switch (override) {
    case FeatureOverrideState::kEnabled:
      return true;
    case FeatureOverrideState::kDisabled:
      return false;
    case FeatureOverrideState::kDefault:
      return base::FeatureList::IsEnabled(feature);
  }

  NOTREACHED_NORETURN();
}

// ---------------------------------------------------------------------------
// Pref initialization flag
// ---------------------------------------------------------------------------

// Whether InitializeAstraFeaturesFromPrefs() has been called.
bool g_astra_features_initialized_from_prefs = false;

}  // namespace

// ---------------------------------------------------------------------------
// Core product features (default-enabled in Astra builds)
// ---------------------------------------------------------------------------
//
// These features are foundational to the Astra product experience and are
// enabled by default in Astra-branded builds.  They can still be disabled
// via command-line flags or user preferences.

// Master Astra branding flag.  Enabled by default only in Astra-branded
// builds.  In non-Astra builds, all Astra UI surfaces and metadata services
// are disabled at runtime even if compiled in.
//
// This provides a runtime kill switch for all Astra behavior independent of
// the BUILDFLAG(IS_ASTRA_BRANDED) compile-time flag.
BASE_FEATURE(kAstraBrandedBuild,
             "AstraBrandedBuild",
#if BUILDFLAG(IS_ASTRA_BRANDED)
             base::FEATURE_ENABLED_BY_DEFAULT);
#else
             base::FEATURE_DISABLED_BY_DEFAULT);
#endif

// Sidebar is enabled by default — it is a core Astra product surface.
// The sidebar projects Chromium TabStripModel state; it does not own tabs.
BASE_FEATURE(kAstraSidebar,
             "AstraSidebar",
             base::FEATURE_ENABLED_BY_DEFAULT);

// Workspaces are enabled by default — they are core Astra product metadata.
// Chromium owns Profile, TabStripModel, and session restore; this feature
// only controls whether workspace metadata is attached to browser state.
BASE_FEATURE(kAstraWorkspaces,
             "AstraWorkspaces",
             base::FEATURE_ENABLED_BY_DEFAULT);

// Split view is enabled by default — it is a core Astra layout feature.
// Split view is a Views-level layout of Chromium-owned WebContents; it does
// not replace or duplicate WebContents ownership.
BASE_FEATURE(kAstraSplitView,
             "AstraSplitView",
             base::FEATURE_ENABLED_BY_DEFAULT);

// ---------------------------------------------------------------------------
// Individual feature flags (default-disabled, experimental / planned)
// ---------------------------------------------------------------------------
//
// These features are disabled by default and are gradually rolled out via
// field trials or enabled by users through preferences.

// Command palette is experimental — disabled by default until the Views UI
// and command controller integration are production-ready.
BASE_FEATURE(kAstraCommandPalette,
             "AstraCommandPalette",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Favorites are experimental — disabled by default until the service layer
// and sidebar presentation are fully implemented.
BASE_FEATURE(kAstraFavorites,
             "AstraFavorites",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Tab search is planned but not yet fully implemented.
// TODO(astra): Evaluate reuse of Chromium's built-in tab search before
// implementing a custom Astra version.
BASE_FEATURE(kAstraTabSearch,
             "AstraTabSearch",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Glance / Peek is experimental — disabled by default until the sidebar
// preview panel is fully implemented.
BASE_FEATURE(kAstraGlance,
             "AstraGlance",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Focus mode is experimental — disabled by default until the focus mode
// service and UI are production-ready.
BASE_FEATURE(kAstraFocusMode,
             "AstraFocusMode",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Reading list integration is experimental — disabled by default until the
// sidebar reading list view is fully implemented.
BASE_FEATURE(kAstraReadingList,
             "AstraReadingList",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Notes are experimental — disabled by default until the notes service
// and sidebar presentation are ready.
BASE_FEATURE(kAstraNotes,
             "AstraNotes",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Memory saver is experimental — disabled by default until the service
// layer and UI are production-ready.
// TODO(astra): Evaluate reuse of Chromium's built-in tab discarding.
BASE_FEATURE(kAstraMemorySaver,
             "AstraMemorySaver",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Screenshot feature is experimental — disabled by default.
BASE_FEATURE(kAstraScreenshot,
             "AstraScreenshot",
             base::FEATURE_DISABLED_BY_DEFAULT);

// PiP enhancements are experimental — disabled by default.
BASE_FEATURE(kAstraPip,
             "AstraPip",
             base::FEATURE_DISABLED_BY_DEFAULT);

// DevTools integration is experimental — disabled by default.
BASE_FEATURE(kAstraDevTools,
             "AstraDevTools",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Accessibility enhancements are experimental — disabled by default.
BASE_FEATURE(kAstraAccessibility,
             "AstraAccessibility",
             base::FEATURE_DISABLED_BY_DEFAULT);

// ---------------------------------------------------------------------------
// Feature list helpers
// ---------------------------------------------------------------------------

void InitializeAstraFeaturesFromPrefs(PrefService* prefs) {
  // Applies user-preference overrides to Astra feature state.
  //
  // Some Astra features may have user-facing toggles in settings that
  // override the default feature state. This function reads those prefs
  // and updates the effective feature state via the override map.
  //
  // Override priority (highest to lowest):
  //   1. Pref-based forced override
  //   2. base::FeatureList (command-line, field trials, default)
  //
  // This should be called during browser startup, after the initial profile
  // is created but before feature-dependent code runs.
  //
  // TODO(astra): Implement actual pref-based feature overrides.
  //   For features that need user control, read the corresponding pref
  //   and update the override map accordingly.  Each feature should have
  //   a pref key (e.g. "astra.features.AstraSidebar") with an int value:
  //     0 = default (use base::FeatureList)
  //     1 = force enabled
  //     2 = force disabled
  //   Chromium pattern: PrefService + feature flag preferences
  //   Astra owner: astra/browser/astra_prefs.h (pref key definitions)

  DVLOG(1) << "Initializing Astra features from prefs...";

  // Clear any existing overrides before applying new ones.
  GetFeatureOverrideMap().clear();

  if (!prefs) {
    DLOG(WARNING) << "InitializeAstraFeaturesFromPrefs called with null prefs — "
                  << "using base::FeatureList defaults.";
    g_astra_features_initialized_from_prefs = false;
    return;
  }

  // TODO(astra): Read per-feature override prefs and populate the
  //   override map.  For example:
  //
  //   for (const auto& entry : kAstraFeatureMetadata) {
  //     std::string pref_key = GetPrefKeyForFeature(entry.feature->name);
  //     int value = prefs->GetInteger(pref_key);
  //     if (value == 1) {
  //       GetFeatureOverrideMap()[entry.feature->name] =
  //           FeatureOverrideState::kEnabled;
  //     } else if (value == 2) {
  //       GetFeatureOverrideMap()[entry.feature->name] =
  //           FeatureOverrideState::kDisabled;
  //     }
  //   }
  //
  // For now, no pref-based overrides are applied — all features use
  // their base::FeatureList default state.

  g_astra_features_initialized_from_prefs = true;

  DVLOG(1) << "Astra features initialized from prefs.  "
           << GetFeatureOverrideMap().size() << " overrides applied.";
}

bool IsAstraFeatureEnabled(const base::Feature& feature,
                           bool requires_branding /*= true*/) {
  // Returns whether |feature| is effectively enabled.
  //
  // This considers both base::FeatureList state and any pref-based
  // overrides.
  //
  // If |requires_branding| is true (the default), the feature is only
  // considered enabled if kAstraBrandedBuild is also enabled.  This ensures
  // that in non-Astra builds, no Astra features are active even if their
  // individual flags are somehow enabled.
  //
  // Most Astra features require branding.  The only exception is features
  // designed to work in Chrome builds too (which should be rare).
  //
  // Parameters:
  //   feature - The feature to check.
  //   requires_branding - If true, also requires kAstraBrandedBuild to be on.
  //
  // Returns true if the feature is effectively enabled.

  if (requires_branding && !GetEffectiveFeatureEnabled(kAstraBrandedBuild)) {
    return false;
  }

  return GetEffectiveFeatureEnabled(feature);
}

std::vector<AstraFeatureInfo> GetAllAstraFeatures() {
  // Returns information about all Astra features.
  //
  // This is useful for:
  //   - Debugging UIs
  //   - about:flags integration
  //   - Logging feature state at startup
  //   - Telemetry / metrics reporting
  //
  // The returned vector is sorted by feature name for consistent ordering.
  // Each entry includes the feature name, effective enabled state, and
  // a human-readable description.

  std::vector<AstraFeatureInfo> features;
  features.reserve(kAstraFeatureCount);

  for (const auto& metadata : kAstraFeatureMetadata) {
    DCHECK(metadata.feature);

    AstraFeatureInfo info;
    info.name = metadata.feature->name;
    info.enabled = GetEffectiveFeatureEnabled(*metadata.feature);
    info.description = metadata.description ? metadata.description : "";
    features.push_back(info);
  }

  // Sort by feature name for deterministic ordering.
  std::sort(features.begin(), features.end(),
            [](const AstraFeatureInfo& a, const AstraFeatureInfo& b) {
              return a.name < b.name;
            });

  return features;
}

int GetEnabledAstraFeatureCount() {
  // Returns the number of Astra features that are currently enabled.
  //
  // This counts features whose effective state is enabled (considering
  // both base::FeatureList and pref overrides).  It does NOT apply the
  // branding gate — use this for metrics and telemetry where you want the
  // raw count of enabled feature flags.
  //
  // Returns the count of enabled features (0 to kAstraFeatureCount).

  int count = 0;
  for (const auto& metadata : kAstraFeatureMetadata) {
    if (GetEffectiveFeatureEnabled(*metadata.feature)) {
      ++count;
    }
  }
  return count;
}

void ResetAstraFeatureOverridesForTesting() {
  // Resets all pref-based feature overrides back to default.
  //
  // This clears the override map so that features return to their
  // base::FeatureList state.  Only for use in unit tests and browser tests.
  //
  // Note: base::FeatureList itself is not resettable in production builds.
  // Tests should use base::test::ScopedFeatureList to override features,
  // and call this function to clear any pref-based overrides.
  //
  // TODO(astra): Use base::FeatureList::ScopedDisallowOverrides in tests
  //   that need to ensure feature state is not modified unexpectedly.

  DVLOG(1) << "ResetAstraFeatureOverridesForTesting() — clearing override map.";

  GetFeatureOverrideMap().clear();
  g_astra_features_initialized_from_prefs = false;
}

}  // namespace astra
