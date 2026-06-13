// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_COMMON_ASTRA_TAB_TYPES_H_
#define ASTRA_COMMON_ASTRA_TAB_TYPES_H_

#include <string>

#include "base/time/time.h"
#include "base/types/bit_flag.h"

namespace astra {

// Opaque identifier for a favorite folder.
// "root" refers to the top-level favorites bar.
using AstraFavoriteFolderId = std::string;

// Opaque identifier for a tab stack.
// Empty string means the tab is not in any stack.
using AstraTabStackId = std::string;

// Orientation of a split view pair.
//
// kHorizontal: tabs side-by-side (left / right).
// kVertical:   tabs stacked (top / bottom).
//
// This enum lives in the common layer so it can be used by both
// AstraTabFeatures (browser layer) and AstraSplitView (ui/views layer)
// without creating a dependency from views on tab_features internals.
enum class AstraSplitViewOrientation {
  kHorizontal,
  kVertical,
};

// Split view state for a tab.
//
// Lightweight struct carrying the split-view configuration of a tab.
// When is_active is false, the other fields are undefined.
//
// This is a data-only type — the actual split view controller and view
// live in the ui/views layer, and the per-tab metadata storage lives in
// AstraTabFeatures (browser layer).  Both use this struct to exchange
// state without coupling their implementation details.
struct AstraSplitViewState {
  // True when the tab is currently part of a split view pair.
  bool is_active = false;

  // Layout orientation of the split view.
  AstraSplitViewOrientation orientation = AstraSplitViewOrientation::kHorizontal;

  // Opaque identifier of the partner tab.  Empty when not in split view.
  // Stored as a string to avoid coupling to any particular tab handle type;
  // the actual lookup is done by the split view controller using
  // WebContents or TabStripModel indices.
  //
  // TODO(astra): Consider using base::Token instead of std::string for the
  //   partner identifier once Astra tab identity stabilizes.
  //   Chromium component: base/token.h.
  std::string partner_tab_id;

  // Split ratio in [0.0, 1.0].  0.5 means equal split.
  // For horizontal orientation this is the width fraction of the left tab;
  // for vertical orientation this is the height fraction of the top tab.
  float ratio = 0.5f;
};

// =========================================================================
// Tab stack state
// =========================================================================

// Stacking state of a tab within a tab stack.
//
// kNormal:    Tab is not stacked (default, standalone tab).
// kStacked:   Tab is part of a stack but has no special role.
// kStackRoot: Tab is the root/parent of a stack (visible when stack is
//             collapsed; clicking it expands or collapses the stack).
// kStackChild: Tab is a child within a stack (only visible when the stack
//              is expanded).
//
// TODO(astra): Integrate with Chromium's tab groups feature as an
//   alternative presentation.  Chromium component: tab_groups.
//   Patch point: chrome/browser/ui/tabs/tab_strip_model.cc.
enum class AstraTabStackState {
  kNormal,
  kStacked,
  kStackRoot,
  kStackChild,
};

// =========================================================================
// Tab source
// =========================================================================

// The origin / cause of a tab being opened.
//
// Used for metrics, heuristics (e.g. "should this tab auto-stack?"),
// and close behavior decisions.
//
// kUserOpened:      Explicit user action (Ctrl+T, new tab button, etc.).
// kRestore:         Restored from session restore or workspace switch.
// kLinkClick:       Opened by clicking a link in another page.
// kPopup:           Opened as a popup window / tab.
// kExtension:       Opened by a browser extension.
// kDevTools:        Opened by DevTools (e.g. inspect element in new tab).
// kWorkspaceSwitch: Opened as part of switching to a workspace.
enum class AstraTabSource {
  kUserOpened,
  kRestore,
  kLinkClick,
  kPopup,
  kExtension,
  kDevTools,
  kWorkspaceSwitch,
};

// =========================================================================
// Tab close behavior
// =========================================================================

// What happens when a tab that is part of a stack is closed.
//
// kDefault:     Use the default behavior (close the tab, adjust stack).
// kKeepInStack: Keep the tab in the stack (e.g. hibernate instead of close).
// kCloseStack:  Close the entire stack when this tab is closed.
// kHibernate:   Hibernate the tab instead of closing (preserves state).
enum class AstraTabCloseBehavior {
  kDefault,
  kKeepInStack,
  kCloseStack,
  kHibernate,
};

// =========================================================================
// Tab feature flags
// =========================================================================

// Bit-flags for Astra-specific tab features.
//
// These flags represent boolean tab properties that are frequently checked
// together (e.g. for UI styling, command enablement).  They are stored as a
// bitmask for efficient bulk queries.
//
// The actual per-tab state storage lives in AstraTabFeatures
// (browser-layer WebContentsUserData).  This flag set is the common-layer
// projection used across app/browser/ui boundaries.
//
// TODO(astra): Add more feature flags as Astra tab features expand.
//   Keep this enum in sync with the accessors on AstraTabFeatures.
//   Chromium component pattern: base/types/bit_flag.h for bit-flag utilities.
enum class AstraTabFeatureFlag : uint32_t {
  kNone = 0,

