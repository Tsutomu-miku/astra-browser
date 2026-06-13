// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_COMMON_ASTRA_WORKSPACE_TYPES_H_
#define ASTRA_COMMON_ASTRA_WORKSPACE_TYPES_H_

#include <string>
#include <vector>

#include "base/time/time.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/geometry/rect.h"

namespace astra {

// Opaque identifier for an Astra workspace.
//
// Currently a std::string for simplicity and easy serialization through
// Chromium's PrefService.  May be upgraded to base::Uuid later if we need
// stronger type safety or collision resistance.
//
// TODO(astra): Evaluate migrating to base::Uuid for workspace IDs once the
//   metadata model stabilizes.  base::Uuid provides stronger type safety and
//   standardized serialization.  Chromium component: base/uuid/uuid.h.
using AstraWorkspaceId = std::string;

// Opaque identifier for a window within a workspace.
using AstraWorkspaceWindowId = std::string;

// =========================================================================
// Workspace layout
// =========================================================================

// Layout mode for tabs within a workspace.
//
// kDefault: Standard tab strip layout (default Chromium behavior).
// kTiled:   Tabs are tiled side-by-side (auto split-view).
// kStacked: Tabs are stacked vertically (Arc-style).
// kGrid:    Tabs arranged in a grid layout.
// kFocus:   Single tab focus mode (hides other tabs).
enum class AstraWorkspaceLayout {
  kDefault,
  kTiled,
  kStacked,
  kGrid,
  kFocus,
};

// =========================================================================
// Workspace color palette
// =========================================================================

// Named workspace colors — a curated 12-color palette.
//
// Each workspace has a color used for UI decoration (sidebar highlight,
// workspace switcher badges, window tint, etc.).  The 12-color palette
// provides enough variety for most users while keeping the UI consistent.
//
// The numeric values are stable — they persist in user preferences.
// Do not reorder or renumber existing entries.
//
// TODO(astra): Consider aligning this with the existing
//   AstraWorkspaceAccentColor enum, or replacing that enum with this one.
//   Chromium owner: theme / color team.
enum class AstraWorkspaceColor {
  kGray = 0,
  kBlue,
  kRed,
  kGreen,
  kYellow,
  kPurple,
  kPink,
  kCyan,
  kOrange,
  kTeal,
  kIndigo,
  kBrown,
};

// =========================================================================
// Workspace accent colors (legacy)
// =========================================================================

// Predefined accent colors for Astra workspaces.
//
// Each workspace has an accent color used for UI decoration (sidebar
// highlight, workspace switcher badges, window tint, etc.).  Predefined
// colors are curated to match Astra's design system; a "custom" variant
// allows arbitrary SkColor values when needed.
//
// The numeric values are stable — they persist in user preferences.
// Do not reorder or renumber existing entries.
//
// TODO(astra): Consider unifying with AstraWorkspaceColor.  This enum
//   provides a "custom" variant while the 12-color palette does not.
enum class AstraWorkspaceAccentColor {
  kBlue = 0,
  kGreen,
  kPurple,
  kOrange,
  kPink,
  kRed,
  kTeal,
  kYellow,
  kGrey,
  kCustom,  // User-specified custom color; see custom_color() on AstraWorkspaceInfo.
};

// Returns the default SkColor for a predefined accent color enum value.
// For kCustom, returns a fallback (blue) — callers should use the custom
// color stored in AstraWorkspaceInfo instead.
//
// TODO(astra): These color values are placeholders.  Finalize the palette
//   with design and integrate with Chromium's color system (ui/color) for
//   proper light/dark theme support.  Chromium component: ui/color/color_provider.h.
SkColor GetAstraAccentColor(AstraWorkspaceAccentColor color);

// Returns the SkColor for a given AstraWorkspaceColor enum value.
//
// Colors are sourced from a curated Material Design palette.
// Use this function to convert the enum to a concrete color for rendering.
//
// TODO(astra): Finalize color values with design.  Consider using
//   Material Color Utilities for tonal palette generation.
//   Chromium component: third_party/material_color_utilities.
SkColor AccentColorForWorkspaceColor(AstraWorkspaceColor color);

// =========================================================================
// Workspace window state
// =========================================================================

// Window state within a workspace.
//
// Lightweight struct describing a single window's state within a workspace.
// Used for workspace persistence and restoration.
//
// This is a data-only type — window management logic lives in the browser
// layer (AstraWorkspaceService), and window creation/restoration uses
// Chromium's Browser and BrowserList infrastructure.
struct AstraWorkspaceWindowState {
  // Opaque window identifier.
  AstraWorkspaceWindowId window_id;

