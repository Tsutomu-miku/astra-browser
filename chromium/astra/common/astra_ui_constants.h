// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_COMMON_ASTRA_UI_CONSTANTS_H_
#define ASTRA_COMMON_ASTRA_UI_CONSTANTS_H_

#include "ui/gfx/geometry/size.h"

namespace astra {

// =========================================================================
// Sidebar dimensions
// =========================================================================
//
// The sidebar is Astra's primary navigation surface.  These constants
// define the default and bounds of the sidebar width, shared between the
// browser layer (persistence, pref defaults) and the ui/views layer
// (layout, resizing).

// Default width of the sidebar in DIPs.
// TODO(astra): Finalize sidebar default width with design.
//   Chromium component: ui/gfx for DIP units, ui/views for layout.
inline constexpr int kAstraSidebarDefaultWidth = 280;

// Minimum width of the sidebar in DIPs (enforced during resize).
inline constexpr int kAstraSidebarMinWidth = 200;

// Maximum width of the sidebar in DIPs (enforced during resize).
inline constexpr int kAstraSidebarMaxWidth = 480;

// Width of the sidebar when collapsed (icon-only mode) in DIPs.
// TODO(astra): Implement collapsed / icon-only sidebar mode.
inline constexpr int kAstraSidebarCollapsedWidth = 56;

// Default number of sidebar sections (favorites, tabs, workspace, etc.).
inline constexpr int kAstraSidebarDefaultSections = 5;

// Height of a standard sidebar row in DIPs.
// TODO(astra): Finalize sidebar row height with design.
//   Chromium analog: BookmarkBarView row height, TabStrip tab height.
inline constexpr int kAstraSidebarRowHeight = 36;

// Height of a sidebar section header in DIPs.
inline constexpr int kAstraSidebarSectionHeaderHeight = 28;

// Vertical padding inside a sidebar section in DIPs.
inline constexpr int kAstraSidebarSectionPadding = 8;

// =========================================================================
// Split view
// =========================================================================

// Default split ratio — equal split.
// Duplicated here from astra_tab_types.h for convenience in UI code.
inline constexpr float kAstraSplitViewDefaultRatio = 0.5f;

// Minimum split view ratio (prevents a pane from becoming too small).
inline constexpr float kAstraSplitViewMinRatio = 0.15f;

// Maximum split view ratio (complement of minimum).
inline constexpr float kAstraSplitViewMaxRatio =
    1.0f - kAstraSplitViewMinRatio;

// Minimum size of a split view pane in DIPs.
// Used by the split view controller to clamp the divider position.
// TODO(astra): Finalize minimum pane size with design.
inline constexpr int kAstraSplitViewMinPaneSize = 150;

// Thickness of the split view divider (draggable handle) in DIPs.
inline constexpr int kAstraSplitViewDividerThickness = 4;

// =========================================================================
// Command palette
// =========================================================================

// Default size of the command palette bubble in DIPs.
// TODO(astra): Finalize command palette dimensions with design.
//   Chromium component: ui/views/bubble/bubble_dialog_delegate_view.h.
inline constexpr gfx::Size kAstraCommandPaletteDefaultSize(600, 480);

// Maximum height of the command palette bubble in DIPs.
inline constexpr int kAstraCommandPaletteMaxHeight = 600;

// Maximum number of search results shown in the command palette.
inline constexpr int kAstraCommandPaletteMaxResults = 10;

// Maximum number of command history items stored.
inline constexpr int kAstraCommandPaletteMaxHistoryItems = 50;

// =========================================================================
// Workspace card
// =========================================================================

// Default width of a workspace card in the overview in DIPs.
inline constexpr int kAstraWorkspaceCardWidth = 240;

// Default height of a workspace card in the overview in DIPs.
inline constexpr int kAstraWorkspaceCardHeight = 160;

// Spacing between workspace cards in the overview grid in DIPs.
inline constexpr int kAstraWorkspaceCardSpacing = 16;

// Padding inside a workspace card in DIPs.
inline constexpr int kAstraWorkspaceCardPadding = 12;

// Corner radius of a workspace card in DIPs.
inline constexpr int kAstraWorkspaceCardCornerRadius = 12;

// =========================================================================
// Corner radii
// =========================================================================
//
// Standard corner radius values used across Astra UI surfaces.
// Follows a stepped scale similar to Chromium's md_style::k*CornerRadius.

// No corner radius (sharp corners).
inline constexpr int kAstraRadiusNone = 0;

// Small corner radius — for compact UI elements (chips, badges).
inline constexpr int kAstraRadiusSmall = 4;

// Medium corner radius — for cards, bubbles, sidebar items.
inline constexpr int kAstraRadiusMedium = 8;

// Large corner radius — for dialogs, panels, overview cards.
inline constexpr int kAstraRadiusLarge = 12;

// Pill / fully rounded corners — for buttons, chips, toggle switches.
inline constexpr int kAstraRadiusPill = 999;

// =========================================================================
// Spacing units
// =========================================================================
//
// Standard spacing values used across Astra UI surfaces.
// Based on a 4px unit grid, consistent with Chromium's MD spacing.

// No spacing.
inline constexpr int kAstraSpacingNone = 0;

// 4px — smallest spacing unit (icon-to-text padding, tight insets).
inline constexpr int kAstraSpacingTiny = 4;

// 8px — small spacing (between related items, internal padding).
inline constexpr int kAstraSpacingSmall = 8;

// 16px — medium spacing (section gaps, card padding).
inline constexpr int kAstraSpacingMedium = 16;

// 24px — large spacing (between major sections, window insets).
inline constexpr int kAstraSpacingLarge = 24;

// 32px — extra large spacing (page-level margins, hero sections).
inline constexpr int kAstraSpacingHuge = 32;

// =========================================================================
// Font sizes
// =========================================================================
//
// Standard font size values used across Astra UI surfaces.
// All values are in DIPs (device-independent pixels).

// Tiny font size — 11px (captions, timestamps, metadata).
inline constexpr int kAstraFontSizeTiny = 11;

// Small font size — 13px (secondary text, descriptions).
inline constexpr int kAstraFontSizeSmall = 13;

// Medium / body font size — 14px (body text, list items).
inline constexpr int kAstraFontSizeMedium = 14;

// Large font size — 16px (subsection headers, emphasized body).
inline constexpr int kAstraFontSizeLarge = 16;

// Title font size — 18px (card titles, section headers).
inline constexpr int kAstraFontSizeTitle = 18;

// Headline font size — 20px (page headlines, dialog titles).
inline constexpr int kAstraFontSizeHeadline = 20;

// Display font size — 24px (hero text, large numbers).
inline constexpr int kAstraFontSizeDisplay = 24;

// Giant font size — 32px (empty state icons, big numbers).
inline constexpr int kAstraFontSizeGiant = 32;

// =========================================================================
// Animation
// =========================================================================

// Short / fast animation duration in milliseconds — for micro-interactions
// (hover state, icon swap, checkbox toggle).
inline constexpr int kAstraAnimationFastMs = 120;

// Default / medium animation duration in milliseconds — for standard
// transitions (sidebar collapse, panel slide, bubble show).
inline constexpr int kAstraAnimationDefaultMs = 200;

// Slow / long animation duration in milliseconds — for major transitions
// (workspace switch, overview enter/exit).
inline constexpr int kAstraAnimationSlowMs = 350;

// =========================================================================
// Icon sizes
// =========================================================================

// Small icon size — 16x16 DIPs (inline with text, badges).
inline constexpr int kAstraIconSizeSmall = 16;

// Medium icon size — 20x20 DIPs (sidebar items, toolbar buttons).
inline constexpr int kAstraIconSizeMedium = 20;

// Large icon size — 24x24 DIPs (primary action buttons).
inline constexpr int kAstraIconSizeLarge = 24;

// Extra large icon size — 32x32 DIPs (workspace avatars, empty states).
inline constexpr int kAstraIconSizeExtraLarge = 32;

// =========================================================================
// Elevation / shadow levels
// =========================================================================
//
// Standard elevation levels used across Astra UI surfaces.
// Each level corresponds to a shadow elevation in DIPs.

// Level 0 — no shadow (flat, flush with surface).
inline constexpr int kAstraElevationLevel0 = 0;

// Level 1 — subtle shadow (cards, sidebar items at rest).
inline constexpr int kAstraElevationLevel1 = 2;

// Level 2 — medium shadow (popups, hovered cards).
inline constexpr int kAstraElevationLevel2 = 4;

// Level 3 — prominent shadow (dialogs, command palette).
inline constexpr int kAstraElevationLevel3 = 8;

// Level 4 — maximum shadow (drag previews, modal overlays).
inline constexpr int kAstraElevationLevel4 = 16;

}  // namespace astra

#endif  // ASTRA_COMMON_ASTRA_UI_CONSTANTS_H_
