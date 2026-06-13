// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Astra Color Mixer implementation.
//
// Registers all Astra-specific ColorIds with their computed values on a
// ui::ColorProvider. Supports light/dark mode and accent color derivation.
//
// Color design principles:
//   - Sidebar colors are slightly offset from the main window background
//     to give the sidebar visual identity without being distracting.
//   - Workspace accent colors are derived from a single base accent color.
//   - All text colors meet WCAG AA contrast ratios against their backgrounds.
//   - Dark mode colors follow Chromium's Material dark theme palette.
//
// TODO(astra): Tune color values against the final visual design spec.
//   Current values are placeholders based on Chromium's Material palette
//   and Google Blue as a reference accent. The design team should provide
//   final color values for light and dark themes.
//
// TODO(astra): Add per-palette overrides (e.g., "high contrast" mode).
//   Chromium pattern: ColorProviderKey includes a contrast mode field that
//   mixers can read to adjust colors for accessibility.
//   Chromium owner: ColorProviderKey (ui/color/color_provider_key.h)

#include "astra/ui/color/astra_color_mixer.h"

#include "astra/common/astra_color_utils.h"
#include "astra/ui/color/astra_color_ids.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/color/color_provider.h"

namespace astra {

namespace {

// ---------------------------------------------------------------------------
// Light mode color defaults
// ---------------------------------------------------------------------------
//
// Light mode palette: warm white background, dark text, subtle borders.
// Based on Chromium's Material light theme with Astra-specific tweaks.

// -- Sidebar (light) --
constexpr SkColor kSidebarBackgroundLight = SkColorSetRGB(0xF8, 0xF9, 0xFA);
constexpr SkColor kSidebarBorderLight = SkColorSetRGB(0xE1, 0xE3, 0xE6);
constexpr SkColor kSidebarItemHoverBgLight = SkColorSetRGB(0xE8, 0xEA, 0xED);
constexpr SkColor kSidebarItemSelectedBgLight = SkColorSetRGB(0xD2, 0xE3, 0xFC);
constexpr SkColor kSidebarItemTextLight = SkColorSetRGB(0x20, 0x21, 0x24);
constexpr SkColor kSidebarItemSelectedTextLight = SkColorSetRGB(0x19, 0x67, 0xD2);
constexpr SkColor kSidebarSectionHeaderTextLight = SkColorSetRGB(0x5F, 0x63, 0x68);
constexpr SkColor kSidebarItemSecondaryTextLight = SkColorSetRGB(0x5F, 0x63, 0x68);

// -- Split view (light) --
constexpr SkColor kSplitViewDividerLight = SkColorSetRGB(0xDA, 0xDC, 0xE0);
constexpr SkColor kSplitViewBackgroundLight = SkColorSetRGB(0xF1, 0xF3, 0xF4);

// -- Command palette (light) --
constexpr SkColor kCommandPaletteBackgroundLight = SK_ColorWHITE;
constexpr SkColor kCommandPaletteSelectedBgLight = SkColorSetRGB(0xE8, 0xF0, 0xFE);
constexpr SkColor kCommandPaletteShortcutTextLight = SkColorSetRGB(0x5F, 0x63, 0x68);
constexpr SkColor kCommandPaletteBorderLight = SkColorSetRGB(0xDA, 0xDC, 0xE0);
constexpr SkColor kCommandPaletteTextLight = SkColorSetRGB(0x20, 0x21, 0x24);
constexpr SkColor kCommandPaletteDescriptionTextLight =
    SkColorSetRGB(0x5F, 0x63, 0x68);
constexpr SkColor kCommandPaletteSearchTextLight = SkColorSetRGB(0x20, 0x21, 0x24);

// -- Focus mode (light) --
constexpr SkColor kFocusModeIndicatorBackgroundLight =
    SkColorSetARGB(0xE6, 0x20, 0x21, 0x24);
constexpr SkColor kFocusModeIndicatorTextLight = SK_ColorWHITE;
constexpr SkColor kFocusModeDimOverlayLight = SkColorSetARGB(0x10, 0, 0, 0);

// -- Glance / Peek (light) --
constexpr SkColor kGlanceBorderLight = SkColorSetRGB(0xDA, 0xDC, 0xE0);
constexpr SkColor kGlanceBackgroundLight = SK_ColorWHITE;
constexpr SkColor kPeekBackgroundLight = SkColorSetRGB(0xF8, 0xF9, 0xFA);

// ---------------------------------------------------------------------------
// Dark mode color defaults
// ---------------------------------------------------------------------------
//
// Dark mode palette: near-black background, light text, softer accents.
// Based on Chromium's Material dark theme with Astra-specific tweaks.

// -- Sidebar (dark) --
constexpr SkColor kSidebarBackgroundDark = SkColorSetRGB(0x20, 0x21, 0x24);
constexpr SkColor kSidebarBorderDark = SkColorSetRGB(0x3C, 0x40, 0x43);
constexpr SkColor kSidebarItemHoverBgDark = SkColorSetRGB(0x3C, 0x40, 0x43);
constexpr SkColor kSidebarItemSelectedBgDark = SkColorSetRGB(0x2B, 0x3A, 0x55);
constexpr SkColor kSidebarItemTextDark = SkColorSetRGB(0xE8, 0xEA, 0xED);
constexpr SkColor kSidebarItemSelectedTextDark = SkColorSetRGB(0x8A, 0xB4, 0xF8);
constexpr SkColor kSidebarSectionHeaderTextDark = SkColorSetRGB(0x9A, 0xA0, 0xA6);
constexpr SkColor kSidebarItemSecondaryTextDark = SkColorSetRGB(0x9A, 0xA0, 0xA6);

// -- Split view (dark) --
constexpr SkColor kSplitViewDividerDark = SkColorSetRGB(0x3C, 0x40, 0x43);
constexpr SkColor kSplitViewBackgroundDark = SkColorSetRGB(0x29, 0x2A, 0x2D);

// -- Command palette (dark) --
constexpr SkColor kCommandPaletteBackgroundDark = SkColorSetRGB(0x29, 0x2A, 0x2D);
constexpr SkColor kCommandPaletteSelectedBgDark = SkColorSetRGB(0x3B, 0x4B, 0x6D);
constexpr SkColor kCommandPaletteShortcutTextDark = SkColorSetRGB(0x9A, 0xA0, 0xA6);
constexpr SkColor kCommandPaletteBorderDark = SkColorSetRGB(0x3C, 0x40, 0x43);
constexpr SkColor kCommandPaletteTextDark = SkColorSetRGB(0xE8, 0xEA, 0xED);
constexpr SkColor kCommandPaletteDescriptionTextDark =
    SkColorSetRGB(0x9A, 0xA0, 0xA6);
constexpr SkColor kCommandPaletteSearchTextDark = SkColorSetRGB(0xE8, 0xEA, 0xED);

// -- Focus mode (dark) --
constexpr SkColor kFocusModeIndicatorBackgroundDark =
    SkColorSetARGB(0xE6, 0x20, 0x21, 0x24);
constexpr SkColor kFocusModeIndicatorTextDark = SK_ColorWHITE;
constexpr SkColor kFocusModeDimOverlayDark = SkColorSetARGB(0x30, 0, 0, 0);

// -- Glance / Peek (dark) --
constexpr SkColor kGlanceBorderDark = SkColorSetRGB(0x3C, 0x40, 0x43);
constexpr SkColor kGlanceBackgroundDark = SkColorSetRGB(0x29, 0x2A, 0x2D);
constexpr SkColor kPeekBackgroundDark = SkColorSetRGB(0x20, 0x21, 0x24);

// ---------------------------------------------------------------------------
// Accent color derivation
// ---------------------------------------------------------------------------
//
// Uses AstraAccentPalette from astra/common/astra_color_utils.h.
// See GetAstraAccentPalette() for the full palette computation logic.

// ---------------------------------------------------------------------------
// Color registration helpers
// ---------------------------------------------------------------------------

// Register all sidebar colors for the current theme mode.
void AddSidebarColors(ui::ColorProvider* provider, bool dark_mode) {
  if (dark_mode) {
    provider->SetColor(kColorAstraSidebarBackground, kSidebarBackgroundDark);
    provider->SetColor(kColorAstraSidebarBorder, kSidebarBorderDark);
    provider->SetColor(kColorAstraSidebarItemHoverBackground,
                       kSidebarItemHoverBgDark);
    provider->SetColor(kColorAstraSidebarItemSelectedBackground,
                       kSidebarItemSelectedBgDark);
    provider->SetColor(kColorAstraSidebarItemText, kSidebarItemTextDark);
    provider->SetColor(kColorAstraSidebarItemSelectedText,
                       kSidebarItemSelectedTextDark);
    provider->SetColor(kColorAstraSidebarSectionHeaderText,
                       kSidebarSectionHeaderTextDark);
    provider->SetColor(kColorAstraSidebarItemSecondaryText,
                       kSidebarItemSecondaryTextDark);
  } else {
    provider->SetColor(kColorAstraSidebarBackground, kSidebarBackgroundLight);
    provider->SetColor(kColorAstraSidebarBorder, kSidebarBorderLight);
    provider->SetColor(kColorAstraSidebarItemHoverBackground,
                       kSidebarItemHoverBgLight);
    provider->SetColor(kColorAstraSidebarItemSelectedBackground,
                       kSidebarItemSelectedBgLight);
    provider->SetColor(kColorAstraSidebarItemText, kSidebarItemTextLight);
    provider->SetColor(kColorAstraSidebarItemSelectedText,
                       kSidebarItemSelectedTextLight);
    provider->SetColor(kColorAstraSidebarSectionHeaderText,
                       kSidebarSectionHeaderTextLight);
    provider->SetColor(kColorAstraSidebarItemSecondaryText,
                       kSidebarItemSecondaryTextLight);
  }
}

// Register all workspace / accent colors.
void AddWorkspaceAccentColors(ui::ColorProvider* provider,
                              bool dark_mode,
                              SkColor accent_color) {
  AstraAccentPalette palette = GetAstraAccentPalette(accent_color, dark_mode);

  provider->SetColor(kColorAstraWorkspaceAccent, palette.base);
  provider->SetColor(kColorAstraWorkspaceAccentHover, palette.hover);
  provider->SetColor(kColorAstraWorkspaceAccentActive, palette.active);
  provider->SetColor(kColorAstraWorkspaceAccentSubtle, palette.subtle);
  provider->SetColor(kColorAstraWorkspaceAccentText, palette.text);
  provider->SetColor(kColorAstraWorkspaceAccentBorder, palette.border);
}

// Register all split view colors.
void AddSplitViewColors(ui::ColorProvider* provider,
                        bool dark_mode,
                        SkColor accent_color) {
  if (dark_mode) {
    provider->SetColor(kColorAstraSplitViewDivider, kSplitViewDividerDark);
    provider->SetColor(kColorAstraSplitViewBackground, kSplitViewBackgroundDark);
  } else {
    provider->SetColor(kColorAstraSplitViewDivider, kSplitViewDividerLight);
    provider->SetColor(kColorAstraSplitViewBackground,
                       kSplitViewBackgroundLight);
  }

  // Drag indicator uses the accent color for visibility during resize.
  provider->SetColor(kColorAstraSplitViewDragIndicator, accent_color);
}

// Register all command palette colors.
void AddCommandPaletteColors(ui::ColorProvider* provider, bool dark_mode) {
  if (dark_mode) {
    provider->SetColor(kColorAstraCommandPaletteBackground,
                       kCommandPaletteBackgroundDark);
    provider->SetColor(kColorAstraCommandPaletteSelectedBackground,
                       kCommandPaletteSelectedBgDark);
    provider->SetColor(kColorAstraCommandPaletteShortcutText,
                       kCommandPaletteShortcutTextDark);
    provider->SetColor(kColorAstraCommandPaletteBorder, kCommandPaletteBorderDark);
    provider->SetColor(kColorAstraCommandPaletteText, kCommandPaletteTextDark);
    provider->SetColor(kColorAstraCommandPaletteDescriptionText,
                       kCommandPaletteDescriptionTextDark);
    provider->SetColor(kColorAstraCommandPaletteSearchText,
                       kCommandPaletteSearchTextDark);
  } else {
    provider->SetColor(kColorAstraCommandPaletteBackground,
                       kCommandPaletteBackgroundLight);
    provider->SetColor(kColorAstraCommandPaletteSelectedBackground,
                       kCommandPaletteSelectedBgLight);
    provider->SetColor(kColorAstraCommandPaletteShortcutText,
                       kCommandPaletteShortcutTextLight);
    provider->SetColor(kColorAstraCommandPaletteBorder,
                       kCommandPaletteBorderLight);
    provider->SetColor(kColorAstraCommandPaletteText, kCommandPaletteTextLight);
    provider->SetColor(kColorAstraCommandPaletteDescriptionText,
                       kCommandPaletteDescriptionTextLight);
    provider->SetColor(kColorAstraCommandPaletteSearchText,
                       kCommandPaletteSearchTextLight);
  }
}

// Register all focus mode colors.
void AddFocusModeColors(ui::ColorProvider* provider, bool dark_mode) {
  if (dark_mode) {
    provider->SetColor(kColorAstraFocusModeIndicatorBackground,
                       kFocusModeIndicatorBackgroundDark);
    provider->SetColor(kColorAstraFocusModeIndicatorText,
                       kFocusModeIndicatorTextDark);
    provider->SetColor(kColorAstraFocusModeDimOverlay, kFocusModeDimOverlayDark);
  } else {
    provider->SetColor(kColorAstraFocusModeIndicatorBackground,
                       kFocusModeIndicatorBackgroundLight);
    provider->SetColor(kColorAstraFocusModeIndicatorText,
                       kFocusModeIndicatorTextLight);
    provider->SetColor(kColorAstraFocusModeDimOverlay, kFocusModeDimOverlayLight);
  }
}

// Register all Glance / Peek colors.
void AddGlancePeekColors(ui::ColorProvider* provider,
                         bool dark_mode,
                         SkColor accent_color) {
  if (dark_mode) {
    provider->SetColor(kColorAstraGlanceBorder, kGlanceBorderDark);
    provider->SetColor(kColorAstraGlanceBackground, kGlanceBackgroundDark);
    provider->SetColor(kColorAstraPeekBackground, kPeekBackgroundDark);
  } else {
    provider->SetColor(kColorAstraGlanceBorder, kGlanceBorderLight);
    provider->SetColor(kColorAstraGlanceBackground, kGlanceBackgroundLight);
    provider->SetColor(kColorAstraPeekBackground, kPeekBackgroundLight);
  }

  // Peek highlight uses the accent color as a visual marker.
  provider->SetColor(kColorAstraPeekHighlight, accent_color);
}

}  // namespace

// ===========================================================================
// Public API
// ===========================================================================

void AddAstraColorMixer(ui::ColorProvider* provider,
                        bool dark_mode,
                        SkColor accent_color) {
  DCHECK(provider);

  // Register all Astra color categories.
  // Order doesn't matter since each SetColor call is independent.
  //
  // TODO(astra): Use ColorProvider's AddMixer() pattern with a mixer
  //   function object instead of direct SetColor calls, to match Chromium's
  //   layering approach where mixers can override each other by priority.
  //   Chromium pattern: ui/color/color_mixer.h
  //   For now, direct SetColor works since Astra colors are additive and
  //   don't need to be overridden by other mixers.
  AddSidebarColors(provider, dark_mode);
  AddWorkspaceAccentColors(provider, dark_mode, accent_color);
  AddSplitViewColors(provider, dark_mode, accent_color);
  AddCommandPaletteColors(provider, dark_mode);
  AddFocusModeColors(provider, dark_mode);
  AddGlancePeekColors(provider, dark_mode, accent_color);
}

}  // namespace astra
