// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Astra color utility functions — reusable color and theme helpers.
//
// These utilities are shared between the color mixer (ui/color), the
// theme service (browser/), and other Astra layers.  They provide
// palette computation, contrast detection, and color blending operations.
//
// Chromium subsystems reused:
//   - SkColor / SkBlendMode for color operations (third_party/skia)
//   - NativeTheme for system dark/light mode detection (ui/native_theme)
//
// TODO(astra): Add more theme utilities as needed (e.g. elevation shadows,
//   shape radii, type scale helpers) as the design system matures.
//   Chromium owner: NativeTheme / ColorProvider teams.

#ifndef ASTRA_COMMON_ASTRA_COLOR_UTILS_H_
#define ASTRA_COMMON_ASTRA_COLOR_UTILS_H_

#include "third_party/skia/include/core/SkColor.h"

namespace astra {

// ---------------------------------------------------------------------------
// Accent palette
// ---------------------------------------------------------------------------

// Six-tone accent palette derived from a single base accent color.
//
// The palette provides all the color variants needed for interactive UI
// elements that use the accent color (buttons, tabs, selection highlights,
// borders, text on accent backgrounds, etc.).
//
// Variants:
//   - base:    the base accent color itself
//   - hover:   accent in hover state (slightly lighter or darker)
//   - active:  accent in pressed/active state
//   - subtle:  low-alpha accent tint for backgrounds and selection highlights
//   - text:    white or black, chosen for WCAG AA contrast on the base accent
//   - border:  medium-alpha accent for borders and dividers
//
// The palette is computed differently in light vs dark mode so the accent
// reads correctly against the surrounding background tone.
struct AstraAccentPalette {
  SkColor base;
  SkColor hover;
  SkColor active;
  SkColor subtle;
  SkColor text;
  SkColor border;
};

// Computes a full accent palette from a base accent color.
//
// Args:
//   base_color - The base accent color (e.g. workspace accent color).
//   dark_mode  - Whether the palette should be tuned for dark mode.
//                Dark mode palettes use brighter hover states and
//                higher-alpha subtle tints to remain visible.
//
// Returns:
//   A 6-tone AstraAccentPalette with base, hover, active, subtle,
//   text, and border variants.
//
// This is the same logic used internally by the Astra color mixer,
// exposed as a standalone utility for use by the theme service and
// other components that need accent color computation outside the
// ColorProvider system (e.g. workspace switcher previews, command
// palette accent badges).
//
// TODO(astra): Consider using the Material Color Utilities library
//   (third_party/material_color_utilities) for proper tonal palette
//   generation instead of the simple lighten/darken approach here.
//   Chromium owner: ui/color/material_color_mixer.cc
AstraAccentPalette GetAstraAccentPalette(SkColor base_color, bool dark_mode);

// ---------------------------------------------------------------------------
// Dark mode detection
// ---------------------------------------------------------------------------

// Returns true if the system is in dark mode.
//
// Queries NativeTheme for the current system color scheme preference.
// This is the same source Chromium's ColorProvider uses to decide
// between light and dark palettes.
//
// Chromium component: NativeTheme (ui/native_theme/native_theme.h)
//   Method: NativeTheme::GetInstanceForNativeUi()->ShouldUseDarkColors()
//
// TODO(astra): Also check ThemeService for user overrides (e.g. the
//   "Use GTK+" or "Classic" theme settings that can force light or dark).
//   Chromium owner: ThemeService (chrome/browser/themes/theme_service.h)
bool IsDarkModeActive();

// ---------------------------------------------------------------------------
// Color utility functions
// ---------------------------------------------------------------------------

// Returns white or black text color based on the background color's
// luminance, ensuring WCAG AA contrast for text on colored backgrounds.
//
// Uses the WCAG relative luminance formula to decide.
SkColor GetContrastTextColor(SkColor background);

// Blends two colors together.
//
// Args:
//   a     - The base (background) color.
//   b     - The foreground color to blend on top.
//   alpha - Blend amount (0.0 = fully a, 1.0 = fully b).
//
// Returns:
//   The blended color, with the same alpha channel as |a|.
//
// This is a simple alpha compositing blend (source-over) in RGB space.
// For perceptually accurate blending, use SkColorSpace or a color
// management library.
SkColor BlendColors(SkColor a, SkColor b, float alpha);

// Lightens a color by the given amount (0-255 added to each channel).
// Clamps to valid channel values.
SkColor LightenColor(SkColor color, uint8_t amount);

// Darkens a color by the given amount (0-255 subtracted from each channel).
// Clamps to valid channel values.
SkColor DarkenColor(SkColor color, uint8_t amount);

// Computes the relative luminance of a color (per WCAG).
// Used for contrast calculations and text color selection.
float RelativeLuminance(SkColor color);

}  // namespace astra

#endif  // ASTRA_COMMON_ASTRA_COLOR_UTILS_H_
