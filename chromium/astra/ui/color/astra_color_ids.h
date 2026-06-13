// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Astra-specific color IDs for the Chromium ColorProvider system.
//
// Astra extends Chromium's color system with product-specific color tokens.
// All Astra colors are registered via AstraColorMixer and resolved through
// the standard ui::ColorProvider interface.
//
// Chromium subsystem: ui::ColorProvider (ui/color/color_provider.h)
// Chromium owner: UI / NativeTheme team
// Astra owner: UI Color System (astra/ui/color/)
//
// Naming convention (per ADR-0022): kColorAstra*
//   - Follows Chromium's kColor* prefix convention for all color IDs.
//   - "Astra" in the middle clearly identifies Astra-specific color tokens.
//   - Range markers use kAstraColorsStart / kAstraColorsEnd, matching the
//     kUiColorsStart / kChromeColorsStart pattern used by Chromium and Chrome.
//
// Usage in views:
//
//   #include "astra/ui/color/astra_color_ids.h"
//
//   SkColor bg = GetColorProvider()->GetColor(kColorAstraSidebarBackground);
//
// How colors are wired into Chromium:
//
//   1. AstraColorMixer is added to each ColorProvider during browser window
//      initialization. The mixer registers Astra-specific ColorIds with
//      light/dark variants and accent-color-derived values.
//
//   2. TODO(astra): Add AstraColorMixer to NativeChromeColorMixer or the
//      appropriate ColorProviderProvider. Patch point:
//        - chrome/browser/ui/color/native_chrome_color_mixer.cc
//        - chrome/browser/ui/color/chrome_color_mixers.h
//      Call AddAstraColorMixer() inside #if BUILDFLAG(IS_ASTRA_BRANDED).
//
//   3. Accent color comes from AstraThemeService or workspace settings.
//      When the accent color changes, the ColorProvider is rebuilt so all
//      dependent Astra colors update automatically.
//
// Color ID numbering:
//   - Astra color IDs start at kAstraColorsStart = kUiColorsEnd.
//   - This avoids collisions with Chromium's built-in color IDs.
//   - New colors must be added between kAstraColorsStart and kAstraColorsEnd.
//
//   TODO(astra): Consider using kChromeColorsEnd as the base instead of
//     kUiColorsEnd, since Astra lives at the Chrome layer. If we add Astra
//     colors on top of UI colors but Chrome also adds colors on top of UI
//     colors, there will be collisions. Using kChromeColorsEnd ensures
//     Astra colors don't overlap with Chrome's color ID space.
//     Chromium owner: ChromeColorIds
//       (chrome/browser/ui/color/chrome_color_id.h)
//     Patch point: Update kAstraColorsStart to kChromeColorsEnd once
//     we build against the full Chrome layer.

#ifndef ASTRA_UI_COLOR_ASTRA_COLOR_IDS_H_
#define ASTRA_UI_COLOR_ASTRA_COLOR_IDS_H_

#include "ui/color/color_id.h"

