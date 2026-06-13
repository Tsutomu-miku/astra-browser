// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/common/astra_color_utils.h"

#include <algorithm>
#include <cmath>

#include "base/check.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/native_theme/native_theme.h"

namespace astra {

// ---------------------------------------------------------------------------
// Color helper functions
// ---------------------------------------------------------------------------

SkColor LightenColor(SkColor color, uint8_t amount) {
  uint8_t r = std::min(255, static_cast<int>(SkColorGetR(color)) + amount);
  uint8_t g = std::min(255, static_cast<int>(SkColorGetG(color)) + amount);
  uint8_t b = std::min(255, static_cast<int>(SkColorGetB(color)) + amount);
  return SkColorSetRGB(r, g, b);
}

SkColor DarkenColor(SkColor color, uint8_t amount) {
  uint8_t r = std::max(0, static_cast<int>(SkColorGetR(color)) - amount);
  uint8_t g = std::max(0, static_cast<int>(SkColorGetG(color)) - amount);
  uint8_t b = std::max(0, static_cast<int>(SkColorGetB(color)) - amount);
  return SkColorSetRGB(r, g, b);
}

float RelativeLuminance(SkColor color) {
  float r = SkColorGetR(color) / 255.0f;
  float g = SkColorGetG(color) / 255.0f;
  float b = SkColorGetB(color) / 255.0f;
  // sRGB to linear approximation (WCAG formula).
  auto to_linear = [](float c) {
    return c <= 0.03928f ? c / 12.92f : powf((c + 0.055f) / 1.055f, 2.4f);
  };
  return 0.2126f * to_linear(r) + 0.7152f * to_linear(g) +
         0.0722f * to_linear(b);
}

SkColor GetContrastTextColor(SkColor background) {
  return RelativeLuminance(background) > 0.5f ? SK_ColorBLACK : SK_ColorWHITE;
}

SkColor BlendColors(SkColor a, SkColor b, float alpha) {
  DCHECK_GE(alpha, 0.0f);
  DCHECK_LE(alpha, 1.0f);

  float inv = 1.0f - alpha;

  uint8_t r = static_cast<uint8_t>(
      std::round(inv * SkColorGetR(a) + alpha * SkColorGetR(b)));
  uint8_t g = static_cast<uint8_t>(
      std::round(inv * SkColorGetG(a) + alpha * SkColorGetG(b)));
  uint8_t b_comp = static_cast<uint8_t>(
      std::round(inv * SkColorGetB(a) + alpha * SkColorGetB(b)));

  // Preserve alpha from the base color (a).
  return SkColorSetARGB(SkColorGetA(a), r, g, b_comp);
}

// ---------------------------------------------------------------------------
// Accent palette computation
// ---------------------------------------------------------------------------

namespace {

// Returns a subtle (low-alpha) version of the accent color, suitable for
// use as a background tint or selection highlight.
// The subtle alpha is tuned so the tint reads as "faint accent flavor"
// rather than a solid color, against both light and dark backgrounds.
SkColor MakeSubtleAccent(SkColor accent_color, bool dark_mode) {
  const uint8_t alpha = dark_mode ? 0x40 : 0x26;
  return SkColorSetA(accent_color, alpha);
}

// Returns the accent color at border-like opacity.
SkColor MakeAccentBorder(SkColor accent_color, bool dark_mode) {
  const uint8_t alpha = dark_mode ? 0x80 : 0x60;
  return SkColorSetA(accent_color, alpha);
}

}  // namespace

AstraAccentPalette GetAstraAccentPalette(SkColor base_color, bool dark_mode) {
  AstraAccentPalette palette;
  palette.base = base_color;

  if (dark_mode) {
    // In dark mode, hover is brighter (more white mixed in),
    // active is slightly darker.
    palette.hover = LightenColor(base_color, 20);
    palette.active = DarkenColor(base_color, 15);
  } else {
    // In light mode, hover is slightly darker (more saturated feel),
    // active is noticeably darker.
    palette.hover = DarkenColor(base_color, 10);
    palette.active = DarkenColor(base_color, 25);
  }

  palette.subtle = MakeSubtleAccent(base_color, dark_mode);
  palette.text = GetContrastTextColor(base_color);
  palette.border = MakeAccentBorder(base_color, dark_mode);

  return palette;
}

// ---------------------------------------------------------------------------
// Dark mode detection
// ---------------------------------------------------------------------------

bool IsDarkModeActive() {
  // Query the native UI theme for the current color scheme preference.
  // This is the same source Chromium uses for its ColorProvider dark/light
  // decision, so Astra colors will match the rest of the browser.
  //
  // Chromium component: NativeTheme (ui/native_theme/native_theme.h)
  //
  // TODO(astra): Also check ThemeService for user theme overrides.
  //   Some Chromium themes force a specific mode regardless of system
  //   settings.  The full logic lives in chrome/browser/themes/theme_service.cc
  //   and chrome/browser/ui/color/chrome_color_provider_utils.cc.
  //   Chromium owner: ThemeService / ChromeColorProviderUtils.
  auto* native_theme = ui::NativeTheme::GetInstanceForNativeUi();
  return native_theme && native_theme->ShouldUseDarkColors();
}

}  // namespace astra
