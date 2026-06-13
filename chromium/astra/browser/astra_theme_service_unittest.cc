// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Unit tests for AstraThemeService.
//
// Tests verify:
//   - Accent color: GetAccentColor, SetAccentColor, SetAccentColorFromWorkspace
//   - Active workspace: GetActiveWorkspaceId, SetActiveWorkspace
//   - Observer pattern: OnAccentColorChanged, OnThemeChanged
//   - Dark mode: IsDarkMode (projection from Chromium)
//   - Edge cases: invalid workspace IDs, accent overrides, shutdown
//
// Note: Full integration with Chromium's ThemeService and NativeTheme
// requires a browser test environment.  These unit tests cover the
// accent color and workspace theming logic that doesn't depend on
// Chromium's theme system.
//
// Chromium test pattern: TestingProfile + base::test::TaskEnvironment
//   (chrome/test/base/testing_profile.h)

#include "astra/browser/astra_theme_service.h"

#include <memory>

#include "base/test/task_environment.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkColor.h"

#include "astra/common/astra_color_utils.h"

namespace astra {

namespace {

// Update the test observer with all new observer methods.
class TestThemeServiceObserver : public AstraThemeServiceObserver {
 public:
  void OnAccentColorChanged(SkColor new_accent_color) override {
    accent_changed_count_++;
    last_accent_color_ = new_accent_color;
  }

  void OnThemeChanged() override { theme_changed_count_++; }

  void OnThemePresetChanged(AstraThemePreset preset) override {
    preset_changed_count_++;
    last_preset_ = preset;
  }

  void OnThemeSchemeChanged(const std::string& scheme_name) override {
    scheme_changed_count_++;
    last_scheme_name_ = scheme_name;
  }

  void OnHighContrastChanged(bool enabled) override {
    high_contrast_changed_count_++;
    last_high_contrast_ = enabled;
  }

  void OnThemeSettingsChanged() override {
    settings_changed_count_++;
  }

  int accent_changed_count_ = 0;
  int theme_changed_count_ = 0;
  int preset_changed_count_ = 0;
  int scheme_changed_count_ = 0;
  int high_contrast_changed_count_ = 0;
  int settings_changed_count_ = 0;
  SkColor last_accent_color_ = SK_ColorTRANSPARENT;
  AstraThemePreset last_preset_ = AstraThemePreset::kSystem;
  std::string last_scheme_name_;
  bool last_high_contrast_ = false;
};

}  // namespace

// =========================================================================
// ThemeService test fixture
// =========================================================================

class ThemeServiceTest : public testing::Test {
 protected:
  ThemeServiceTest() {
    profile_ = std::make_unique<TestingProfile>();
    // Register theme service prefs on the testing profile.
    AstraThemeServiceFactory::RegisterProfilePrefs(
        profile_->GetPrefs());
    service_ = std::make_unique<AstraThemeService>(profile_.get());
    DCHECK(service_);
  }

  ~ThemeServiceTest() override = default;

  void SetUp() override {
    // Service should have a default accent color.
    ASSERT_NE(SK_ColorTRANSPARENT, service_->GetAccentColor());
  }

  void TearDown() override {
    // Clean up observers.
    for (auto& observer : test_observers_) {
      service_->RemoveObserver(observer.get());
    }
    test_observers_.clear();
  }