namespace astra {

// ---------------------------------------------------------------------------
// Color ID range
// ---------------------------------------------------------------------------

// Start of Astra-specific color IDs. All Astra colors are numbered after
// Chromium's built-in UI colors (kUiColorsEnd) to avoid collisions.
//
// TODO(astra): See note above about kChromeColorsEnd vs kUiColorsEnd.
constexpr ui::ColorId kAstraColorsStart = ui::kUiColorsEnd;

// ---------------------------------------------------------------------------
// Sidebar colors
// ---------------------------------------------------------------------------
//
// The sidebar is Astra's primary navigation surface (vertical tab strip,
// workspace switcher, favorites, history, etc.). These colors define the
// sidebar's visual appearance.

// Sidebar panel background color. Slightly different from the main window
// background to give the sidebar its own visual identity.
constexpr ui::ColorId kColorAstraSidebarBackground = kAstraColorsStart + 0;

// Border color between the sidebar and the main content area.
constexpr ui::ColorId kColorAstraSidebarBorder = kAstraColorsStart + 1;

// Background of a sidebar item on hover.
constexpr ui::ColorId kColorAstraSidebarItemHoverBackground =
    kAstraColorsStart + 2;

// Background of a sidebar item when selected / active.
// Used for the current tab or active workspace.
constexpr ui::ColorId kColorAstraSidebarItemSelectedBackground =
    kAstraColorsStart + 3;

// Text color of a sidebar item in its default state.
constexpr ui::ColorId kColorAstraSidebarItemText = kAstraColorsStart + 4;

// Text color of a sidebar item when selected / active.
constexpr ui::ColorId kColorAstraSidebarItemSelectedText =
    kAstraColorsStart + 5;

// Text color for sidebar section headers (e.g., "Favorites", "Open Tabs").
constexpr ui::ColorId kColorAstraSidebarSectionHeaderText =
    kAstraColorsStart + 6;

// Text color for secondary / descriptive text in sidebar items.
// Used for URLs, timestamps, counts, etc.
constexpr ui::ColorId kColorAstraSidebarItemSecondaryText =
    kAstraColorsStart + 7;

// ---------------------------------------------------------------------------
// Workspace / accent colors
// ---------------------------------------------------------------------------
//
// Workspaces have an accent color that is used as a visual identity.
// These colors are derived from the workspace's accent_color setting.
// The accent color is injected by AstraColorMixer and propagated through
// the ColorProvider system.
//
// TODO(astra): Connect accent color to AstraWorkspaceService and
//   AstraThemeService. When the active workspace changes, trigger a
//   ColorProvider rebuild so accent-derived colors update.
//   Chromium owner: ColorProviderKey / ThemeService
//   (chrome/browser/themes/theme_service.h)
//
//   TODO(astra): Consider whether accent colors should be dynamic (read
//     from service at render time) rather than static ColorProvider colors.
//     ADR-0022 mentions that accent colors are "dynamic per-workspace colors"
//     that "are not part of the static color provider." If the active
//     workspace changes frequently, a ColorProvider rebuild on every switch
//     might be too expensive.
//     Astra owner: AstraThemeService / AstraWorkspaceService

// Primary workspace accent color. Used as a vibrant brand color for the
// active workspace (e.g., workspace indicator dot, switcher highlight).
constexpr ui::ColorId kColorAstraWorkspaceAccent = kAstraColorsStart + 20;

// Hover state of the accent color (slightly lighter / more saturated).
// Used for hover effects on accent-colored elements.
constexpr ui::ColorId kColorAstraWorkspaceAccentHover = kAstraColorsStart + 21;

// Active / pressed state of the accent color (slightly darker).
// Used for pressed states on accent-colored buttons.
constexpr ui::ColorId kColorAstraWorkspaceAccentActive = kAstraColorsStart + 22;

// Subtle accent background. A low-alpha version of the accent color, used
// as a background tint for the active workspace or highlighted sections.
constexpr ui::ColorId kColorAstraWorkspaceAccentSubtle = kAstraColorsStart + 23;

// Text color for elements drawn on top of an accent-colored background.
// Usually white or near-white, chosen for contrast with the accent color.
constexpr ui::ColorId kColorAstraWorkspaceAccentText = kAstraColorsStart + 24;

// Accent color at reduced opacity, used as a border or divider.
constexpr ui::ColorId kColorAstraWorkspaceAccentBorder = kAstraColorsStart + 25;

// ---------------------------------------------------------------------------
// Split View colors
// ---------------------------------------------------------------------------
//
// Split view shows two WebContents side by side with a draggable divider.

// Color of the split view divider line between two panes.
constexpr ui::ColorId kColorAstraSplitViewDivider = kAstraColorsStart + 40;

// Color of the drag indicator shown while resizing the split view divider.
constexpr ui::ColorId kColorAstraSplitViewDragIndicator =
    kAstraColorsStart + 41;

// Background tint behind the split view area.
constexpr ui::ColorId kColorAstraSplitViewBackground = kAstraColorsStart + 42;

// ---------------------------------------------------------------------------
// Command Palette colors
// ---------------------------------------------------------------------------
//
// The command palette is a quick-access command search UI (Cmd/Ctrl+K).

// Background color of the command palette popup.
constexpr ui::ColorId kColorAstraCommandPaletteBackground =
    kAstraColorsStart + 60;

// Background of the selected / highlighted command item.
constexpr ui::ColorId kColorAstraCommandPaletteSelectedBackground =
    kAstraColorsStart + 61;

// Text color for the keyboard shortcut hint in command palette items.
constexpr ui::ColorId kColorAstraCommandPaletteShortcutText =
    kAstraColorsStart + 62;

// Border color of the command palette popup.
constexpr ui::ColorId kColorAstraCommandPaletteBorder = kAstraColorsStart + 63;

// Text color for command names in the palette.
constexpr ui::ColorId kColorAstraCommandPaletteText = kAstraColorsStart + 64;

// Text color for command descriptions (secondary text).
constexpr ui::ColorId kColorAstraCommandPaletteDescriptionText =
    kAstraColorsStart + 65;

// Color of the search field text in the command palette.
constexpr ui::ColorId kColorAstraCommandPaletteSearchText =
    kAstraColorsStart + 66;

// ---------------------------------------------------------------------------
// Focus Mode colors
// ---------------------------------------------------------------------------
//
// Focus mode hides distractions and shows a subtle focus indicator.

// Background color of the focus mode indicator badge.
// Semi-transparent, dark appearance so it doesn't distract.
constexpr ui::ColorId kColorAstraFocusModeIndicatorBackground =
    kAstraColorsStart + 80;

// Text color of the focus mode indicator.
constexpr ui::ColorId kColorAstraFocusModeIndicatorText =
    kAstraColorsStart + 81;

// Color of the focus mode dimming overlay (applied to non-content areas).
constexpr ui::ColorId kColorAstraFocusModeDimOverlay = kAstraColorsStart + 82;

// ---------------------------------------------------------------------------
// Glance / Peek colors
// ---------------------------------------------------------------------------
//
// Glance is a quick preview popup that appears when hovering over a
// sidebar item. Peek is a larger in-place preview.

// Border color of the Glance preview popup.
constexpr ui::ColorId kColorAstraGlanceBorder = kAstraColorsStart + 100;

// Background color of the Glance preview popup.
constexpr ui::ColorId kColorAstraGlanceBackground = kAstraColorsStart + 101;

// Highlight color for the peek / preview edge indicator.
constexpr ui::ColorId kColorAstraPeekHighlight = kAstraColorsStart + 102;

// Background of the peek preview area.
constexpr ui::ColorId kColorAstraPeekBackground = kAstraColorsStart + 103;

// ---------------------------------------------------------------------------
// End marker
// ---------------------------------------------------------------------------

// One past the last Astra color ID. Must be updated when adding new colors.
constexpr ui::ColorId kAstraColorsEnd = kAstraColorsStart + 128;

// Ensure the range is valid: start must be >= kUiColorsEnd, and end must
// be greater than start.
static_assert(kAstraColorsStart >= ui::kUiColorsEnd,
              "Astra colors must start at or after UI colors end");
static_assert(kAstraColorsEnd > kAstraColorsStart,
              "Astra color range must be non-empty");

}  // namespace astra

#endif  // ASTRA_UI_COLOR_ASTRA_COLOR_IDS_H_
