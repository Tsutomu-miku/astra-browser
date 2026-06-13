// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_ACCESSIBILITY_ASTRA_ACCESSIBILITY_IDS_H_
#define ASTRA_UI_ACCESSIBILITY_ASTRA_ACCESSIBILITY_IDS_H_

namespace astra {
namespace accessibility {

// =========================================================================
// Astra Accessibility Identifiers
// =========================================================================
//
// Enum-style constants for Astra-specific accessibility IDs used in screen
// reader announcements, custom actions, and Astra UI element identification.
//
// These IDs are in a range starting well after Chromium's built-in AX IDs
// to avoid collisions.  Chromium's built-in AX IDs are defined in
// ui/accessibility/ax_enums.mojom and are generated as sequential enums.
//
// For custom AX actions, Chromium uses ax::mojom::Action which has a
// kCustomAction value that can be paired with a custom action ID string.
// For announced events and UI element identification, we use these
// Astra-specific IDs.
//
// TODO(astra): Align these IDs with Chromium's AX ID allocation strategy.
//   Chromium's AX tree uses int32_t IDs assigned by AXTreeID and AXNodeID.
//   For custom actions, use AXCustomAction objects with string IDs.
//   For announcement identifiers, use string attributes on live regions.
//   These enum values serve as logical IDs for Astra UI components.
// Chromium owner: ui/accessibility/ax_enums.mojom.h
// Chromium owner: ui/accessibility/ax_node_data.h (custom_action_id attribute)
// =========================================================================

// Base offset for Astra accessibility IDs.
// Starts at 10000 to leave ample room for Chromium's built-in values.
//
// TODO(astra): Verify that 10000 is safely above all Chromium built-in
//   AX enum values.  The actual Chromium AX enums are mojom-generated
//   and their numeric values depend on declaration order in
//   ui/accessibility/ax_enums.mojom.
// Chromium source: ui/accessibility/ax_enums.mojom
constexpr int kAstraAccessibilityIdBase = 10000;

// Astra UI element accessibility IDs.
// These identify specific Astra UI components in the accessibility tree.
// They are used as role descriptions, group labels, and custom attributes
// to help screen reader users understand Astra-specific UI structure.
enum class AstraAccessibilityId {
  // Sidebar and navigation
  kSidebarContainer = kAstraAccessibilityIdBase + 1,
  kSidebarItem = kAstraAccessibilityIdBase + 2,
  kSidebarSection = kAstraAccessibilityIdBase + 3,
  kSidebarFavoritesSection = kAstraAccessibilityIdBase + 4,
  kSidebarHistorySection = kAstraAccessibilityIdBase + 5,
  kSidebarDownloadsSection = kAstraAccessibilityIdBase + 6,
  kSidebarExtensionsSection = kAstraAccessibilityIdBase + 7,

  // Spaces / workspaces
  kSpaceSelector = kAstraAccessibilityIdBase + 20,
  kSpaceItem = kAstraAccessibilityIdBase + 21,
  kSpaceSwitcher = kAstraAccessibilityIdBase + 22,

  // Tab and split view
  kSplitViewContainer = kAstraAccessibilityIdBase + 40,
  kSplitViewDivider = kAstraAccessibilityIdBase + 41,
  kTabGroup = kAstraAccessibilityIdBase + 42,
  kFavoriteTab = kAstraAccessibilityIdBase + 43,

  // Glance / preview
  kGlancePreview = kAstraAccessibilityIdBase + 60,
  kGlanceTooltip = kAstraAccessibilityIdBase + 61,

  // Command palette
  kCommandPalette = kAstraAccessibilityIdBase + 80,
  kCommandPaletteItem = kAstraAccessibilityIdBase + 81,

  // Status and announcements
  kStatusAnnouncement = kAstraAccessibilityIdBase + 100,
  kAlertAnnouncement = kAstraAccessibilityIdBase + 101,
  kLiveRegionStatus = kAstraAccessibilityIdBase + 102,

  // Custom actions
  kActionPinTab = kAstraAccessibilityIdBase + 120,
  kActionUnpinTab = kAstraAccessibilityIdBase + 121,
  kActionMoveToSpace = kAstraAccessibilityIdBase + 122,
  kActionAddToFavorites = kAstraAccessibilityIdBase + 123,
  kActionRemoveFromFavorites = kAstraAccessibilityIdBase + 124,
  kActionSplitView = kAstraAccessibilityIdBase + 125,
  kActionCloseSplit = kAstraAccessibilityIdBase + 126,

  // Focus rings and highlights
  kFocusRingSidebar = kAstraAccessibilityIdBase + 140,
  kFocusRingWidget = kAstraAccessibilityIdBase + 141,
  kFocusRingBubble = kAstraAccessibilityIdBase + 142,
};

// Returns a string representation of the accessibility ID.
// Used for custom action IDs and debugging output.
//
// TODO(astra): Consider using std::string_view or base::StringPiece
//   for these string constants to avoid allocations.
// Chromium pattern: ui/accessibility/ax_enum_util.h (ToString functions)
const char* AstraAccessibilityIdToString(AstraAccessibilityId id);

}  // namespace accessibility
}  // namespace astra

#endif  // ASTRA_UI_ACCESSIBILITY_ASTRA_ACCESSIBILITY_IDS_H_