  // Window bounds in screen coordinates (DIPs).
  gfx::Rect bounds;

  // Whether the window is minimized.
  bool is_minimized = false;

  // Whether the window is maximized.
  bool is_maximized = false;

  // Whether the window is in fullscreen mode.
  bool is_fullscreen = false;

  // Index of the active tab in this window's tab strip.
  // -1 means no active tab (empty window).
  int active_tab_index = -1;
};

// =========================================================================
// Workspace info
// =========================================================================

// Lightweight metadata describing an Astra workspace.
//
// This is a pure data struct — no logic, no observers, no persistence.
// It carries the canonical workspace metadata fields that are shared across
// the app/browser/ui layers.
//
// Chromium owns: nothing here — this is Astra product metadata.
// Persistence: through AstraWorkspaceService (PrefService-backed).
struct AstraWorkspaceInfo {
  AstraWorkspaceId id;
  std::u16string name;

  // --- Visual properties ---

  // Accent color (legacy enum with custom support).
  AstraWorkspaceAccentColor accent_color = AstraWorkspaceAccentColor::kBlue;

  // Custom color value, only meaningful when accent_color == kCustom.
  SkColor custom_color = SK_ColorBLUE;

  // Workspace color from the 12-color palette.
  // Used for quick visual identification in the workspace switcher.
  AstraWorkspaceColor color = AstraWorkspaceColor::kBlue;

  // Icon identifier (e.g. emoji or material icon name).
  // Empty string means use the default icon.
  std::string icon;

  // Layout mode for this workspace.
  AstraWorkspaceLayout layout = AstraWorkspaceLayout::kDefault;

  // --- State properties ---

  // Sort position in the workspace list (0-based).
  size_t order_index = 0;

  // True if this is the default workspace (cannot be deleted).
  bool is_default = false;

  // True if this workspace is pinned to the top of the list.
  bool is_pinned = false;

  // Number of tabs across all windows in this workspace.
  // Cached for quick display in the workspace switcher.
  int tab_count = 0;

  // Number of windows in this workspace.
  int window_count = 0;

  // --- Time properties ---

  // When the workspace was created.
  base::Time created_time;

  // Last time the workspace was accessed (switched to).
  base::Time last_accessed_time;
};

// Ordered list of workspace metadata entries.
// Sorted by order_index ascending.
using AstraWorkspaceList = std::vector<AstraWorkspaceInfo>;

// List of window state entries for a workspace.
using AstraWorkspaceWindowList = std::vector<AstraWorkspaceWindowState>;

// =========================================================================
// ID validation
// =========================================================================

// Validates a workspace ID format.
//
// A valid workspace ID must:
//   - Be non-empty.
//   - Contain only alphanumeric characters, underscores, and hyphens.
//   - Be at most 64 characters long.
//
// This is a format check only — it does not verify that the ID corresponds
// to an existing workspace.
//
// TODO(astra): When migrating to base::Uuid, replace this with Uuid
//   validation.  Chromium component: base/uuid/uuid.h.
bool IsValidWorkspaceId(const std::string& id);

// =========================================================================
// Constants
// =========================================================================

// Sentinel value for an invalid/unknown workspace ID.
// TODO(astra): Use base::Uuid's nil/empty concept once migrated to Uuid.
inline const char kAstraInvalidWorkspaceId[] = "";

// ID of the default workspace.  Always exists.
inline const char kAstraDefaultWorkspaceId[] = "default";

// Maximum length of a workspace ID.
inline constexpr size_t kAstraMaxWorkspaceIdLength = 64;

}  // namespace astra

#endif  // ASTRA_COMMON_ASTRA_WORKSPACE_TYPES_H_
