// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Unit tests for AstraColorMixer and Astra color IDs.
//
// Tests verify:
//   - All Astra color IDs are registered by the mixer.
//   - Light and dark modes produce different colors where expected.
//   - Accent color derivation produces valid, distinct variants.
//   - No color ID collisions with Chromium's built-in color space.
//   - Color values are valid (non-transparent where expected).

#include "astra/ui/color/astra_color_mixer.h"

#include "astra/ui/color/astra_color_ids.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"

namespace astra {

namespace {

// Test accent color (Google Blue 500) — used consistently across tests.
constexpr SkColor kTestAccent = SkColorSetRGB(0x1A, 0x73, 0xE8);

// Creates a ColorProvider with Astra colors registered for the given mode.
std::unique_ptr<ui::ColorProvider> CreateProviderWithAstraColors(
    bool dark_mode,
    SkColor accent_color = kTestAccent) {
  auto provider = std::make_unique<ui::ColorProvider>();
  AddAstraColorMixer(provider.get(), dark_mode, accent_color);
  return provider;
}

}  // namespace

// =========================================================================
// Color ID range tests
// =========================================================================

TEST(AstraColorMixerTest, ColorIdRangeIsValid) {
  EXPECT_GE(kAstraColorsStart, ui::kUiColorsEnd);
  EXPECT_GT(kAstraColorsEnd, kAstraColorsStart);
  // Ensure we have room for at least the current set of colors.
  EXPECT_LT(kAstraColorsStart + 128, kAstraColorsEnd + 1);
}

TEST(AstraColorMixerTest, NoCollisionWithUiColors) {
  // Astra colors start after UI colors end.
  EXPECT_EQ(kAstraColorsStart, ui::kUiColorsEnd);
  // The first Astra color must be at or after the last UI color.
  EXPECT_GE(kColorAstraSidebarBackground, ui::kUiColorsEnd);
}

// =========================================================================
// Light mode registration tests
// =========================================================================

TEST(AstraColorMixerTest, LightMode_SidebarColorsRegistered) {
  auto provider = CreateProviderWithAstraColors(/*dark_mode=*/false);

  // All sidebar colors should be registered and return valid colors.
  EXPECT_NE(SK_ColorTRANSPARENT,
            provider->GetColor(kColorAstraSidebarBackground));
  EXPECT_NE(SK_ColorTRANSPARENT,
            provider->GetColor(kColorAstraSidebarBorder));
  EXPECT_NE(SK_ColorTRANSPARENT,
            provider->GetColor(kColorAstraSidebarItemHoverBackground));
  EXPECT_NE(SK_ColorTRANSPARENT,
            provider->GetColor(kColorAstraSidebarItemSelectedBackground));
  EXPECT_NE(SK_ColorTRANSPARENT,
            provider->GetColor(kColorAstraSidebarItemText));
  EXPECT_NE(SK_ColorTRANSPARENT,
            provider->GetColor(kColorAstraSidebarItemSelectedText));
  EXPECT_NE(SK_ColorTRANSPARENT,
            provider->GetColor(kColorAstraSidebarSectionHeaderText));
  EXPECT_NE(SK_ColorTRANSPARENT,
            provider->GetColor(kColorAstraSidebarItemSecondaryText));
}

TEST(AstraColorMixerTest, LightMode_WorkspaceAccentColorsRegistered) {
  auto provider = CreateProviderWithAstraColors(/*dark_mode=*/false);

  EXPECT_NE(SK_ColorTRANSPARENT,
            provider->GetColor(kColorAstraWorkspaceAccent));
  EXPECT_NE(SK_ColorTRANSPARENT,
            provider->GetColor(kColorAstraWorkspaceAccentHover));
  EXPECT_NE(SK_ColorTRANSPARENT,
            provider->GetColor(kColorAstraWorkspaceAccentActive));
  // Subtle and border have alpha — just check they're not fully transparent.
  EXPECT_NE(0U, SkColorGetA(provider->GetColor(
                    kColorAstraWorkspaceAccentSubtle)));
  EXPECT_NE(SK_ColorTRANSPARENT,
            provider->GetColor(kColorAstraWorkspaceAccentText));
  EXPECT_NE(0U, SkColorGetA(provider->GetColor(
                    kColorAstraWorkspaceAccentBorder)));
}

TEST(AstraColorMixerTest, LightMode_SplitViewColorsRegistered) {
  auto provider = CreateProviderWithAstraColors(/*dark_mode=*/false);

  EXPECT_NE(SK_ColorTRANSPARENT,
            provider->GetColor(kColorAstraSplitViewDivider));
  EXPECT_NE(SK_ColorTRANSPARENT,
            provider->GetColor(kColorAstraSplitViewDragIndicator));
  EXPECT_NE(SK_ColorTRANSPARENT,
            provider->GetColor(kColorAstraSplitViewBackground));
}

TEST(AstraColorMixerTest, LightMode_CommandPaletteColorsRegistered) {
  auto provider = CreateProviderWithAstraColors(/*dark_mode=*/false);

  EXPECT_NE(SK_ColorTRANSPARENT,
            provider->GetColor(kColorAstraCommandPaletteBackground));
  EXPECT_NE(SK_ColorTRANSPARENT,
            provider->GetColor(kColorAstraCommandPaletteSelectedBackground));
  EXPECT_NE(SK_ColorTRANSPARENT,
            provider->GetColor(kColorAstraCommandPaletteShortcutText));
  EXPECT_NE(SK_ColorTRANSPARENT,
            provider->GetColor(kColorAstraCommandPaletteBorder));
  EXPECT_NE(SK_ColorTRANSPARENT,
            provider->GetColor(kColorAstraCommandPaletteText));
  EXPECT_NE(SK_ColorTRANSPARENT,
            provider->GetColor(kColorAstraCommandPaletteDescriptionText));
  EXPECT_NE(SK_ColorTRANSPARENT,
            provider->GetColor(kColorAstraCommandPaletteSearchText));
}

TEST(AstraColorMixerTest, LightMode_FocusModeColorsRegistered) {
  auto provider = CreateProviderWithAstraColors(/*dark_mode=*/false);

  EXPECT_NE(SK_ColorTRANSPARENT,
            provider->GetColor(kColorAstraFocusModeIndicatorBackground));
  EXPECT_NE(SK_ColorTRANSPARENT,
            provider->GetColor(kColorAstraFocusModeIndicatorText));
  // Dim overlay has alpha — just check it's not fully opaque.
  EXPECT_LT(SkColorGetA(provider->GetColor(kColorAstraFocusModeDimOverlay)),
            255U);
}

TEST(AstraColorMixerTest, LightMode_GlancePeekColorsRegistered) {
  auto provider = CreateProviderWithAstraColors(/*dark_mode=*/false);

  EXPECT_NE(SK_ColorTRANSPARENT,
            provider->GetColor(kColorAstraGlanceBorder));
  EXPECT_NE(SK_ColorTRANSPARENT,
            provider->GetColor(kColorAstraGlanceBackground));
  EXPECT_NE(SK_ColorTRANSPARENT,
            provider->GetColor(kColorAstraPeekHighlight));
  EXPECT_NE(SK_ColorTRANSPARENT,
            provider->GetColor(kColorAstraPeekBackground));
}

// =========================================================================
// Dark mode registration tests
// =========================================================================

TEST(AstraColorMixerTest, DarkMode_AllColorCategoriesRegistered) {
  auto provider = CreateProviderWithAstraColors(/*dark_mode=*/true);

  // Spot-check a few colors from each category to verify dark mode works.
  EXPECT_NE(SK_ColorTRANSPARENT,
            provider->GetColor(kColorAstraSidebarBackground));
  EXPECT_NE(SK_ColorTRANSPARENT,
            provider->GetColor(kColorAstraWorkspaceAccent));
  EXPECT_NE(SK_ColorTRANSPARENT,
            provider->GetColor(kColorAstraSplitViewDivider));
  EXPECT_NE(SK_ColorTRANSPARENT,
            provider->GetColor(kColorAstraCommandPaletteText));
  EXPECT_NE(SK_ColorTRANSPARENT,
            provider->GetColor(kColorAstraFocusModeIndicatorText));
  EXPECT_NE(SK_ColorTRANSPARENT,
            provider->GetColor(kColorAstraGlanceBorder));
}

// =========================================================================
// Light vs dark mode difference tests
// =========================================================================

TEST(AstraColorMixerTest, DarkModeDiffersFromLightMode) {
  auto light_provider = CreateProviderWithAstraColors(/*dark_mode=*/false);
  auto dark_provider = CreateProviderWithAstraColors(/*dark_mode=*/true);

  // Background colors should differ between light and dark modes.
  EXPECT_NE(light_provider->GetColor(kColorAstraSidebarBackground),
            dark_provider->GetColor(kColorAstraSidebarBackground));
  EXPECT_NE(light_provider->GetColor(kColorAstraCommandPaletteBackground),
            dark_provider->GetColor(kColorAstraCommandPaletteBackground));
  EXPECT_NE(light_provider->GetColor(kColorAstraSplitViewBackground),
            dark_provider->GetColor(kColorAstraSplitViewBackground));

  // Text colors should differ (dark text on light bg, light text on dark bg).
  EXPECT_NE(light_provider->GetColor(kColorAstraSidebarItemText),
            dark_provider->GetColor(kColorAstraSidebarItemText));
  EXPECT_NE(light_provider->GetColor(kColorAstraCommandPaletteText),
            dark_provider->GetColor(kColorAstraCommandPaletteText));
}

TEST(AstraColorMixerTest, DarkModeBackgroundsAreDarker) {
  auto light_provider = CreateProviderWithAstraColors(/*dark_mode=*/false);
  auto dark_provider = CreateProviderWithAstraColors(/*dark_mode=*/true);

  // Sidebar background should be darker in dark mode.
  SkColor light_bg = light_provider->GetColor(kColorAstraSidebarBackground);
  SkColor dark_bg = dark_provider->GetColor(kColorAstraSidebarBackground);

  // Compare approximate luminance — dark mode should have lower luminance.
  auto luminance = [](SkColor c) {
    return 0.299f * SkColorGetR(c) + 0.587f * SkColorGetG(c) +
           0.114f * SkColorGetB(c);
  };

  EXPECT_LT(luminance(dark_bg), luminance(light_bg));
}

// =========================================================================
// Accent color derivation tests
// =========================================================================

TEST(AstraColorMixerTest, AccentColorVariantsAreDistinct) {
  auto provider = CreateProviderWithAstraColors(/*dark_mode=*/false, kTestAccent);

  SkColor base = provider->GetColor(kColorAstraWorkspaceAccent);
  SkColor hover = provider->GetColor(kColorAstraWorkspaceAccentHover);
  SkColor active = provider->GetColor(kColorAstraWorkspaceAccentActive);
  SkColor subtle = provider->GetColor(kColorAstraWorkspaceAccentSubtle);
  SkColor text = provider->GetColor(kColorAstraWorkspaceAccentText);
  SkColor border = provider->GetColor(kColorAstraWorkspaceAccentBorder);

  // Base should match the input accent color.
  EXPECT_EQ(base, kTestAccent);

  // Hover and active should be different from base and from each other.
  EXPECT_NE(base, hover);
  EXPECT_NE(base, active);
  EXPECT_NE(hover, active);

  // Subtle should have reduced alpha.
  EXPECT_LT(SkColorGetA(subtle), SkColorGetA(base));
  EXPECT_GT(SkColorGetA(subtle), 0U);

  // Border should have reduced alpha but more than subtle.
  EXPECT_LT(SkColorGetA(border), SkColorGetA(base));
  EXPECT_GT(SkColorGetA(border), SkColorGetA(subtle));

  // Text should be either white or black (contrast color).
  EXPECT_TRUE(text == SK_ColorWHITE || text == SK_ColorBLACK);
}

TEST(AstraColorMixerTest, DifferentAccentColorsProduceDifferentResults) {
  SkColor accent1 = SkColorSetRGB(0xFF, 0x00, 0x00);  // Red
  SkColor accent2 = SkColorSetRGB(0x00, 0xFF, 0x00);  // Green

  auto provider1 = CreateProviderWithAstraColors(/*dark_mode=*/false, accent1);
  auto provider2 = CreateProviderWithAstraColors(/*dark_mode=*/false, accent2);

  // The base accent color should differ.
  EXPECT_NE(provider1->GetColor(kColorAstraWorkspaceAccent),
            provider2->GetColor(kColorAstraWorkspaceAccent));

  // Derived colors should also differ.
  EXPECT_NE(provider1->GetColor(kColorAstraWorkspaceAccentHover),
            provider2->GetColor(kColorAstraWorkspaceAccentHover));
  EXPECT_NE(provider1->GetColor(kColorAstraWorkspaceAccentActive),
            provider2->GetColor(kColorAstraWorkspaceAccentActive));

  // Drag indicator and peek highlight use accent color directly.
  EXPECT_NE(provider1->GetColor(kColorAstraSplitViewDragIndicator),
            provider2->GetColor(kColorAstraSplitViewDragIndicator));
  EXPECT_NE(provider1->GetColor(kColorAstraPeekHighlight),
            provider2->GetColor(kColorAstraPeekHighlight));
}

TEST(AstraColorMixerTest, DarkModeAccentHoverIsBrighter) {
  SkColor accent = SkColorSetRGB(0x40, 0x80, 0xC0);

  auto light_provider = CreateProviderWithAstraColors(/*dark_mode=*/false, accent);
  auto dark_provider = CreateProviderWithAstraColors(/*dark_mode=*/true, accent);

  SkColor light_hover =
      light_provider->GetColor(kColorAstraWorkspaceAccentHover);
  SkColor dark_hover =
      dark_provider->GetColor(kColorAstraWorkspaceAccentHover);

  // In dark mode, hover should be lighter than in light mode
  // (dark mode brightens, light mode darkens).
  auto brightness = [](SkColor c) {
    return 0.299f * SkColorGetR(c) + 0.587f * SkColorGetG(c) +
           0.114f * SkColorGetB(c);
  };

  EXPECT_GT(brightness(dark_hover), brightness(light_hover));
}

// =========================================================================
// Null provider test
// =========================================================================

TEST(AstraColorMixerDeathTest, NullProviderCrashes) {
  // TODO(astra): This test verifies the DCHECK on null provider.
  // In non-DCHECK builds, this may not crash. For the overlay repo
  // skeleton, we skip this test and just verify valid providers work.
  GTEST_SKIP() << "Skipped — DCHECK behavior depends on build config";
}

// =========================================================================
// Color ID ordering tests
// =========================================================================

TEST(AstraColorIdsTest, ColorIdsAreInRange) {
  // All defined color IDs should fall within [kAstraColorsStart, kAstraColorsEnd).
  EXPECT_GE(kColorAstraSidebarBackground, kAstraColorsStart);
  EXPECT_LT(kColorAstraSidebarBackground, kAstraColorsEnd);

  EXPECT_GE(kColorAstraWorkspaceAccent, kAstraColorsStart);
  EXPECT_LT(kColorAstraWorkspaceAccent, kAstraColorsEnd);

  EXPECT_GE(kColorAstraSplitViewDivider, kAstraColorsStart);
  EXPECT_LT(kColorAstraSplitViewDivider, kAstraColorsEnd);

  EXPECT_GE(kColorAstraCommandPaletteBackground, kAstraColorsStart);
  EXPECT_LT(kColorAstraCommandPaletteBackground, kAstraColorsEnd);

  EXPECT_GE(kColorAstraFocusModeIndicatorBackground, kAstraColorsStart);
  EXPECT_LT(kColorAstraFocusModeIndicatorBackground, kAstraColorsEnd);

  EXPECT_GE(kColorAstraGlanceBorder, kAstraColorsStart);
  EXPECT_LT(kColorAstraGlanceBorder, kAstraColorsEnd);

  // The last defined color should be within range.
  EXPECT_GE(kColorAstraPeekBackground, kAstraColorsStart);
  EXPECT_LT(kColorAstraPeekBackground, kAstraColorsEnd);
}

TEST(AstraColorIdsTest, CategoryColorsAreGrouped) {
  // Sidebar colors occupy the first block (0-19).
  EXPECT_EQ(kColorAstraSidebarBackground, kAstraColorsStart + 0);
  EXPECT_LT(kColorAstraSidebarItemSecondaryText, kAstraColorsStart + 20);

  // Workspace accent colors start at offset 20.
  EXPECT_EQ(kColorAstraWorkspaceAccent, kAstraColorsStart + 20);
  EXPECT_LT(kColorAstraWorkspaceAccentBorder, kAstraColorsStart + 40);

  // Split view colors start at offset 40.
  EXPECT_EQ(kColorAstraSplitViewDivider, kAstraColorsStart + 40);
  EXPECT_LT(kColorAstraSplitViewBackground, kAstraColorsStart + 60);

  // Command palette colors start at offset 60.
  EXPECT_EQ(kColorAstraCommandPaletteBackground, kAstraColorsStart + 60);
  EXPECT_LT(kColorAstraCommandPaletteSearchText, kAstraColorsStart + 80);

  // Focus mode colors start at offset 80.
  EXPECT_EQ(kColorAstraFocusModeIndicatorBackground, kAstraColorsStart + 80);
  EXPECT_LT(kColorAstraFocusModeDimOverlay, kAstraColorsStart + 100);

  // Glance/peek colors start at offset 100.
  EXPECT_EQ(kColorAstraGlanceBorder, kAstraColorsStart + 100);
  EXPECT_LT(kColorAstraPeekBackground, kAstraColorsEnd);
}

}  // namespace astra