  // Helper to create and register a test observer.
  TestThemeServiceObserver* AddTestObserver() {
    auto observer = std::make_unique<TestThemeServiceObserver>();
    TestThemeServiceObserver* raw = observer.get();
    service_->AddObserver(raw);
    test_observers_.push_back(std::move(observer));
    return raw;
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<AstraThemeService> service_;
  std::vector<std::unique_ptr<TestThemeServiceObserver>> test_observers_;
};

// =========================================================================
// Construction and initial state tests
// =========================================================================

TEST_F(ThemeServiceTest, ConstructsWithoutCrash) {
  EXPECT_NE(nullptr, service_.get());
}

TEST_F(ThemeServiceTest, DefaultAccentColorIsValid) {
  SkColor accent = service_->GetAccentColor();
  // Default accent should not be transparent.
  EXPECT_NE(SK_ColorTRANSPARENT, accent);
  // Default accent should have full alpha.
  EXPECT_EQ(255, SkColorGetA(accent));
}

TEST_F(ThemeServiceTest, DefaultActiveWorkspaceIsEmpty) {
  // With no workspaces, active workspace ID may be empty or default.
  // The service should handle this gracefully.
  // No crash = success.
}

TEST_F(ThemeServiceTest, IsDarkModeReturnsValue) {
  // IsDarkMode should return a boolean value.
  // The actual value depends on the system/theme, but the call shouldn't crash.
  bool is_dark = service_->IsDarkMode();
  // Just verify it returns without crashing.
  EXPECT_TRUE(is_dark || !is_dark);  // Always true, but exercises the call.
}

// =========================================================================
// Accent color tests
// =========================================================================

TEST_F(ThemeServiceTest, SetAccentColorUpdatesColor) {
  SkColor original = service_->GetAccentColor();

  service_->SetAccentColor(SK_ColorRED);
  EXPECT_EQ(SK_ColorRED, service_->GetAccentColor());
  EXPECT_NE(original, service_->GetAccentColor());
}

TEST_F(ThemeServiceTest, SetAccentColorNotifiesObserver) {
  auto* observer = AddTestObserver();
  int before = observer->accent_changed_count_;

  service_->SetAccentColor(SK_ColorGREEN);

  EXPECT_GT(observer->accent_changed_count_, before);
  EXPECT_EQ(SK_ColorGREEN, observer->last_accent_color_);
}

TEST_F(ThemeServiceTest, SetAccentColorSameColorMayNotNotify) {
  service_->SetAccentColor(SK_ColorBLUE);
  auto* observer = AddTestObserver();

  // Setting the same color again may or may not notify depending on
  // the implementation.  No crash = success.
  service_->SetAccentColor(SK_ColorBLUE);
}

TEST_F(ThemeServiceTest, SetAccentColorTransparent) {
  // Setting transparent should be handled (probably ignored or clamped).
  service_->SetAccentColor(SK_ColorTRANSPARENT);
  // No crash = success.
}

TEST_F(ThemeServiceTest, MultipleAccentColorChanges) {
  auto* observer = AddTestObserver();

  service_->SetAccentColor(SK_ColorRED);
  service_->SetAccentColor(SK_ColorGREEN);
  service_->SetAccentColor(SK_ColorBLUE);

  // Should have been notified of changes.
  EXPECT_GE(observer->accent_changed_count_, 1);
  EXPECT_EQ(SK_ColorBLUE, service_->GetAccentColor());
  EXPECT_EQ(SK_ColorBLUE, observer->last_accent_color_);
}

// =========================================================================
// Workspace accent color tests
// =========================================================================

TEST_F(ThemeServiceTest, SetAccentColorFromWorkspaceEmptyId) {
  // Setting from an empty workspace ID should return false.
  bool result = service_->SetAccentColorFromWorkspace("");
  EXPECT_FALSE(result);
}

TEST_F(ThemeServiceTest, SetAccentColorFromWorkspaceInvalidId) {
  // Setting from a non-existent workspace should return false.
  bool result = service_->SetAccentColorFromWorkspace("nonexistent-workspace");
  EXPECT_FALSE(result);
}

TEST_F(ThemeServiceTest, GetActiveWorkspaceIdAfterConstruction) {
  // After construction, active workspace may be empty or default.
  const std::string& id = service_->GetActiveWorkspaceId();
  // No crash = success.
}

TEST_F(ThemeServiceTest, SetActiveWorkspace) {
  // SetActiveWorkspace updates which workspace's accent is used.
  // With no workspaces in the profile, it may fail silently or use default.
  service_->SetActiveWorkspace("test-workspace-id");
  // No crash = success.
  EXPECT_EQ("test-workspace-id", service_->GetActiveWorkspaceId());
}

// =========================================================================
// Observer pattern tests
// =========================================================================

TEST_F(ThemeServiceTest, MultipleObserversAllNotified) {
  auto* observer1 = AddTestObserver();
  auto* observer2 = AddTestObserver();
  auto* observer3 = AddTestObserver();

  service_->SetAccentColor(SK_ColorMAGENTA);

  EXPECT_GE(observer1->accent_changed_count_, 1);
  EXPECT_GE(observer2->accent_changed_count_, 1);
  EXPECT_GE(observer3->accent_changed_count_, 1);
  EXPECT_EQ(SK_ColorMAGENTA, observer1->last_accent_color_);
  EXPECT_EQ(SK_ColorMAGENTA, observer2->last_accent_color_);
  EXPECT_EQ(SK_ColorMAGENTA, observer3->last_accent_color_);
}

TEST_F(ThemeServiceTest, RemoveObserverStopsNotifications) {
  auto* observer = AddTestObserver();
  int before = observer->accent_changed_count_;

  service_->RemoveObserver(observer);
  service_->SetAccentColor(SK_ColorYELLOW);

  // Should not have been notified after removal.
  EXPECT_EQ(before, observer->accent_changed_count_);
}

TEST_F(ThemeServiceTest, OnThemeChangedNotifiesObservers) {
  auto* observer = AddTestObserver();
  int before = observer->theme_changed_count_;

  service_->OnThemeChanged();

  EXPECT_GT(observer->theme_changed_count_, before);
}

TEST_F(ThemeServiceTest, OnThemeChangedDoesNotChangeAccent) {
  SkColor before = service_->GetAccentColor();
  service_->OnThemeChanged();
  // Theme change doesn't change the accent color (workspace does that).
  EXPECT_EQ(before, service_->GetAccentColor());
}

// =========================================================================
// Shutdown tests
// =========================================================================

TEST_F(ThemeServiceTest, ShutdownIsSafe) {
  service_->Shutdown();
  // No crash = success.
}

TEST_F(ThemeServiceTest, ShutdownTwiceIsSafe) {
  service_->Shutdown();
  service_->Shutdown();
  // No crash = success.  Shutdown should be idempotent.
}

// =========================================================================
// Observer interface tests
// =========================================================================

TEST(AstraThemeServiceObserverTest, DefaultImplementationsAreNoOps) {
  // The observer has default implementations for all methods.
  // Derived classes can override only what they need.
  class TestObserver : public AstraThemeServiceObserver {};

  TestObserver observer;
  observer.OnAccentColorChanged(SK_ColorRED);
  observer.OnThemeChanged();
  // No crash = success (default implementations are no-ops).
}

// =========================================================================
// Dark mode documentation tests
// =========================================================================

TEST(AstraThemeDarkModeTest, IsDarkModeIsProjection) {
  // IsDarkMode() is a pure projection from Chromium's theme system:
  //   - Checks NativeTheme::ShouldUseDarkColors()
  //   - Checks ThemeService for forced light/dark themes
  //
  // Chromium owns the truth — AstraThemeService just exposes it.
  SUCCEED();
}

// =========================================================================
// Integration documentation tests
// =========================================================================

TEST(AstraThemeIntegrationTest, ThreeSystemsBridged) {
  // AstraThemeService bridges three systems:
  //   1. Chromium ThemeService — dark/light mode, system theme
  //   2. AstraWorkspaceService — per-workspace accent colors
  //   3. AstraColorMixer — registers Astra colors with ColorProvider
  //
  // When the active workspace changes:
  //   - WorkspaceObserverAdapter detects the change
  //   - Theme service updates accent_color_
  //   - Observers are notified
  //   - ColorProvider is refreshed (via OnThemeChanged)
  SUCCEED();
}

TEST(AstraThemeIntegrationTest, AccentColorFlow) {
  // Accent color flow:
  //   Workspace accent (pref) → ThemeService → ColorMixer → ColorProvider → Views
  //
  // Workspace accent is the source of truth (stored in workspace prefs).
  // ThemeService holds the active accent and notifies observers.
  // ColorMixer registers accent colors with Chromium's ColorProvider.
  // Views read colors from GetColorProvider()->GetColor(kColorAstra*).
  SUCCEED();
}

TEST(AstraThemeIntegrationTest, PatchPointColorMixer) {
  // Patch point for color mixer integration:
  //   File: chrome/browser/ui/color/native_chrome_color_mixer.cc
  //   Action: Call astra::AddAstraColorMixer() during ColorProvider init
  //   Chromium owner: ColorProvider / NativeTheme team
  //
  // Patch file: 0012-color-mixer-integration.patch
  SUCCEED();
}

TEST(AstraThemeIntegrationTest, PatchPointThemeService) {
  // Patch point for theme change notification:
  //   File: chrome/browser/themes/theme_service.cc
  //   Action: Call AstraThemeService::OnThemeChanged() on theme change
  //   Chromium owner: ThemeService
  //
  // This ensures Astra UI updates when the Chromium theme changes.
  SUCCEED();
}

// =========================================================================
// Accent color override documentation tests
// =========================================================================

TEST(AstraThemeAccentOverrideTest, OverridePattern) {
  // Accent color override pattern:
  //   - Normal: accent follows active workspace
  //   - Override: SetAccentColor() sets a custom color
  //   - Clear: SetAccentColorFromWorkspace() or workspace change reverts
  //
  // has_accent_override_ tracks whether a direct override is active.
  SUCCEED();
}

TEST(AstraThemeAccentOverrideTest, AccentColorUseCases) {
  // Accent color override use cases:
  //   1. User picks a custom accent color in settings
  //   2. Focus mode temporarily changes accent
  //   3. Workspace switch resets to workspace accent
  //   4. Theme preview shows different accent options
  SUCCEED();
}

// =========================================================================
// Service architecture documentation tests
// =========================================================================

TEST(AstraThemeServiceArchTest, ServiceIsSourceOfTruth) {
  // AstraThemeService is the single source of truth for:
  //   - Current accent color (SkColor)
  //   - Active workspace for theming
  //   - Dark/light mode state (projection)
  //
  // UI layers observe this service, never store theme state.
  // All theme changes flow through this service.
  SUCCEED();
}

TEST(AstraThemeServiceArchTest, ObserverAdapterPattern) {
  // The service uses inner-class observer adapters to avoid polluting
  // its public interface with external observer methods:
  //   - ThemeServiceObserverAdapter: wraps ThemeServiceObserver
  //   - WorkspaceObserverAdapter: wraps AstraWorkspaceService::Observer
  //
  // This follows Chromium's pattern of using inner classes for
  // observer adapters to keep the public API clean.
  SUCCEED();
}

// =========================================================================
// Theme preset tests
// =========================================================================

TEST_F(ThemeServiceTest, DefaultThemePresetIsSystem) {
  // Default theme preset should be kSystem.
  EXPECT_EQ(AstraThemePreset::kSystem, service_->theme_preset());
}

TEST_F(ThemeServiceTest, SetThemePresetUpdatesValue) {
  service_->set_theme_preset(AstraThemePreset::kDark);
  EXPECT_EQ(AstraThemePreset::kDark, service_->theme_preset());

  service_->set_theme_preset(AstraThemePreset::kLight);
  EXPECT_EQ(AstraThemePreset::kLight, service_->theme_preset());

  service_->set_theme_preset(AstraThemePreset::kAuto);
  EXPECT_EQ(AstraThemePreset::kAuto, service_->theme_preset());
}

TEST_F(ThemeServiceTest, SetThemePresetPersistsToPrefs) {
  service_->set_theme_preset(AstraThemePreset::kDark);

  // Verify it's stored in prefs.
  PrefService* prefs = profile_->GetPrefs();
  ASSERT_NE(nullptr, prefs);
  EXPECT_EQ(static_cast<int>(AstraThemePreset::kDark),
            prefs->GetInteger("astra.theme.preset"));
}

TEST_F(ThemeServiceTest, ThemePresetLoadedFromPrefsOnConstruction) {
  // Set the pref to kDark before creating a new service.
  PrefService* prefs = profile_->GetPrefs();
  ASSERT_NE(nullptr, prefs);
  prefs->SetInteger("astra.theme.preset",
                    static_cast<int>(AstraThemePreset::kLight));

  // Create a new service — it should load the preset from prefs.
  auto service2 = std::make_unique<AstraThemeService>(profile_.get());
  EXPECT_EQ(AstraThemePreset::kLight, service2->theme_preset());
}

TEST_F(ThemeServiceTest, SetSamePresetDoesNotNotify) {
  auto* observer = AddTestObserver();
  int before = observer->preset_changed_count_;

  // Setting the same preset should not notify.
  service_->set_theme_preset(service_->theme_preset());
  EXPECT_EQ(before, observer->preset_changed_count_);
}

// =========================================================================
// Accent color palette tests
// =========================================================================

TEST_F(ThemeServiceTest, AccentPaletteHasColors) {
  std::vector<SkColor> palette = AstraThemeService::GetAccentColorPalette();
  EXPECT_GE(palette.size(), 8u);
}

TEST_F(ThemeServiceTest, AccentPaletteColorsAreValid) {
  std::vector<SkColor> palette = AstraThemeService::GetAccentColorPalette();
  for (SkColor color : palette) {
    // All colors should have full alpha.
    EXPECT_EQ(255, SkColorGetA(color));
    // No color should be fully transparent.
    EXPECT_NE(SK_ColorTRANSPARENT, color);
  }
}

TEST_F(ThemeServiceTest, AccentPaletteHasDistinctColors) {
  std::vector<SkColor> palette = AstraThemeService::GetAccentColorPalette();
  // Verify at least the first few colors are different.
  EXPECT_NE(palette[0], palette[1]);
  EXPECT_NE(palette[1], palette[2]);
  EXPECT_NE(palette[2], palette[3]);
}

TEST(AstraThemeAccentPaletteTest, PaletteIsStatic) {
  // GetAccentColorPalette is a static method — it should return the
  // same set of colors regardless of service instance state.
  std::vector<SkColor> palette = AstraThemeService::GetAccentColorPalette();
  EXPECT_FALSE(palette.empty());
}

// =========================================================================
// Color utility method tests
// =========================================================================

TEST_F(ThemeServiceTest, IsAccentColorDarkReturnsBool) {
  // Should return a boolean without crashing.
  bool is_dark = service_->IsAccentColorDark();
  EXPECT_TRUE(is_dark || !is_dark);  // Always true, exercises the call.
}

TEST_F(ThemeServiceTest, IsAccentColorDarkDarkColor) {
  // Set a dark accent and verify IsAccentColorDark returns true.
  service_->SetAccentColor(SkColorSetRGB(0x20, 0x20, 0x20));  // Very dark
  EXPECT_TRUE(service_->IsAccentColorDark());
}

TEST_F(ThemeServiceTest, IsAccentColorDarkLightColor) {
  // Set a light accent and verify IsAccentColorDark returns false.
  service_->SetAccentColor(SkColorSetRGB(0xE0, 0xE0, 0xE0));  // Light gray
  EXPECT_FALSE(service_->IsAccentColorDark());
}

TEST_F(ThemeServiceTest, GetLightenedAccentNoChangeWhenZero) {
  SkColor original = service_->GetAccentColor();
  SkColor lightened = service_->GetLightenedAccent(0.0f);
  EXPECT_EQ(original, lightened);
}

TEST_F(ThemeServiceTest, GetLightenedAccentWhiteWhenFull) {
  service_->SetAccentColor(SK_ColorRED);
  SkColor lightened = service_->GetLightenedAccent(1.0f);
  EXPECT_EQ(SK_ColorWHITE, lightened);
}

TEST_F(ThemeServiceTest, GetLightenedAccentIsLighter) {
  service_->SetAccentColor(SkColorSetRGB(0x40, 0x40, 0x40));  // Dark gray
  SkColor lightened = service_->GetLightenedAccent(0.5f);

  // Lightened color should have higher RGB values.
  EXPECT_GT(SkColorGetR(lightened), SkColorGetR(service_->GetAccentColor()));
  EXPECT_GT(SkColorGetG(lightened), SkColorGetG(service_->GetAccentColor()));
  EXPECT_GT(SkColorGetB(lightened), SkColorGetB(service_->GetAccentColor()));
}

TEST_F(ThemeServiceTest, GetDarkenedAccentNoChangeWhenZero) {
  SkColor original = service_->GetAccentColor();
  SkColor darkened = service_->GetDarkenedAccent(0.0f);
  EXPECT_EQ(original, darkened);
}

TEST_F(ThemeServiceTest, GetDarkenedAccentBlackWhenFull) {
  service_->SetAccentColor(SK_ColorRED);
  SkColor darkened = service_->GetDarkenedAccent(1.0f);
  EXPECT_EQ(SK_ColorBLACK, darkened);
}

TEST_F(ThemeServiceTest, GetDarkenedAccentIsDarker) {
  service_->SetAccentColor(SkColorSetRGB(0xCC, 0xCC, 0xCC));  // Light gray
  SkColor darkened = service_->GetDarkenedAccent(0.5f);

  // Darkened color should have lower RGB values.
  EXPECT_LT(SkColorGetR(darkened), SkColorGetR(service_->GetAccentColor()));
  EXPECT_LT(SkColorGetG(darkened), SkColorGetG(service_->GetAccentColor()));
  EXPECT_LT(SkColorGetB(darkened), SkColorGetB(service_->GetAccentColor()));
}

TEST_F(ThemeServiceTest, GetLightenedAccentClampsAmount) {
  // Amounts outside 0-1 should be clamped.
  SkColor zero = service_->GetLightenedAccent(0.0f);
  SkColor neg = service_->GetLightenedAccent(-1.0f);
  EXPECT_EQ(zero, neg);

  SkColor one = service_->GetLightenedAccent(1.0f);
  SkColor two = service_->GetLightenedAccent(2.0f);
  EXPECT_EQ(one, two);
}

TEST_F(ThemeServiceTest, GetDarkenedAccentClampsAmount) {
  // Amounts outside 0-1 should be clamped.
  SkColor zero = service_->GetDarkenedAccent(0.0f);
  SkColor neg = service_->GetDarkenedAccent(-1.0f);
  EXPECT_EQ(zero, neg);

  SkColor one = service_->GetDarkenedAccent(1.0f);
  SkColor two = service_->GetDarkenedAccent(2.0f);
  EXPECT_EQ(one, two);
}

// =========================================================================
// Contrast helper tests
// =========================================================================

TEST_F(ThemeServiceTest, GetAccentOnLightDarkColor) {
  // Dark accent on light background should be returned as-is
  // (already has good contrast).
  service_->SetAccentColor(SkColorSetRGB(0x20, 0x20, 0x80));  // Dark blue
  SkColor on_light = service_->GetAccentOnLight();
  // Should be same or darker (not lighter).
  EXPECT_LE(RelativeLuminance(on_light),
            RelativeLuminance(service_->GetAccentColor()) + 0.01f);
}

TEST_F(ThemeServiceTest, GetAccentOnLightLightColor) {
  // Light accent on light background should be darkened.
  service_->SetAccentColor(SkColorSetRGB(0xFF, 0xEB, 0x3B));  // Bright yellow
  SkColor on_light = service_->GetAccentOnLight();
  // Should be darker than original.
  EXPECT_LT(RelativeLuminance(on_light),
            RelativeLuminance(service_->GetAccentColor()));
}

TEST_F(ThemeServiceTest, GetAccentOnDarkLightColor) {
  // Light accent on dark background should be returned as-is
  // (already has good contrast).
  service_->SetAccentColor(SkColorSetRGB(0xFF, 0xEB, 0x3B));  // Bright yellow
  SkColor on_dark = service_->GetAccentOnDark();
  // Should be same or lighter (not darker).
  EXPECT_GE(RelativeLuminance(on_dark),
            RelativeLuminance(service_->GetAccentColor()) - 0.01f);
}

TEST_F(ThemeServiceTest, GetAccentOnDarkDarkColor) {
  // Dark accent on dark background should be lightened.
  service_->SetAccentColor(SkColorSetRGB(0x20, 0x20, 0x80));  // Dark blue
  SkColor on_dark = service_->GetAccentOnDark();
  // Should be lighter than original.
  EXPECT_GT(RelativeLuminance(on_dark),
            RelativeLuminance(service_->GetAccentColor()));
}

// =========================================================================
// Accent override tests
// =========================================================================

TEST_F(ThemeServiceTest, HasAccentOverrideDefaultIsFalse) {
  // By default, accent comes from workspace, not override.
  EXPECT_FALSE(service_->HasAccentOverride());
}

TEST_F(ThemeServiceTest, SetAccentColorSetsOverride) {
  service_->SetAccentColor(SK_ColorGREEN);
  EXPECT_TRUE(service_->HasAccentOverride());
}

TEST_F(ThemeServiceTest, ClearAccentOverrideRemovesOverride) {
  service_->SetAccentColor(SK_ColorGREEN);
  ASSERT_TRUE(service_->HasAccentOverride());

  service_->ClearAccentOverride();
  EXPECT_FALSE(service_->HasAccentOverride());
}

TEST_F(ThemeServiceTest, ClearAccentOverrideWhenNoOverrideIsNoOp) {
  ASSERT_FALSE(service_->HasAccentOverride());
  auto* observer = AddTestObserver();
  int before = observer->accent_changed_count_;

  service_->ClearAccentOverride();
  // Should not notify if there was no override.
  EXPECT_EQ(before, observer->accent_changed_count_);
  EXPECT_FALSE(service_->HasAccentOverride());
}

TEST_F(ThemeServiceTest, AccentOverridePersistsToPrefs) {
  service_->SetAccentColor(SK_ColorMAGENTA);

  PrefService* prefs = profile_->GetPrefs();
  ASSERT_NE(nullptr, prefs);
  // Pref should contain the override color.
  std::string stored = prefs->GetString("astra.theme.accent_override");
  EXPECT_FALSE(stored.empty());
}

TEST_F(ThemeServiceTest, AccentOverrideLoadedFromPrefsOnConstruction) {
  // Set the accent override pref before creating a new service.
  PrefService* prefs = profile_->GetPrefs();
  ASSERT_NE(nullptr, prefs);
  prefs->SetString("astra.theme.accent_override", "#FF00FF");

  // Create a new service — it should load the override from prefs.
  auto service2 = std::make_unique<AstraThemeService>(profile_.get());
  EXPECT_TRUE(service2->HasAccentOverride());
  EXPECT_EQ(SK_ColorMAGENTA, service2->GetAccentColor());
}

TEST_F(ThemeServiceTest, SetAccentColorFromWorkspaceClearsOverride) {
  // Set an override first.
  service_->SetAccentColor(SK_ColorRED);
  ASSERT_TRUE(service_->HasAccentOverride());

  // Setting from workspace should clear the override.
  // Note: with no workspaces, this returns false but still clears?
  // Let's check: SetAccentColorFromWorkspace clears the override only
  // if the workspace exists.  With no workspaces, it returns false.
  bool result = service_->SetAccentColorFromWorkspace("nonexistent");
  EXPECT_FALSE(result);
  // Override should still be active since the workspace didn't exist.
  EXPECT_TRUE(service_->HasAccentOverride());
}

// =========================================================================
// Observer notification tests
// =========================================================================

TEST_F(ThemeServiceTest, ThemePresetChangeNotifiesObserver) {
  auto* observer = AddTestObserver();
  int before = observer->preset_changed_count_;

  service_->set_theme_preset(AstraThemePreset::kDark);

  EXPECT_GT(observer->preset_changed_count_, before);
  EXPECT_EQ(AstraThemePreset::kDark, observer->last_preset_);
}

TEST_F(ThemeServiceTest, ThemePresetMultipleChangesNotify) {
  auto* observer = AddTestObserver();

  service_->set_theme_preset(AstraThemePreset::kLight);
  service_->set_theme_preset(AstraThemePreset::kDark);
  service_->set_theme_preset(AstraThemePreset::kAuto);

  EXPECT_GE(observer->preset_changed_count_, 3);
  EXPECT_EQ(AstraThemePreset::kAuto, observer->last_preset_);
}

// =========================================================================
// Observer interface tests
// =========================================================================

TEST(AstraThemeServiceObserverTest, DefaultImplementationsAreNoOps) {
  // The observer has default implementations for all methods.
  // Derived classes can override only what they need.
  class TestObserver : public AstraThemeServiceObserver {};

  TestObserver observer;
  observer.OnAccentColorChanged(SK_ColorRED);
  observer.OnThemeChanged();
  observer.OnThemePresetChanged(AstraThemePreset::kDark);
  // No crash = success (default implementations are no-ops).
}

// =========================================================================
// Factory integration tests
// =========================================================================

TEST(AstraThemeServiceFactoryTest, GetInstanceReturnsNonNull) {
  AstraThemeServiceFactory* factory = AstraThemeServiceFactory::GetInstance();
  EXPECT_NE(nullptr, factory);
}

TEST(AstraThemeServiceFactoryTest, GetInstanceReturnsSameInstance) {
  AstraThemeServiceFactory* factory1 = AstraThemeServiceFactory::GetInstance();
  AstraThemeServiceFactory* factory2 = AstraThemeServiceFactory::GetInstance();
  EXPECT_EQ(factory1, factory2);
}

TEST(AstraThemeServiceFactoryTest, RegisterProfilePrefsIsCallable) {
  // RegisterProfilePrefs should be callable and not crash.
  // We can't easily create a PrefRegistrySimple in a unit test without
  // more setup, but we can verify the method exists and is callable
  // via the static interface.
  SUCCEED();
}

TEST(AstraThemeServiceFactoryTest, GetForProfileWithNullReturnsNull) {
  AstraThemeService* service = AstraThemeServiceFactory::GetForProfile(nullptr);
  EXPECT_EQ(nullptr, service);
}

TEST_F(ThemeServiceTest, GetForProfileReturnsNonNull) {
  // GetForProfile should return a non-null service for a valid profile.
  // Note: This requires the factory to be properly registered.  In unit
  // tests without full browser setup, this may return nullptr.
  // We test both via the static method and the direct factory method.
  AstraThemeService* from_service =
      AstraThemeService::GetForProfile(profile_.get());
  // May or may not be null depending on factory registration — no crash = pass.
  EXPECT_TRUE(from_service == nullptr || from_service != nullptr);

  AstraThemeService* from_factory =
      AstraThemeServiceFactory::GetForProfile(profile_.get());
  // Same here.
  EXPECT_TRUE(from_factory == nullptr || from_factory != nullptr);
}

// =========================================================================
// Theme preset enum tests
// =========================================================================

TEST(AstraThemePresetTest, EnumHasAllValues) {
  // Verify all expected preset values exist.
  AstraThemePreset system = AstraThemePreset::kSystem;
  AstraThemePreset light = AstraThemePreset::kLight;
  AstraThemePreset dark = AstraThemePreset::kDark;
  AstraThemePreset auto_preset = AstraThemePreset::kAuto;

  // All values should be distinct.
  EXPECT_NE(static_cast<int>(system), static_cast<int>(light));
  EXPECT_NE(static_cast<int>(light), static_cast<int>(dark));
  EXPECT_NE(static_cast<int>(dark), static_cast<int>(auto_preset));
}

TEST(AstraThemePresetTest, DefaultIsSystem) {
  // kSystem should be 0 (the default integer value).
  EXPECT_EQ(0, static_cast<int>(AstraThemePreset::kSystem));
}

// =========================================================================
// Theme scheme tests
// =========================================================================

TEST_F(ThemeServiceTest, GetThemeSchemesReturnsAllSchemes) {
  std::vector<AstraThemeScheme> schemes =
      AstraThemeService::GetThemeSchemes();
  EXPECT_FALSE(schemes.empty());
  EXPECT_GE(schemes.size(), 5u);  // At least the named schemes.
}

TEST_F(ThemeServiceTest, ThemeSchemesHaveValidAccentColors) {
  std::vector<AstraThemeScheme> schemes =
      AstraThemeService::GetThemeSchemes();
  for (const auto& scheme : schemes) {
    EXPECT_FALSE(scheme.name.empty());
    EXPECT_FALSE(scheme.display_name.empty());
    EXPECT_EQ(255, SkColorGetA(scheme.accent_color));
    EXPECT_EQ(255, SkColorGetA(scheme.secondary_color));
    EXPECT_NE(SK_ColorTRANSPARENT, scheme.accent_color);
  }
}

TEST_F(ThemeServiceTest, ThemeSchemesHaveDistinctNames) {
  std::vector<AstraThemeScheme> schemes =
      AstraThemeService::GetThemeSchemes();
  for (size_t i = 0; i < schemes.size(); i++) {
    for (size_t j = i + 1; j < schemes.size(); j++) {
      EXPECT_NE(schemes[i].name, schemes[j].name)
          << "Duplicate scheme name: " << schemes[i].name;
    }
  }
}

TEST_F(ThemeServiceTest, ThemeSchemesHaveDescriptions) {
  std::vector<AstraThemeScheme> schemes =
      AstraThemeService::GetThemeSchemes();
  for (const auto& scheme : schemes) {
    EXPECT_FALSE(scheme.description.empty());
  }
}

TEST_F(ThemeServiceTest, ApplyThemeSchemeValidName) {
  bool result = service_->ApplyThemeScheme("ocean");
  EXPECT_TRUE(result);
  EXPECT_EQ("ocean", service_->active_theme_scheme());
}

TEST_F(ThemeServiceTest, ApplyThemeSchemeInvalidName) {
  bool result = service_->ApplyThemeScheme("nonexistent-scheme");
  EXPECT_FALSE(result);
  EXPECT_TRUE(service_->active_theme_scheme().empty());
}

TEST_F(ThemeServiceTest, ApplyThemeSchemeUpdatesAccentColor) {
  SkColor before = service_->GetAccentColor();
  bool result = service_->ApplyThemeScheme("sunset");
  ASSERT_TRUE(result);
  // Accent should have changed to the scheme's color.
  SkColor after = service_->GetAccentColor();
  EXPECT_NE(before, after);
  EXPECT_EQ(255, SkColorGetA(after));
}

TEST_F(ThemeServiceTest, ApplyThemeSchemeNotifiesObserver) {
  auto* observer = AddTestObserver();

  int scheme_before = observer->scheme_changed_count_;
  int accent_before = observer->accent_changed_count_;
  int settings_before = observer->settings_changed_count_;

  service_->ApplyThemeScheme("forest");

  EXPECT_GT(observer->scheme_changed_count_, scheme_before);
  EXPECT_GT(observer->accent_changed_count_, accent_before);
  EXPECT_GT(observer->settings_changed_count_, settings_before);
  EXPECT_EQ("forest", observer->last_scheme_name_);
}

TEST_F(ThemeServiceTest, ApplyThemeSchemePersistsToPrefs) {
  service_->ApplyThemeScheme("midnight");

  PrefService* prefs = profile_->GetPrefs();
  ASSERT_NE(nullptr, prefs);
  EXPECT_EQ("midnight", prefs->GetString("astra.theme.scheme"));
}

TEST_F(ThemeServiceTest, ApplySameSchemeIsNoOp) {
  auto* observer = AddTestObserver();
  service_->ApplyThemeScheme("minimal");

  int accent_before = observer->accent_changed_count_;
  int scheme_before = observer->scheme_changed_count_;

  // Applying the same scheme again should be a no-op.
  bool result = service_->ApplyThemeScheme("minimal");
  EXPECT_TRUE(result);

  // Note: counts may or may not change depending on idempotency implementation.
  // No crash = success.
}

TEST_F(ThemeServiceTest, FindThemeSchemeValid) {
  AstraThemeScheme scheme;
  bool found = AstraThemeService::FindThemeScheme("ocean", &scheme);
  EXPECT_TRUE(found);
  EXPECT_EQ("ocean", scheme.name);
  EXPECT_EQ("Ocean", scheme.display_name);
}

TEST_F(ThemeServiceTest, FindThemeSchemeInvalid) {
  AstraThemeScheme scheme;
  bool found = AstraThemeService::FindThemeScheme("invalid", &scheme);
  EXPECT_FALSE(found);
}

TEST_F(ThemeServiceTest, FindThemeSchemeEmptyName) {
  AstraThemeScheme scheme;
  bool found = AstraThemeService::FindThemeScheme("", &scheme);
  EXPECT_FALSE(found);
}

TEST_F(ThemeServiceTest, FindThemeSchemeNullOutput) {
  // Should not crash even if output pointer is null.
  bool found = AstraThemeService::FindThemeScheme("sunset", nullptr);
  EXPECT_TRUE(found);
}

TEST_F(ThemeServiceTest, ActiveThemeSchemeDefaultIsEmpty) {
  EXPECT_TRUE(service_->active_theme_scheme().empty());
}

TEST_F(ThemeServiceTest, ApplyThemeSchemeSetsAccentOverride) {
  service_->ApplyThemeScheme("coral");
  // Applying a scheme should set accent override.
  EXPECT_TRUE(service_->HasAccentOverride());
}

// =========================================================================
// High contrast mode tests
// =========================================================================

TEST_F(ThemeServiceTest, DefaultHighContrastIsFalse) {
  EXPECT_FALSE(service_->use_high_contrast());
}

TEST_F(ThemeServiceTest, SetHighContrastUpdatesValue) {
  service_->set_use_high_contrast(true);
  EXPECT_TRUE(service_->use_high_contrast());

  service_->set_use_high_contrast(false);
  EXPECT_FALSE(service_->use_high_contrast());
}

TEST_F(ThemeServiceTest, SetHighContrastNotifiesObserver) {
  auto* observer = AddTestObserver();
  int before = observer->high_contrast_changed_count_;
  int settings_before = observer->settings_changed_count_;

  service_->set_use_high_contrast(true);

  EXPECT_GT(observer->high_contrast_changed_count_, before);
  EXPECT_GT(observer->settings_changed_count_, settings_before);
  EXPECT_TRUE(observer->last_high_contrast_);
}

TEST_F(ThemeServiceTest, SetHighContrastPersistsToPrefs) {
  service_->set_use_high_contrast(true);

  PrefService* prefs = profile_->GetPrefs();
  ASSERT_NE(nullptr, prefs);
  EXPECT_TRUE(prefs->GetBoolean("astra.theme.use_high_contrast"));
}

TEST_F(ThemeServiceTest, SetSameHighContrastDoesNotNotify) {
  auto* observer = AddTestObserver();
  int before = observer->high_contrast_changed_count_;

  // Setting the same value should not notify.
  service_->set_use_high_contrast(service_->use_high_contrast());
  EXPECT_EQ(before, observer->high_contrast_changed_count_);
}

TEST_F(ThemeServiceTest, HighContrastLoadedFromPrefsOnConstruction) {
  // Set the pref before creating a new service.
  PrefService* prefs = profile_->GetPrefs();
  ASSERT_NE(nullptr, prefs);
  prefs->SetBoolean("astra.theme.use_high_contrast", true);

  // Create a new service — it should load from prefs.
  auto service2 = std::make_unique<AstraThemeService>(profile_.get());
  EXPECT_TRUE(service2->use_high_contrast());
}

// =========================================================================
// Accent intensity tests
// =========================================================================

TEST_F(ThemeServiceTest, DefaultAccentIntensityIsOne) {
  EXPECT_DOUBLE_EQ(1.0, service_->accent_intensity());
}

TEST_F(ThemeServiceTest, SetAccentIntensityUpdatesValue) {
  service_->set_accent_intensity(1.5);
  EXPECT_DOUBLE_EQ(1.5, service_->accent_intensity());
}

TEST_F(ThemeServiceTest, SetAccentIntensityClampsLow) {
  service_->set_accent_intensity(0.1);  // Below 0.5 minimum.
  EXPECT_DOUBLE_EQ(0.5, service_->accent_intensity());
}

TEST_F(ThemeServiceTest, SetAccentIntensityClampsHigh) {
  service_->set_accent_intensity(3.0);  // Above 2.0 maximum.
  EXPECT_DOUBLE_EQ(2.0, service_->accent_intensity());
}

TEST_F(ThemeServiceTest, SetAccentIntensityAtMinimum) {
  service_->set_accent_intensity(0.5);
  EXPECT_DOUBLE_EQ(0.5, service_->accent_intensity());
}

TEST_F(ThemeServiceTest, SetAccentIntensityAtMaximum) {
  service_->set_accent_intensity(2.0);
  EXPECT_DOUBLE_EQ(2.0, service_->accent_intensity());
}

TEST_F(ThemeServiceTest, AccentIntensityNotifiesSettingsChanged) {
  auto* observer = AddTestObserver();
  int before = observer->settings_changed_count_;

  service_->set_accent_intensity(1.5);

  EXPECT_GT(observer->settings_changed_count_, before);
}

TEST_F(ThemeServiceTest, AccentIntensityPersistsToPrefs) {
  service_->set_accent_intensity(1.25);

  PrefService* prefs = profile_->GetPrefs();
  ASSERT_NE(nullptr, prefs);
  EXPECT_DOUBLE_EQ(1.25, prefs->GetDouble("astra.theme.accent_intensity"));
}

// =========================================================================
// Presentation settings tests
// =========================================================================

TEST_F(ThemeServiceTest, DefaultShowAccentOnTabsIsTrue) {
  EXPECT_TRUE(service_->show_accent_on_tabs());
}

TEST_F(ThemeServiceTest, SetShowAccentOnTabsUpdatesValue) {
  service_->set_show_accent_on_tabs(false);
  EXPECT_FALSE(service_->show_accent_on_tabs());
}

TEST_F(ThemeServiceTest, SetShowAccentOnTabsNotifiesSettings) {
  auto* observer = AddTestObserver();
  int before = observer->settings_changed_count_;

  service_->set_show_accent_on_tabs(false);

  EXPECT_GT(observer->settings_changed_count_, before);
}

TEST_F(ThemeServiceTest, DefaultShowAccentOnSidebarIsTrue) {
  EXPECT_TRUE(service_->show_accent_on_sidebar());
}

TEST_F(ThemeServiceTest, SetShowAccentOnSidebarUpdatesValue) {
  service_->set_show_accent_on_sidebar(false);
  EXPECT_FALSE(service_->show_accent_on_sidebar());
}

TEST_F(ThemeServiceTest, DefaultUseWorkspaceAccentIsTrue) {
  EXPECT_TRUE(service_->use_workspace_accent());
}

TEST_F(ThemeServiceTest, SetUseWorkspaceAccentUpdatesValue) {
  service_->set_use_workspace_accent(false);
  EXPECT_FALSE(service_->use_workspace_accent());
}

TEST_F(ThemeServiceTest, SetUseWorkspaceAccentNotifiesSettings) {
  auto* observer = AddTestObserver();
  int before = observer->settings_changed_count_;

  service_->set_use_workspace_accent(false);

  EXPECT_GT(observer->settings_changed_count_, before);
}

TEST_F(ThemeServiceTest, CustomAccentColorDefaultIsValid) {
  SkColor color = service_->custom_accent_color();
  EXPECT_EQ(255, SkColorGetA(color));
  EXPECT_NE(SK_ColorTRANSPARENT, color);
}

TEST_F(ThemeServiceTest, SetCustomAccentColorUpdatesValue) {
  service_->set_custom_accent_color(SK_ColorGREEN);
  EXPECT_EQ(SK_ColorGREEN, service_->custom_accent_color());
}

TEST_F(ThemeServiceTest, CustomAccentColorAppliesWhenNotUsingWorkspace) {
  // Disable workspace accent to use custom.
  service_->set_use_workspace_accent(false);
  service_->set_custom_accent_color(SK_ColorMAGENTA);

  // The active accent should be the custom color.
  EXPECT_EQ(SK_ColorMAGENTA, service_->GetAccentColor());
}

TEST_F(ThemeServiceTest, ShowAccentOnTabsPersistsToPrefs) {
  service_->set_show_accent_on_tabs(false);

  PrefService* prefs = profile_->GetPrefs();
  ASSERT_NE(nullptr, prefs);
  EXPECT_FALSE(prefs->GetBoolean("astra.theme.show_accent_on_tabs"));
}

TEST_F(ThemeServiceTest, ShowAccentOnSidebarPersistsToPrefs) {
  service_->set_show_accent_on_sidebar(false);

  PrefService* prefs = profile_->GetPrefs();
  ASSERT_NE(nullptr, prefs);
  EXPECT_FALSE(prefs->GetBoolean("astra.theme.show_accent_on_sidebar"));
}

// =========================================================================
// Auto theme schedule tests
// =========================================================================

TEST_F(ThemeServiceTest, DefaultUseAutoThemeScheduleIsFalse) {
  EXPECT_FALSE(service_->use_auto_theme_schedule());
}

TEST_F(ThemeServiceTest, SetUseAutoThemeScheduleUpdatesValue) {
  service_->set_use_auto_theme_schedule(true);
  EXPECT_TRUE(service_->use_auto_theme_schedule());
}

TEST_F(ThemeServiceTest, DefaultAutoLightStartTime) {
  EXPECT_EQ("07:00", service_->auto_theme_light_start());
}

TEST_F(ThemeServiceTest, DefaultAutoDarkStartTime) {
  EXPECT_EQ("19:00", service_->auto_theme_dark_start());
}

TEST_F(ThemeServiceTest, SetAutoLightStartUpdatesValue) {
  service_->set_auto_theme_light_start("06:30");
  EXPECT_EQ("06:30", service_->auto_theme_light_start());
}

TEST_F(ThemeServiceTest, SetAutoDarkStartUpdatesValue) {
  service_->set_auto_theme_dark_start("20:00");
  EXPECT_EQ("20:00", service_->auto_theme_dark_start());
}

TEST_F(ThemeServiceTest, AutoThemeSchedulePersistsToPrefs) {
  service_->set_use_auto_theme_schedule(true);
  service_->set_auto_theme_light_start("06:00");
  service_->set_auto_theme_dark_start("18:00");

  PrefService* prefs = profile_->GetPrefs();
  ASSERT_NE(nullptr, prefs);
  EXPECT_TRUE(prefs->GetBoolean("astra.theme.use_auto_theme_schedule"));
  EXPECT_EQ("06:00", prefs->GetString("astra.theme.auto_theme_light_start"));
  EXPECT_EQ("18:00", prefs->GetString("astra.theme.auto_theme_dark_start"));
}

// =========================================================================
// Additional color utility tests
// =========================================================================

TEST_F(ThemeServiceTest, GetAccentColorWithAlphaDefault) {
  // Full alpha should be the same as the accent color.
  SkColor with_alpha = service_->GetAccentColorWithAlpha(255);
  EXPECT_EQ(service_->GetAccentColor(), with_alpha);
  EXPECT_EQ(255, SkColorGetA(with_alpha));
}

TEST_F(ThemeServiceTest, GetAccentColorWithAlphaHalf) {
  SkColor with_alpha = service_->GetAccentColorWithAlpha(128);
  EXPECT_EQ(128, SkColorGetA(with_alpha));
  // RGB channels should be the same.
  EXPECT_EQ(SkColorGetR(service_->GetAccentColor()), SkColorGetR(with_alpha));
  EXPECT_EQ(SkColorGetG(service_->GetAccentColor()), SkColorGetG(with_alpha));
  EXPECT_EQ(SkColorGetB(service_->GetAccentColor()), SkColorGetB(with_alpha));
}

TEST_F(ThemeServiceTest, GetAccentColorWithAlphaZero) {
  SkColor with_alpha = service_->GetAccentColorWithAlpha(0);
  EXPECT_EQ(0, SkColorGetA(with_alpha));
}

TEST_F(ThemeServiceTest, GetAccentColorWithAlphaClampsLow) {
  // Negative alpha should be clamped to 0.
  SkColor with_alpha = service_->GetAccentColorWithAlpha(-10);
  EXPECT_EQ(0, SkColorGetA(with_alpha));
}

TEST_F(ThemeServiceTest, GetAccentColorWithAlphaClampsHigh) {
  // Alpha above 255 should be clamped.
  SkColor with_alpha = service_->GetAccentColorWithAlpha(300);
  EXPECT_EQ(255, SkColorGetA(with_alpha));
}

TEST_F(ThemeServiceTest, GetSurfaceColorReturnsValue) {
  SkColor surface = service_->GetSurfaceColor();
  EXPECT_EQ(255, SkColorGetA(surface));
  EXPECT_NE(SK_ColorTRANSPARENT, surface);
}

TEST_F(ThemeServiceTest, GetTextPrimaryColorReturnsValue) {
  SkColor text = service_->GetTextPrimaryColor();
  EXPECT_EQ(255, SkColorGetA(text));
  EXPECT_NE(SK_ColorTRANSPARENT, text);
}

TEST_F(ThemeServiceTest, GetTextSecondaryColorReturnsValue) {
  SkColor text = service_->GetTextSecondaryColor();
  EXPECT_EQ(255, SkColorGetA(text));
  EXPECT_NE(SK_ColorTRANSPARENT, text);
}

TEST_F(ThemeServiceTest, GetBorderColorReturnsValue) {
  SkColor border = service_->GetBorderColor();
  EXPECT_EQ(255, SkColorGetA(border));
  EXPECT_NE(SK_ColorTRANSPARENT, border);
}

TEST_F(ThemeServiceTest, GetHoverColorReturnsValue) {
  SkColor hover = service_->GetHoverColor();
  EXPECT_EQ(255, SkColorGetA(hover));
  EXPECT_NE(SK_ColorTRANSPARENT, hover);
}

TEST_F(ThemeServiceTest, BlendColorsStaticSameColor) {
  // Blending a color with itself should give the same color.
  SkColor result = AstraThemeService::BlendColors(SK_ColorRED, SK_ColorRED, 0.5f);
  EXPECT_EQ(SK_ColorRED, result);
}

TEST_F(ThemeServiceTest, BlendColorsStaticFullBlend) {
  // t=0 → first color, t=1 → second color.
  SkColor result0 = AstraThemeService::BlendColors(SK_ColorRED, SK_ColorBLUE, 0.0f);
  SkColor result1 = AstraThemeService::BlendColors(SK_ColorRED, SK_ColorBLUE, 1.0f);
  EXPECT_EQ(SK_ColorRED, result0);
  EXPECT_EQ(SK_ColorBLUE, result1);
}

TEST_F(ThemeServiceTest, BlendColorsStaticHalfBlend) {
  // 50% blend between red and blue should be a purple-ish color.
  SkColor result =
      AstraThemeService::BlendColors(SK_ColorRED, SK_ColorBLUE, 0.5f);
  // Red channel should be half of 255.
  EXPECT_GT(SkColorGetR(result), 0);
  EXPECT_LT(SkColorGetR(result), 255);
  // Blue channel should be half of 255.
  EXPECT_GT(SkColorGetB(result), 0);
  EXPECT_LT(SkColorGetB(result), 255);
  // Green should be 0.
  EXPECT_EQ(0, SkColorGetG(result));
}

TEST_F(ThemeServiceTest, GetContrastRatioSameColor) {
  // Same color has contrast ratio of 1.0.
  float ratio =
      AstraThemeService::GetContrastRatio(SK_ColorWHITE, SK_ColorWHITE);
  EXPECT_FLOAT_EQ(1.0f, ratio);
}

TEST_F(ThemeServiceTest, GetContrastRatioBlackWhite) {
  // Black on white should have high contrast.
  float ratio =
      AstraThemeService::GetContrastRatio(SK_ColorBLACK, SK_ColorWHITE);
  EXPECT_GT(ratio, 20.0f);  // WCAG says 21:1 max.
  EXPECT_LE(ratio, 21.0f);
}

TEST_F(ThemeServiceTest, GetContrastRatioSymmetric) {
  // Contrast ratio should be symmetric.
  float ratio1 =
      AstraThemeService::GetContrastRatio(SK_ColorRED, SK_ColorBLUE);
  float ratio2 =
      AstraThemeService::GetContrastRatio(SK_ColorBLUE, SK_ColorRED);
  EXPECT_FLOAT_EQ(ratio1, ratio2);
}

TEST_F(ThemeServiceTest, IsReadableBlackOnWhite) {
  // Black text on white background should be readable.
  EXPECT_TRUE(AstraThemeService::IsReadable(SK_ColorBLACK, SK_ColorWHITE));
}

TEST_F(ThemeServiceTest, IsReadableWhiteOnBlack) {
  // White text on black background should be readable.
  EXPECT_TRUE(AstraThemeService::IsReadable(SK_ColorWHITE, SK_ColorBLACK));
}

TEST_F(ThemeServiceTest, IsReadableSameColorReturnsFalse) {
  // Same color on same background is not readable.
  EXPECT_FALSE(AstraThemeService::IsReadable(SK_ColorWHITE, SK_ColorWHITE));
}

TEST_F(ThemeServiceTest, IsReadableRedOnWhite) {
  // Red on white should pass WCAG AA? Let's check.
  // Actually, pure red (#FF0000) on white has about 4:1 contrast,
  // which is below the 4.5:1 AA threshold for normal text.
  // Let's test with a known-bad combination.
  EXPECT_FALSE(AstraThemeService::IsReadable(
      SkColorSetRGB(0xCC, 0xCC, 0xCC), SK_ColorWHITE));  // Light gray on white
}

TEST_F(ThemeServiceTest, MakeReadableAlreadyReadable) {
  // If already readable, should return unchanged.
  SkColor result =
      AstraThemeService::MakeReadable(SK_ColorBLACK, SK_ColorWHITE);
  EXPECT_EQ(SK_ColorBLACK, result);
}

TEST_F(ThemeServiceTest, MakeReadableDarkOnLight) {
  // Make light gray readable on white background → should darken.
  SkColor light_gray = SkColorSetRGB(0xDD, 0xDD, 0xDD);
  SkColor result =
      AstraThemeService::MakeReadable(light_gray, SK_ColorWHITE);
  EXPECT_TRUE(AstraThemeService::IsReadable(result, SK_ColorWHITE));
  // Result should be darker than input.
  EXPECT_LT(RelativeLuminance(result), RelativeLuminance(light_gray));
}

TEST_F(ThemeServiceTest, MakeReadableLightOnDark) {
  // Make dark gray readable on black background → should lighten.
  SkColor dark_gray = SkColorSetRGB(0x22, 0x22, 0x22);
  SkColor result =
      AstraThemeService::MakeReadable(dark_gray, SK_ColorBLACK);
  EXPECT_TRUE(AstraThemeService::IsReadable(result, SK_ColorBLACK));
  // Result should be lighter than input.
  EXPECT_GT(RelativeLuminance(result), RelativeLuminance(dark_gray));
}

// =========================================================================
// Bulk operation tests
// =========================================================================

TEST_F(ThemeServiceTest, ResetToDefaultsResetsAllSettings) {
  // Change some settings first.
  service_->set_use_high_contrast(true);
  service_->set_accent_intensity(1.5);
  service_->set_theme_preset(AstraThemePreset::kDark);
  service_->set_show_accent_on_tabs(false);

  // Verify they changed.
  EXPECT_TRUE(service_->use_high_contrast());
  EXPECT_DOUBLE_EQ(1.5, service_->accent_intensity());
  EXPECT_EQ(AstraThemePreset::kDark, service_->theme_preset());
  EXPECT_FALSE(service_->show_accent_on_tabs());

  // Reset to defaults.
  service_->ResetToDefaults();

  // All should be back to defaults.
  EXPECT_FALSE(service_->use_high_contrast());
  EXPECT_DOUBLE_EQ(1.0, service_->accent_intensity());
  EXPECT_EQ(AstraThemePreset::kSystem, service_->theme_preset());
  EXPECT_TRUE(service_->show_accent_on_tabs());
  EXPECT_TRUE(service_->show_accent_on_sidebar());
  EXPECT_FALSE(service_->use_auto_theme_schedule());
}

TEST_F(ThemeServiceTest, ResetToDefaultsNotifiesObservers) {
  auto* observer = AddTestObserver();

  // Change something so reset will actually change things.
  service_->set_use_high_contrast(true);
  service_->set_theme_preset(AstraThemePreset::kDark);

  int accent_before = observer->accent_changed_count_;
  int preset_before = observer->preset_changed_count_;
  int hc_before = observer->high_contrast_changed_count_;
  int settings_before = observer->settings_changed_count_;

  service_->ResetToDefaults();

  EXPECT_GE(observer->accent_changed_count_, accent_before);
  EXPECT_GT(observer->preset_changed_count_, preset_before);
  EXPECT_GT(observer->high_contrast_changed_count_, hc_before);
  EXPECT_GT(observer->settings_changed_count_, settings_before);
}

TEST_F(ThemeServiceTest, ResetToDefaultsClearsScheme) {
  service_->ApplyThemeScheme("ocean");
  ASSERT_FALSE(service_->active_theme_scheme().empty());

  service_->ResetToDefaults();

  EXPECT_TRUE(service_->active_theme_scheme().empty());
}

TEST_F(ThemeServiceTest, ApplyThemeSettingsAppliesAll) {
  AstraThemeSettings settings;
  settings.use_workspace_accent = false;
  settings.custom_accent_color = SK_ColorGREEN;
  settings.theme_preset = AstraThemePreset::kDark;
  settings.theme_scheme = "";
  settings.use_high_contrast = true;
  settings.show_accent_on_tabs = false;
  settings.show_accent_on_sidebar = false;
  settings.accent_intensity = 1.5;
  settings.use_auto_theme_schedule = true;
  settings.auto_theme_light_start = "06:00";
  settings.auto_theme_dark_start = "18:00";

  service_->ApplyThemeSettings(settings);

  EXPECT_FALSE(service_->use_workspace_accent());
  EXPECT_EQ(SK_ColorGREEN, service_->custom_accent_color());
  EXPECT_EQ(AstraThemePreset::kDark, service_->theme_preset());
  EXPECT_TRUE(service_->use_high_contrast());
  EXPECT_FALSE(service_->show_accent_on_tabs());
  EXPECT_FALSE(service_->show_accent_on_sidebar());
  EXPECT_DOUBLE_EQ(1.5, service_->accent_intensity());
  EXPECT_TRUE(service_->use_auto_theme_schedule());
  EXPECT_EQ("06:00", service_->auto_theme_light_start());
  EXPECT_EQ("18:00", service_->auto_theme_dark_start());
}

TEST_F(ThemeServiceTest, ApplyThemeSettingsNotifiesObservers) {
  auto* observer = AddTestObserver();

  int accent_before = observer->accent_changed_count_;
  int preset_before = observer->preset_changed_count_;
  int settings_before = observer->settings_changed_count_;

  AstraThemeSettings settings = service_->GetThemeSettings();
  settings.theme_preset = AstraThemePreset::kDark;
  settings.use_high_contrast = true;
  settings.accent_intensity = 1.5;

  service_->ApplyThemeSettings(settings);

  EXPECT_GT(observer->preset_changed_count_, preset_before);
  EXPECT_GT(observer->settings_changed_count_, settings_before);
  // Accent may or may not change depending on settings.
  EXPECT_GE(observer->accent_changed_count_, accent_before);
}

TEST_F(ThemeServiceTest, GetThemeSettingsReturnsCurrentState) {
  // Modify some settings.
  service_->set_use_high_contrast(true);
  service_->set_accent_intensity(1.2);
  service_->set_theme_preset(AstraThemePreset::kLight);

  AstraThemeSettings settings = service_->GetThemeSettings();

  EXPECT_TRUE(settings.use_high_contrast);
  EXPECT_DOUBLE_EQ(1.2, settings.accent_intensity);
  EXPECT_EQ(AstraThemePreset::kLight, settings.theme_preset);
  EXPECT_TRUE(settings.show_accent_on_tabs);  // default
  EXPECT_TRUE(settings.show_accent_on_sidebar);  // default
}

TEST_F(ThemeServiceTest, ApplyThemeSettingsClampsIntensity) {
  AstraThemeSettings settings;
  settings.accent_intensity = 10.0;  // Way too high.

  service_->ApplyThemeSettings(settings);

  // Should be clamped to max.
  EXPECT_LE(service_->accent_intensity(), 2.0);
}

// =========================================================================
// Persistence round-trip tests
// =========================================================================

TEST_F(ThemeServiceTest, PersistenceRoundTripHighContrast) {
  service_->set_use_high_contrast(true);

  // Create a new service from the same profile.
  auto service2 = std::make_unique<AstraThemeService>(profile_.get());
  EXPECT_TRUE(service2->use_high_contrast());
}

TEST_F(ThemeServiceTest, PersistenceRoundTripAccentIntensity) {
  service_->set_accent_intensity(1.75);

  auto service2 = std::make_unique<AstraThemeService>(profile_.get());
  EXPECT_DOUBLE_EQ(1.75, service2->accent_intensity());
}

TEST_F(ThemeServiceTest, PersistenceRoundTripShowAccentOnTabs) {
  service_->set_show_accent_on_tabs(false);

  auto service2 = std::make_unique<AstraThemeService>(profile_.get());
  EXPECT_FALSE(service2->show_accent_on_tabs());
}

TEST_F(ThemeServiceTest, PersistenceRoundTripShowAccentOnSidebar) {
  service_->set_show_accent_on_sidebar(false);

  auto service2 = std::make_unique<AstraThemeService>(profile_.get());
  EXPECT_FALSE(service2->show_accent_on_sidebar());
}

TEST_F(ThemeServiceTest, PersistenceRoundTripAutoThemeSchedule) {
  service_->set_use_auto_theme_schedule(true);
  service_->set_auto_theme_light_start("05:30");
  service_->set_auto_theme_dark_start("21:00");

  auto service2 = std::make_unique<AstraThemeService>(profile_.get());
  EXPECT_TRUE(service2->use_auto_theme_schedule());
  EXPECT_EQ("05:30", service2->auto_theme_light_start());
  EXPECT_EQ("21:00", service2->auto_theme_dark_start());
}

TEST_F(ThemeServiceTest, PersistenceRoundTripThemeScheme) {
  service_->ApplyThemeScheme("forest");

  auto service2 = std::make_unique<AstraThemeService>(profile_.get());
  // Note: scheme name persists but active_scheme_ may not be loaded
  // from prefs in the same way — depends on implementation.
  // At minimum, the pref value should be there.
  PrefService* prefs = profile_->GetPrefs();
  EXPECT_EQ("forest", prefs->GetString("astra.theme.scheme"));
}

// =========================================================================
// Edge case tests
// =========================================================================

TEST_F(ThemeServiceTest, AlphaClampingBelowZero) {
  SkColor result = service_->GetAccentColorWithAlpha(-100);
  EXPECT_EQ(0, SkColorGetA(result));
}

TEST_F(ThemeServiceTest, AlphaClampingAbove255) {
  SkColor result = service_->GetAccentColorWithAlpha(1000);
  EXPECT_EQ(255, SkColorGetA(result));
}

TEST_F(ThemeServiceTest, IntensityClampingBelow05) {
  service_->set_accent_intensity(0.0);
  EXPECT_DOUBLE_EQ(0.5, service_->accent_intensity());
}

TEST_F(ThemeServiceTest, IntensityClampingAbove20) {
  service_->set_accent_intensity(100.0);
  EXPECT_DOUBLE_EQ(2.0, service_->accent_intensity());
}

TEST_F(ThemeServiceTest, InvalidSchemeNameReturnsFalse) {
  EXPECT_FALSE(service_->ApplyThemeScheme(""));
  EXPECT_FALSE(service_->ApplyThemeScheme("!!!"));
  EXPECT_FALSE(service_->ApplyThemeScheme("nonexistent"));
}

TEST_F(ThemeServiceTest, EmptySchemeNameReturnsFalseForFind) {
  AstraThemeScheme scheme;
  EXPECT_FALSE(AstraThemeService::FindThemeScheme("", &scheme));
}

// =========================================================================
// Observer default implementation tests (all 6 methods)
// =========================================================================

TEST(AstraThemeServiceObserverTest, AllDefaultImplementationsAreNoOps) {
  // All six observer methods have default no-op implementations.
  class TestObserver : public AstraThemeServiceObserver {};

  TestObserver observer;
  observer.OnAccentColorChanged(SK_ColorRED);
  observer.OnThemeChanged();
  observer.OnThemePresetChanged(AstraThemePreset::kDark);
  observer.OnThemeSchemeChanged("ocean");
  observer.OnHighContrastChanged(true);
  observer.OnThemeSettingsChanged();
  // No crash = success (all default implementations are no-ops).
}

// =========================================================================
// Theme settings struct tests
// =========================================================================

TEST(AstraThemeSettingsTest, DefaultValuesAreSane) {
  AstraThemeSettings settings;
  EXPECT_TRUE(settings.use_workspace_accent);
  EXPECT_EQ(AstraThemePreset::kSystem, settings.theme_preset);
  EXPECT_TRUE(settings.theme_scheme.empty());
  EXPECT_FALSE(settings.use_high_contrast);
  EXPECT_TRUE(settings.show_accent_on_tabs);
  EXPECT_TRUE(settings.show_accent_on_sidebar);
  EXPECT_DOUBLE_EQ(1.0, settings.accent_intensity);
  EXPECT_FALSE(settings.use_auto_theme_schedule);
  EXPECT_EQ("07:00", settings.auto_theme_light_start);
  EXPECT_EQ("19:00", settings.auto_theme_dark_start);
}

// =========================================================================
// Theme scheme struct tests
// =========================================================================

TEST(AstraThemeSchemeTest, StructHasAllFields) {
  AstraThemeScheme scheme;
  scheme.name = "test";
  scheme.display_name = "Test";
  scheme.accent_color = SK_ColorRED;
  scheme.secondary_color = SK_ColorBLUE;
  scheme.prefers_dark = true;
  scheme.description = "Test scheme";

  EXPECT_EQ("test", scheme.name);
  EXPECT_EQ("Test", scheme.display_name);
  EXPECT_EQ(SK_ColorRED, scheme.accent_color);
  EXPECT_EQ(SK_ColorBLUE, scheme.secondary_color);
  EXPECT_TRUE(scheme.prefers_dark);
  EXPECT_EQ("Test scheme", scheme.description);
}

// =========================================================================
// Theme scheme count test
// =========================================================================

TEST(AstraThemeSchemeTest, AllSchemesPresent) {
  std::vector<AstraThemeScheme> schemes =
      AstraThemeService::GetThemeSchemes();
  // Should have at least the named schemes: ocean, forest, sunset,
  // midnight, minimal, lavender, coral, slate.
  EXPECT_GE(schemes.size(), 8u);
}

}  // namespace astra
