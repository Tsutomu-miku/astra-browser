// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Unit tests for Astra color utilities (color helpers, dark mode detection,
// accent palette computation).

#include "astra/common/astra_color_utils.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkColor.h"

namespace astra {

namespace {

// Test colors with known luminance values.
constexpr SkColor kWhite = SK_ColorWHITE;
constexpr SkColor kBlack = SK_ColorBLACK;
constexpr SkColor kRed = SkColorSetRGB(0xFF, 0x00, 0x00);
constexpr SkColor kGreen = SkColorSetRGB(0x00, 0xFF, 0x00);
constexpr SkColor kBlue = SkColorSetRGB(0x00, 0x00, 0xFF);
constexpr SkColor kGray = SkColorSetRGB(0x80, 0x80, 0x80);
constexpr SkColor kGoogleBlue = SkColorSetRGB(0x1A, 0x73, 0xE8);

}  // namespace

// =========================================================================
// RelativeLuminance tests
// =========================================================================

TEST(AstraColorUtilsTest, RelativeLuminance_WhiteIsHigh) {
  // White should have luminance close to 1.0.
  EXPECT_GT(RelativeLuminance(kWhite), 0.9f);
}

TEST(AstraColorUtilsTest, RelativeLuminance_BlackIsLow) {
  // Black should have luminance close to 0.0.
  EXPECT_LT(RelativeLuminance(kBlack), 0.01f);
}

TEST(AstraColorUtilsTest, RelativeLuminance_GreenIsBrighterThanBlue) {
  // Green contributes more to perceived luminance than blue.
  EXPECT_GT(RelativeLuminance(kGreen), RelativeLuminance(kBlue));
}

TEST(AstraColorUtilsTest, RelativeLuminance_RedIsBrighterThanBlue) {
  // Red contributes more to perceived luminance than blue.
  EXPECT_GT(RelativeLuminance(kRed), RelativeLuminance(kBlue));
}

TEST(AstraColorUtilsTest, RelativeLuminance_GrayIsMidRange) {
  // Medium gray should be around 0.217 (sRGB midpoint ~21.4%).
  float lum = RelativeLuminance(kGray);
  EXPECT_GT(lum, 0.15f);
  EXPECT_LT(lum, 0.3f);
}

TEST(AstraColorUtilsTest, RelativeLuminance_IsInRange) {
  // Luminance should always be between 0 and 1.
  EXPECT_GE(RelativeLuminance(kWhite), 0.0f);
  EXPECT_LE(RelativeLuminance(kWhite), 1.0f);
  EXPECT_GE(RelativeLuminance(kBlack), 0.0f);
  EXPECT_LE(RelativeLuminance(kBlack), 1.0f);
  EXPECT_GE(RelativeLuminance(kGoogleBlue), 0.0f);
  EXPECT_LE(RelativeLuminance(kGoogleBlue), 1.0f);
}

// =========================================================================
// GetContrastTextColor tests
// =========================================================================

TEST(AstraColorUtilsTest, ContrastTextColor_WhiteOnBlack) {
  // White background → black text.
  EXPECT_EQ(SK_ColorBLACK, GetContrastTextColor(kWhite));
}

TEST(AstraColorUtilsTest, ContrastTextColor_BlackOnWhite) {
  // Black background → white text.
  EXPECT_EQ(SK_ColorWHITE, GetContrastTextColor(kBlack));
}

TEST(AstraColorUtilsTest, ContrastTextColor_GoogleBlue) {
  // Google Blue (#1A73E8) should have white text (dark enough).
  EXPECT_EQ(SK_ColorWHITE, GetContrastTextColor(kGoogleBlue));
}

TEST(AstraColorUtilsTest, ContrastTextColor_LightGray) {
  // Light gray background → dark text.
  SkColor light_gray = SkColorSetRGB(0xDD, 0xDD, 0xDD);
  EXPECT_EQ(SK_ColorBLACK, GetContrastTextColor(light_gray));
}

TEST(AstraColorUtilsTest, ContrastTextColor_DarkGray) {
  // Dark gray background → light text.
  SkColor dark_gray = SkColorSetRGB(0x33, 0x33, 0x33);
  EXPECT_EQ(SK_ColorWHITE, GetContrastTextColor(dark_gray));
}

// =========================================================================
// LightenColor tests
// =========================================================================

TEST(AstraColorUtilsTest, LightenColor_BlackBecomesGray) {
  SkColor result = LightenColor(kBlack, 128);
  EXPECT_EQ(SkColorGetR(result), 128);
  EXPECT_EQ(SkColorGetG(result), 128);
  EXPECT_EQ(SkColorGetB(result), 128);
  EXPECT_EQ(SkColorGetA(result), 255U);
}

TEST(AstraColorUtilsTest, LightenColor_WhiteStaysWhite) {
  // Lightening white should not overflow.
  SkColor result = LightenColor(kWhite, 50);
  EXPECT_EQ(SkColorGetR(result), 255);
  EXPECT_EQ(SkColorGetG(result), 255);
  EXPECT_EQ(SkColorGetB(result), 255);
}

TEST(AstraColorUtilsTest, LightenColor_ZeroIsNoOp) {
  // Lightening by zero should return the same color.
  SkColor result = LightenColor(kGoogleBlue, 0);
  EXPECT_EQ(result, kGoogleBlue);
}

TEST(AstraColorUtilsTest, LightenColor_RedChannel) {
  SkColor red = SkColorSetRGB(100, 50, 25);
  SkColor result = LightenColor(red, 30);
  EXPECT_EQ(SkColorGetR(result), 130);
  EXPECT_EQ(SkColorGetG(result), 80);
  EXPECT_EQ(SkColorGetB(result), 55);
}

TEST(AstraColorUtilsTest, LightenColor_PartialClamp) {
  // Some channels clamp, others don't.
  SkColor color = SkColorSetRGB(250, 100, 200);
  SkColor result = LightenColor(color, 20);
  EXPECT_EQ(SkColorGetR(result), 255);  // clamped
  EXPECT_EQ(SkColorGetG(result), 120);  // not clamped
  EXPECT_EQ(SkColorGetB(result), 220);  // not clamped
}

// =========================================================================
// DarkenColor tests
// =========================================================================

TEST(AstraColorUtilsTest, DarkenColor_WhiteBecomesGray) {
  SkColor result = DarkenColor(kWhite, 128);
  EXPECT_EQ(SkColorGetR(result), 127);
  EXPECT_EQ(SkColorGetG(result), 127);
  EXPECT_EQ(SkColorGetB(result), 127);
}

TEST(AstraColorUtilsTest, DarkenColor_BlackStaysBlack) {
  // Darkening black should not underflow.
  SkColor result = DarkenColor(kBlack, 50);
  EXPECT_EQ(SkColorGetR(result), 0);
  EXPECT_EQ(SkColorGetG(result), 0);
  EXPECT_EQ(SkColorGetB(result), 0);
}

TEST(AstraColorUtilsTest, DarkenColor_ZeroIsNoOp) {
  SkColor result = DarkenColor(kGoogleBlue, 0);
  EXPECT_EQ(result, kGoogleBlue);
}

TEST(AstraColorUtilsTest, DarkenColor_RedChannel) {
  SkColor color = SkColorSetRGB(100, 50, 25);
  SkColor result = DarkenColor(color, 20);
  EXPECT_EQ(SkColorGetR(result), 80);
  EXPECT_EQ(SkColorGetG(result), 30);
  EXPECT_EQ(SkColorGetB(result), 5);
}

TEST(AstraColorUtilsTest, DarkenColor_PartialClamp) {
  SkColor color = SkColorSetRGB(30, 5, 10);
  SkColor result = DarkenColor(color, 20);
  EXPECT_EQ(SkColorGetR(result), 10);   // not clamped
  EXPECT_EQ(SkColorGetG(result), 0);    // clamped
  EXPECT_EQ(SkColorGetB(result), 0);    // clamped
}

// =========================================================================
// BlendColors tests
// =========================================================================

TEST(AstraColorUtilsTest, BlendColors_ZeroAlphaIsBase) {
  // Alpha = 0 → result is the base color.
  SkColor result = BlendColors(kWhite, kBlack, 0.0f);
  EXPECT_EQ(result, kWhite);
}

TEST(AstraColorUtilsTest, BlendColors_FullAlphaIsForeground) {
  // Alpha = 1 → result is the foreground color.
  SkColor result = BlendColors(kWhite, kBlack, 1.0f);
  EXPECT_EQ(result, kBlack);
}

TEST(AstraColorUtilsTest, BlendColors_HalfAlphaIsMidGray) {
  // 50% blend of white and black → mid gray.
  SkColor result = BlendColors(kWhite, kBlack, 0.5f);
  // Should be approximately 128,128,128.
  EXPECT_NEAR(SkColorGetR(result), 128, 1);
  EXPECT_NEAR(SkColorGetG(result), 128, 1);
  EXPECT_NEAR(SkColorGetB(result), 128, 1);
}

TEST(AstraColorUtilsTest, BlendColors_PreservesAlphaOfBase) {
  // The blend result should have the same alpha as the base color.
  SkColor base = SkColorSetARGB(0x80, 0xFF, 0xFF, 0xFF);
  SkColor fore = SkColorSetARGB(0xFF, 0x00, 0x00, 0x00);
  SkColor result = BlendColors(base, fore, 0.5f);
  EXPECT_EQ(SkColorGetA(result), SkColorGetA(base));
}

TEST(AstraColorUtilsTest, BlendColors_RedWithBlue) {
  // Blend red and blue → purple-ish.
  SkColor result = BlendColors(kRed, kBlue, 0.5f);
  EXPECT_GT(SkColorGetR(result), 0);
  EXPECT_GT(SkColorGetB(result), 0);
  EXPECT_EQ(SkColorGetG(result), 0);
}

// =========================================================================
// GetAstraAccentPalette tests
// =========================================================================

TEST(AstraColorUtilsTest, AccentPalette_BaseMatchesInput) {
  AstraAccentPalette palette = GetAstraAccentPalette(kGoogleBlue, false);
  EXPECT_EQ(palette.base, kGoogleBlue);
}

TEST(AstraColorUtilsTest, AccentPalette_AllVariantsExist) {
  AstraAccentPalette palette = GetAstraAccentPalette(kGoogleBlue, false);

  // All fields should be valid colors (non-transparent RGB values).
  EXPECT_NE(SkColorGetR(palette.hover), 0U);
  EXPECT_NE(SkColorGetG(palette.hover), 0U);
  EXPECT_NE(SkColorGetB(palette.hover), 0U);

  EXPECT_NE(SkColorGetR(palette.active), 0U);
  EXPECT_NE(SkColorGetG(palette.active), 0U);
  EXPECT_NE(SkColorGetB(palette.active), 0U);

  // Subtle and border have alpha < 255.
  EXPECT_LT(SkColorGetA(palette.subtle), 255U);
  EXPECT_GT(SkColorGetA(palette.subtle), 0U);
  EXPECT_LT(SkColorGetA(palette.border), 255U);
  EXPECT_GT(SkColorGetA(palette.border), 0U);

  // Text is either white or black.
  EXPECT_TRUE(palette.text == SK_ColorWHITE || palette.text == SK_ColorBLACK);
}

TEST(AstraColorUtilsTest, AccentPalette_VariantsAreDistinct) {
  AstraAccentPalette palette = GetAstraAccentPalette(kGoogleBlue, false);

  // Hover and active should differ from base.
  EXPECT_NE(palette.base, palette.hover);
  EXPECT_NE(palette.base, palette.active);
  EXPECT_NE(palette.hover, palette.active);
}

TEST(AstraColorUtilsTest, AccentPalette_DarkModeIsDifferent) {
  AstraAccentPalette light = GetAstraAccentPalette(kGoogleBlue, false);
  AstraAccentPalette dark = GetAstraAccentPalette(kGoogleBlue, true);

  // Hover and subtle should differ between light and dark modes.
  EXPECT_NE(light.hover, dark.hover);
  EXPECT_NE(light.subtle, dark.subtle);
  EXPECT_NE(light.border, dark.border);
}

TEST(AstraColorUtilsTest, AccentPalette_DarkHoverIsBrighter) {
  // In dark mode, the hover state should be brighter than in light mode.
  SkColor accent = SkColorSetRGB(0x40, 0x80, 0xC0);

  AstraAccentPalette light = GetAstraAccentPalette(accent, false);
  AstraAccentPalette dark = GetAstraAccentPalette(accent, true);

  auto brightness = [](SkColor c) {
    return 0.299f * SkColorGetR(c) + 0.587f * SkColorGetG(c) +
           0.114f * SkColorGetB(c);
  };

  EXPECT_GT(brightness(dark.hover), brightness(light.hover));
}

TEST(AstraColorUtilsTest, AccentPalette_TextHasContrast) {
  // Text color should have good contrast against the base color.
  // For Google Blue (dark-ish), text should be white.
  AstraAccentPalette palette = GetAstraAccentPalette(kGoogleBlue, false);
  EXPECT_EQ(palette.text, SK_ColorWHITE);

  // For a light accent color, text should be black.
  SkColor light_accent = SkColorSetRGB(0xFF, 0xCC, 0x80);
  AstraAccentPalette light_palette = GetAstraAccentPalette(light_accent, false);
  EXPECT_EQ(light_palette.text, SK_ColorBLACK);
}

TEST(AstraColorUtilsTest, AccentPalette_BorderMoreOpaqueThanSubtle) {
  // Border should have higher alpha than subtle.
  AstraAccentPalette palette = GetAstraAccentPalette(kGoogleBlue, false);
  EXPECT_GT(SkColorGetA(palette.border), SkColorGetA(palette.subtle));
}

TEST(AstraColorUtilsTest, AccentPalette_DarkSubtleMoreOpaqueThanLight) {
  // Dark mode subtle tint should be more opaque to be visible against dark bg.
  AstraAccentPalette light = GetAstraAccentPalette(kGoogleBlue, false);
  AstraAccentPalette dark = GetAstraAccentPalette(kGoogleBlue, true);
  EXPECT_GT(SkColorGetA(dark.subtle), SkColorGetA(light.subtle));
}

// =========================================================================
// IsDarkModeActive tests
// =========================================================================

TEST(AstraColorUtilsTest, IsDarkModeActive_ReturnsValidValue) {
  // Just verify the function doesn't crash and returns a boolean.
  // The actual value depends on the system theme, which we can't control.
  bool result = IsDarkModeActive();
  // Result should be a valid bool (implicitly tested by compiling).
  EXPECT_TRUE(result == true || result == false);
}

}  // namespace astra