  // --- First byte (bits 0-7): core Astra features ---

  kFavorite = 1 << 0,       // Tab is in the favorites section.
  kPinned = 1 << 1,         // Tab is pinned (tab strip pin).
  kGlanceTab = 1 << 2,      // Tab is displayed as a glance/peek preview.
  kInStack = 1 << 3,        // Tab is stacked under a parent tab.
  kInSplitView = 1 << 4,    // Tab is part of a split view pair.
  kPipTab = 1 << 5,         // Tab is in picture-in-picture mode.
  kSuspended = 1 << 6,      // Tab is suspended/discarded (memory saver).
  kSidebarHidden = 1 << 7,  // Tab is hidden from the sidebar tab list.

  // --- Second byte (bits 8-15): content and state flags ---

  kReadingList = 1 << 8,     // Tab is saved to the reading list.
  kNote = 1 << 9,            // Tab has an associated user note.
  kPinnedInSidebar = 1 << 10, // Tab is pinned in the sidebar section.
  kGlancePreview = 1 << 11,  // Tab shows a glance preview on hover.
  kAutoDiscardable = 1 << 12,// Tab can be auto-discarded by memory saver.
  kMuted = 1 << 13,          // Tab audio is muted.
  kAudible = 1 << 14,        // Tab is currently playing audio.
  kHibernated = 1 << 15,     // Tab is hibernated (deep suspension).
};

// Bitmask type combining AstraTabFeatureFlag values.
//
// Use base::HasFlag / base::SetFlag / base::ClearFlag helpers from
// base/types/bit_flag.h to manipulate flags.
using AstraTabFeatureFlags = uint32_t;

// =========================================================================
// Tab metadata
// =========================================================================

// Extended metadata for an Astra tab.
//
// This struct carries the Astra-specific tab metadata that is shared across
// layers.  It is a pure data carrier — no logic, no observers.
//
// The actual per-tab state storage lives in AstraTabFeatures
// (browser-layer WebContentsUserData).  This struct is the common-layer
// projection used for data exchange across app/browser/ui boundaries.
//
// Chromium owns: core tab state (WebContents, navigation, etc.).
// Astra owns: stack identity, source tracking, close behavior, access stats.
struct AstraTabMetadata {
  // ID of the tab stack this tab belongs to.
  // Empty string means the tab is not in any stack.
  AstraTabStackId stack_id;

  // How / why this tab was opened.
  AstraTabSource source = AstraTabSource::kUserOpened;

  // What happens when this tab is closed (especially in a stack context).
  AstraTabCloseBehavior close_behavior = AstraTabCloseBehavior::kDefault;

  // Last time the tab was actively accessed (foregrounded, navigated, etc.).
  base::Time last_access_time;

  // Number of times this tab has been visited / accessed.
  // Used for recency and usage heuristics.
  int visit_count = 0;

  // Cache key for the tab thumbnail image.
  // Empty string means no thumbnail is available.
  std::string thumbnail_cache_key;
};

// Root folder ID for the top-level favorites bar.
inline const char kAstraFavoriteRootFolderId[] = "root";

// Default split view ratio (equal split).
inline constexpr float kAstraDefaultSplitViewRatio = 0.5f;

// Minimum split view ratio (prevents a pane from becoming too small).
// TODO(astra): Finalize minimum ratio with design.  Consider making this
//   a preference or theme value.  Chromium component: components/prefs.
inline constexpr float kAstraMinSplitViewRatio = 0.15f;

// Maximum split view ratio (complement of minimum).
inline constexpr float kAstraMaxSplitViewRatio =
    1.0f - kAstraMinSplitViewRatio;

// Sentinel value for an invalid / empty tab stack ID.
inline const char kAstraInvalidTabStackId[] = "";

}  // namespace astra

#endif  // ASTRA_COMMON_ASTRA_TAB_TYPES_H_
