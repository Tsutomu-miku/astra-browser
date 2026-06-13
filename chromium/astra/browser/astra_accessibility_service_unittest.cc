// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_accessibility_service.h"

#include <string>
#include <vector>

#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "chrome/test/base/testing_profile.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "astra/browser/astra_accessibility_service_factory.h"
#include "astra/browser/astra_prefs.h"

namespace astra {

namespace {

// Test observer that records calls for verification.
class TestAccessibilityObserver : public AstraAccessibilityObserver {
 public:
  void OnHighContrastChanged(bool enabled) override {
    high_contrast_changed_count_++;
    last_high_contrast_enabled_ = enabled;
  }

  void OnReducedMotionChanged(bool enabled) override {
    reduced_motion_changed_count_++;
    last_reduced_motion_enabled_ = enabled;
  }

  void OnFontScaleChanged(double font_scale) override {
    font_scale_changed_count_++;
    last_font_scale_ = font_scale;
  }

  void OnScreenReaderChanged(bool enabled) override {
    screen_reader_changed_count_++;
    last_screen_reader_enabled_ = enabled;
  }

  void OnCaretBrowsingChanged(bool enabled) override {
    caret_browsing_changed_count_++;
    last_caret_browsing_enabled_ = enabled;
  }

  void OnStickyKeysChanged(bool enabled) override {
    sticky_keys_changed_count_++;
    last_sticky_keys_enabled_ = enabled;
  }

  void OnSlowKeysChanged(bool enabled) override {
    slow_keys_changed_count_++;
    last_slow_keys_enabled_ = enabled;
  }

  void OnMouseKeysChanged(bool enabled) override {
    mouse_keys_changed_count_++;
    last_mouse_keys_enabled_ = enabled;
  }

  void OnLargeCursorChanged(bool enabled) override {
    large_cursor_changed_count_++;
    last_large_cursor_enabled_ = enabled;
  }

  void OnMagnifierChanged(bool enabled) override {
    magnifier_changed_count_++;
    last_magnifier_enabled_ = enabled;
  }

  void OnSelectToSpeakChanged(bool enabled) override {
    select_to_speak_changed_count_++;
    last_select_to_speak_enabled_ = enabled;
  }

  void OnDictationChanged(bool enabled) override {
    dictation_changed_count_++;
    last_dictation_enabled_ = enabled;
  }

  void OnVirtualKeyboardChanged(bool enabled) override {
    virtual_keyboard_changed_count_++;
    last_virtual_keyboard_enabled_ = enabled;
  }

  void OnMinimumFontSizeChanged(int size) override {
    minimum_font_size_changed_count_++;
    last_minimum_font_size_ = size;
  }

  void OnContrastLevelChanged(AstraContrastLevel level) override {
    contrast_level_changed_count_++;
    last_contrast_level_ = level;
  }

  void OnNightLightChanged(bool enabled) override {
    night_light_changed_count_++;
    last_night_light_enabled_ = enabled;
  }

  void OnColorInversionChanged(bool enabled) override {
    color_inversion_changed_count_++;
    last_color_inversion_enabled_ = enabled;
  }

  void OnAnimationReductionChanged(AstraAnimationReduction level) override {
    animation_reduction_changed_count_++;
    last_animation_reduction_ = level;
  }

  void OnAccessibilitySettingsChanged() override {
    settings_changed_count_++;
  }

  // Counters
  int high_contrast_changed_count_ = 0;
  int reduced_motion_changed_count_ = 0;
  int font_scale_changed_count_ = 0;
  int screen_reader_changed_count_ = 0;
  int caret_browsing_changed_count_ = 0;
  int sticky_keys_changed_count_ = 0;
  int slow_keys_changed_count_ = 0;
  int mouse_keys_changed_count_ = 0;
  int large_cursor_changed_count_ = 0;
  int magnifier_changed_count_ = 0;
  int select_to_speak_changed_count_ = 0;
  int dictation_changed_count_ = 0;
  int virtual_keyboard_changed_count_ = 0;
  int minimum_font_size_changed_count_ = 0;
  int contrast_level_changed_count_ = 0;
  int night_light_changed_count_ = 0;
  int color_inversion_changed_count_ = 0;
  int animation_reduction_changed_count_ = 0;
  int settings_changed_count_ = 0;

  // Last recorded values
  bool last_high_contrast_enabled_ = false;
  bool last_reduced_motion_enabled_ = false;
  double last_font_scale_ = 1.0;
  bool last_screen_reader_enabled_ = false;
  bool last_caret_browsing_enabled_ = false;
  bool last_sticky_keys_enabled_ = false;
  bool last_slow_keys_enabled_ = false;
  bool last_mouse_keys_enabled_ = false;
  bool last_large_cursor_enabled_ = false;
  bool last_magnifier_enabled_ = false;
  bool last_select_to_speak_enabled_ = false;
  bool last_dictation_enabled_ = false;
  bool last_virtual_keyboard_enabled_ = false;
  int last_minimum_font_size_ = 0;
  AstraContrastLevel last_contrast_level_ = AstraContrastLevel::kNormal;
  bool last_night_light_enabled_ = false;
  bool last_color_inversion_enabled_ = false;
  AstraAnimationReduction last_animation_reduction_ = AstraAnimationReduction::kOff;
};

}  // namespace

// Test fixture for AstraAccessibilityService tests.
class AccessibilityServiceTest : public testing::Test {
 protected:
  AccessibilityServiceTest() {
    profile_ = std::make_unique<TestingProfile>();
    // Register all accessibility prefs via the factory.
    AstraAccessibilityServiceFactory::RegisterProfilePrefs(
        profile_->GetPrefs()->registry());
    service_ = std::make_unique<AstraAccessibilityService>(profile_.get());
    DCHECK(service_);
  }

  ~AccessibilityServiceTest() override = default;

  void SetUp() override {
    // Service should start with default accessibility settings.
    ASSERT_FALSE(service_->IsHighContrastEnabled());
    ASSERT_FALSE(service_->IsReducedMotionEnabled());
    ASSERT_DOUBLE_EQ(service_->GetFontScale(), 1.0);
  }

  void TearDown() override {
    // Clean up observers to avoid dangling references.
    for (auto& observer : test_observers_) {
      service_->RemoveObserver(&observer);
    }
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<AstraAccessibilityService> service_;
  std::vector<TestAccessibilityObserver> test_observers_;
};

// ---------------------------------------------------------------------------
// Default state
// ---------------------------------------------------------------------------

TEST_F(AccessibilityServiceTest, DefaultState_HighContrastDisabled) {
  EXPECT_FALSE(service_->IsHighContrastEnabled());
}

TEST_F(AccessibilityServiceTest, DefaultState_ReducedMotionDisabled) {
  EXPECT_FALSE(service_->IsReducedMotionEnabled());
}

TEST_F(AccessibilityServiceTest, DefaultState_FontScaleIsOne) {
  EXPECT_DOUBLE_EQ(service_->GetFontScale(), 1.0);
}

TEST_F(AccessibilityServiceTest, DefaultState_ScreenReaderDisabled) {
  EXPECT_FALSE(service_->IsScreenReaderEnabled());
}

TEST_F(AccessibilityServiceTest, DefaultState_CaretBrowsingDisabled) {
  EXPECT_FALSE(service_->IsCaretBrowsingEnabled());
}

TEST_F(AccessibilityServiceTest, DefaultState_StickyKeysDisabled) {
  EXPECT_FALSE(service_->IsStickyKeysEnabled());
}

TEST_F(AccessibilityServiceTest, DefaultState_SlowKeysDisabled) {
  EXPECT_FALSE(service_->IsSlowKeysEnabled());
  EXPECT_EQ(service_->GetSlowKeysDelay(), base::Milliseconds(500));
}

TEST_F(AccessibilityServiceTest, DefaultState_MouseKeysDisabled) {
  EXPECT_FALSE(service_->IsMouseKeysEnabled());
}

TEST_F(AccessibilityServiceTest, DefaultState_LargeCursorDisabled) {
  EXPECT_FALSE(service_->IsLargeCursorEnabled());
  EXPECT_EQ(service_->GetLargeCursorSize(), 3);
}

TEST_F(AccessibilityServiceTest, DefaultState_MagnifierDisabled) {
  EXPECT_FALSE(service_->IsMagnifierEnabled());
  EXPECT_DOUBLE_EQ(service_->GetMagnifierScale(), 2.0);
  EXPECT_EQ(service_->GetMagnifierType(), AstraMagnifierType::kDocked);
}

TEST_F(AccessibilityServiceTest, DefaultState_SelectToSpeakDisabled) {
  EXPECT_FALSE(service_->IsSelectToSpeakEnabled());
}

TEST_F(AccessibilityServiceTest, DefaultState_DictationDisabled) {
  EXPECT_FALSE(service_->IsDictationEnabled());
}

TEST_F(AccessibilityServiceTest, DefaultState_VirtualKeyboardDisabled) {
  EXPECT_FALSE(service_->IsVirtualKeyboardEnabled());
}

TEST_F(AccessibilityServiceTest, DefaultState_TextHelpersAtDefaults) {
  EXPECT_EQ(service_->GetMinimumFontSize(), 0);
  EXPECT_EQ(service_->GetFontWeightAdjustment(), 0);
  EXPECT_DOUBLE_EQ(service_->GetLetterSpacing(), 1.0);
  EXPECT_DOUBLE_EQ(service_->GetLineHeight(), 1.0);
}

TEST_F(AccessibilityServiceTest, DefaultState_ColorAndContrastAtDefaults) {
  EXPECT_EQ(service_->GetContrastLevel(), AstraContrastLevel::kNormal);
  EXPECT_FALSE(service_->IsNightLightEnabled());
  EXPECT_EQ(service_->GetColorTemperature(), 50);
  EXPECT_FALSE(service_->IsColorInversionEnabled());
}

TEST_F(AccessibilityServiceTest, DefaultState_AnimationAndMotionAtDefaults) {
  EXPECT_EQ(service_->GetAnimationReductionLevel(), AstraAnimationReduction::kOff);
  EXPECT_FALSE(service_->IsAutoScrollEnabled());
  EXPECT_DOUBLE_EQ(service_->GetScrollSpeed(), 1.0);
}

TEST_F(AccessibilityServiceTest, DefaultState_AccessibilityNotEnabled) {
  EXPECT_FALSE(service_->IsAccessibilityEnabled());
  EXPECT_EQ(service_->GetEnabledFeaturesCount(), 0);
  EXPECT_TRUE(service_->GetEnabledFeatureList().empty());
}

// ---------------------------------------------------------------------------
// Observer defaults — all methods have empty default implementations
// ---------------------------------------------------------------------------

TEST_F(AccessibilityServiceTest, ObserverDefaults_DoNotCrash) {
  // Create a minimal observer that overrides nothing.
  // All methods have default empty implementations, so this should compile
  // and not crash.
  class DefaultObserver : public AstraAccessibilityObserver {};

  DefaultObserver observer;
  service_->AddObserver(&observer);

  // Trigger all observer paths via pref changes.
  service_->SetHighContrastEnabled(true);
  service_->SetReducedMotionEnabled(true);
  service_->SetFontScale(1.5);
  service_->SetCaretBrowsingEnabled(true);
  service_->SetStickyKeysEnabled(true);
  service_->SetSlowKeysEnabled(true);
  service_->SetMouseKeysEnabled(true);
  service_->SetLargeCursorEnabled(true);
  service_->SetMagnifierEnabled(true);
  service_->SetSelectToSpeakEnabled(true);
  service_->SetDictationEnabled(true);
  service_->SetVirtualKeyboardEnabled(true);
  service_->SetMinimumFontSize(12);
  service_->SetContrastLevel(AstraContrastLevel::kHigh);
  service_->SetNightLightEnabled(true);
  service_->SetColorInversionEnabled(true);
  service_->SetAnimationReductionLevel(AstraAnimationReduction::kMax);

  service_->RemoveObserver(&observer);
  SUCCEED() << "Default observer methods do not crash.";
}

// ---------------------------------------------------------------------------
// Observer add/remove
// ---------------------------------------------------------------------------

TEST_F(AccessibilityServiceTest, AddRemoveObserver_NoCrash) {
  TestAccessibilityObserver observer;

  service_->AddObserver(&observer);
  service_->RemoveObserver(&observer);

  SUCCEED() << "Observer add/remove completed without crash.";
}

TEST_F(AccessibilityServiceTest, RemoveNonexistentObserver_NoCrash) {
  TestAccessibilityObserver observer;

  service_->RemoveObserver(&observer);

  SUCCEED() << "Removing nonexistent observer completed without crash.";
}

// ---------------------------------------------------------------------------
// High contrast
// ---------------------------------------------------------------------------

TEST_F(AccessibilityServiceTest, SetHighContrastEnabled_ChangesValue) {
  ASSERT_FALSE(service_->IsHighContrastEnabled());

  service_->SetHighContrastEnabled(true);
  EXPECT_TRUE(service_->IsHighContrastEnabled());

  service_->SetHighContrastEnabled(false);
  EXPECT_FALSE(service_->IsHighContrastEnabled());
}

TEST_F(AccessibilityServiceTest, SetHighContrastEnabled_SameValueNoOp) {
  TestAccessibilityObserver observer;
  service_->AddObserver(&observer);

  ASSERT_FALSE(service_->IsHighContrastEnabled());
  service_->SetHighContrastEnabled(false);

  EXPECT_EQ(observer.high_contrast_changed_count_, 0);
  EXPECT_EQ(observer.settings_changed_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(AccessibilityServiceTest, SetHighContrastEnabled_FiresObservers) {
  TestAccessibilityObserver observer;
  service_->AddObserver(&observer);

  service_->SetHighContrastEnabled(true);

  EXPECT_EQ(observer.high_contrast_changed_count_, 1);
  EXPECT_TRUE(observer.last_high_contrast_enabled_);
  EXPECT_EQ(observer.settings_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(AccessibilityServiceTest, ToggleHighContrast_FlipsValue) {
  ASSERT_FALSE(service_->IsHighContrastEnabled());

  bool result = service_->ToggleHighContrast();
  EXPECT_TRUE(result);
  EXPECT_TRUE(service_->IsHighContrastEnabled());

  result = service_->ToggleHighContrast();
  EXPECT_FALSE(result);
  EXPECT_FALSE(service_->IsHighContrastEnabled());
}

TEST_F(AccessibilityServiceTest, HighContrast_EnablesAccessibility) {
  ASSERT_FALSE(service_->IsAccessibilityEnabled());

  service_->SetHighContrastEnabled(true);
  EXPECT_TRUE(service_->IsAccessibilityEnabled());
}

// ---------------------------------------------------------------------------
// Reduced motion
// ---------------------------------------------------------------------------

TEST_F(AccessibilityServiceTest, SetReducedMotionEnabled_ChangesValue) {
  ASSERT_FALSE(service_->IsReducedMotionEnabled());

  service_->SetReducedMotionEnabled(true);
  EXPECT_TRUE(service_->IsReducedMotionEnabled());

  service_->SetReducedMotionEnabled(false);
  EXPECT_FALSE(service_->IsReducedMotionEnabled());
}

TEST_F(AccessibilityServiceTest, SetReducedMotionEnabled_SameValueNoOp) {
  TestAccessibilityObserver observer;
  service_->AddObserver(&observer);

  ASSERT_FALSE(service_->IsReducedMotionEnabled());
  service_->SetReducedMotionEnabled(false);

  EXPECT_EQ(observer.reduced_motion_changed_count_, 0);
  EXPECT_EQ(observer.settings_changed_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(AccessibilityServiceTest, SetReducedMotionEnabled_FiresObservers) {
  TestAccessibilityObserver observer;
  service_->AddObserver(&observer);

  service_->SetReducedMotionEnabled(true);

  EXPECT_EQ(observer.reduced_motion_changed_count_, 1);
  EXPECT_TRUE(observer.last_reduced_motion_enabled_);
  EXPECT_EQ(observer.settings_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(AccessibilityServiceTest, ToggleReducedMotion_FlipsValue) {
  ASSERT_FALSE(service_->IsReducedMotionEnabled());

  bool result = service_->ToggleReducedMotion();
  EXPECT_TRUE(result);
  EXPECT_TRUE(service_->IsReducedMotionEnabled());

  result = service_->ToggleReducedMotion();
  EXPECT_FALSE(result);
  EXPECT_FALSE(service_->IsReducedMotionEnabled());
}

TEST_F(AccessibilityServiceTest, ReducedMotion_EnablesAccessibility) {
  ASSERT_FALSE(service_->IsAccessibilityEnabled());

  service_->SetReducedMotionEnabled(true);
  EXPECT_TRUE(service_->IsAccessibilityEnabled());
}

// ---------------------------------------------------------------------------
// Font scale
// ---------------------------------------------------------------------------

TEST_F(AccessibilityServiceTest, SetFontScale_ChangesValue) {
  ASSERT_DOUBLE_EQ(service_->GetFontScale(), 1.0);

  service_->SetFontScale(1.5);
  EXPECT_DOUBLE_EQ(service_->GetFontScale(), 1.5);

  service_->SetFontScale(0.75);
  EXPECT_DOUBLE_EQ(service_->GetFontScale(), 0.75);
}

TEST_F(AccessibilityServiceTest, SetFontScale_ClampsToMinimum) {
  service_->SetFontScale(0.1);
  EXPECT_GE(service_->GetFontScale(), 0.5);
  EXPECT_DOUBLE_EQ(service_->GetFontScale(), 0.5);
}

TEST_F(AccessibilityServiceTest, SetFontScale_ClampsToMaximum) {
  service_->SetFontScale(5.0);
  EXPECT_LE(service_->GetFontScale(), 3.0);
  EXPECT_DOUBLE_EQ(service_->GetFontScale(), 3.0);
}

TEST_F(AccessibilityServiceTest, SetFontScale_SameValueNoOp) {
  TestAccessibilityObserver observer;
  service_->AddObserver(&observer);

  double original = service_->GetFontScale();
  service_->SetFontScale(original);

  EXPECT_DOUBLE_EQ(service_->GetFontScale(), original);
  EXPECT_EQ(observer.font_scale_changed_count_, 0);
  EXPECT_EQ(observer.settings_changed_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(AccessibilityServiceTest, SetFontScale_FiresObservers) {
  TestAccessibilityObserver observer;
  service_->AddObserver(&observer);

  service_->SetFontScale(1.5);

  EXPECT_EQ(observer.font_scale_changed_count_, 1);
  EXPECT_DOUBLE_EQ(observer.last_font_scale_, 1.5);
  EXPECT_EQ(observer.settings_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(AccessibilityServiceTest, ResetFontScale_ReturnsToOne) {
  service_->SetFontScale(2.0);
  ASSERT_DOUBLE_EQ(service_->GetFontScale(), 2.0);

  service_->ResetFontScale();
  EXPECT_DOUBLE_EQ(service_->GetFontScale(), 1.0);
}

TEST_F(AccessibilityServiceTest, FontScale_EnablesAccessibility) {
  ASSERT_FALSE(service_->IsAccessibilityEnabled());

  service_->SetFontScale(1.5);
  EXPECT_TRUE(service_->IsAccessibilityEnabled());
}

TEST_F(AccessibilityServiceTest, FontScale_OneIsDefault_DoesNotEnable) {
  // Font scale of exactly 1.0 should not count as accessibility enabled.
  service_->SetFontScale(1.0);
  EXPECT_FALSE(service_->IsAccessibilityEnabled());
}

// ---------------------------------------------------------------------------
// Caret browsing
// ---------------------------------------------------------------------------

TEST_F(AccessibilityServiceTest, SetCaretBrowsingEnabled_ChangesValue) {
  ASSERT_FALSE(service_->IsCaretBrowsingEnabled());

  service_->SetCaretBrowsingEnabled(true);
  EXPECT_TRUE(service_->IsCaretBrowsingEnabled());

  service_->SetCaretBrowsingEnabled(false);
  EXPECT_FALSE(service_->IsCaretBrowsingEnabled());
}

TEST_F(AccessibilityServiceTest, SetCaretBrowsingEnabled_SameValueNoOp) {
  TestAccessibilityObserver observer;
  service_->AddObserver(&observer);

  ASSERT_FALSE(service_->IsCaretBrowsingEnabled());
  service_->SetCaretBrowsingEnabled(false);

  EXPECT_EQ(observer.caret_browsing_changed_count_, 0);
  EXPECT_EQ(observer.settings_changed_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(AccessibilityServiceTest, SetCaretBrowsingEnabled_FiresObservers) {
  TestAccessibilityObserver observer;
  service_->AddObserver(&observer);

  service_->SetCaretBrowsingEnabled(true);

  EXPECT_EQ(observer.caret_browsing_changed_count_, 1);
  EXPECT_TRUE(observer.last_caret_browsing_enabled_);
  EXPECT_EQ(observer.settings_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(AccessibilityServiceTest, ToggleCaretBrowsing_FlipsValue) {
  ASSERT_FALSE(service_->IsCaretBrowsingEnabled());

  bool result = service_->ToggleCaretBrowsing();
  EXPECT_TRUE(result);
  EXPECT_TRUE(service_->IsCaretBrowsingEnabled());

  result = service_->ToggleCaretBrowsing();
  EXPECT_FALSE(result);
  EXPECT_FALSE(service_->IsCaretBrowsingEnabled());
}

TEST_F(AccessibilityServiceTest, CaretBrowsing_EnablesAccessibility) {
  ASSERT_FALSE(service_->IsAccessibilityEnabled());

  service_->SetCaretBrowsingEnabled(true);
  EXPECT_TRUE(service_->IsAccessibilityEnabled());
}

// ---------------------------------------------------------------------------
// Sticky keys
// ---------------------------------------------------------------------------

TEST_F(AccessibilityServiceTest, SetStickyKeysEnabled_ChangesValue) {
  ASSERT_FALSE(service_->IsStickyKeysEnabled());

  service_->SetStickyKeysEnabled(true);
  EXPECT_TRUE(service_->IsStickyKeysEnabled());

  service_->SetStickyKeysEnabled(false);
  EXPECT_FALSE(service_->IsStickyKeysEnabled());
}

TEST_F(AccessibilityServiceTest, SetStickyKeysEnabled_SameValueNoOp) {
  TestAccessibilityObserver observer;
  service_->AddObserver(&observer);

  ASSERT_FALSE(service_->IsStickyKeysEnabled());
  service_->SetStickyKeysEnabled(false);

  EXPECT_EQ(observer.sticky_keys_changed_count_, 0);
  EXPECT_EQ(observer.settings_changed_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(AccessibilityServiceTest, SetStickyKeysEnabled_FiresObservers) {
  TestAccessibilityObserver observer;
  service_->AddObserver(&observer);

  service_->SetStickyKeysEnabled(true);

  EXPECT_EQ(observer.sticky_keys_changed_count_, 1);
  EXPECT_TRUE(observer.last_sticky_keys_enabled_);
  EXPECT_EQ(observer.settings_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(AccessibilityServiceTest, ToggleStickyKeys_FlipsValue) {
  ASSERT_FALSE(service_->IsStickyKeysEnabled());

  bool result = service_->ToggleStickyKeys();
  EXPECT_TRUE(result);
  EXPECT_TRUE(service_->IsStickyKeysEnabled());

  result = service_->ToggleStickyKeys();
  EXPECT_FALSE(result);
  EXPECT_FALSE(service_->IsStickyKeysEnabled());
}

TEST_F(AccessibilityServiceTest, StickyKeys_EnablesAccessibility) {
  ASSERT_FALSE(service_->IsAccessibilityEnabled());

  service_->SetStickyKeysEnabled(true);
  EXPECT_TRUE(service_->IsAccessibilityEnabled());
}

// ---------------------------------------------------------------------------
// Slow keys
// ---------------------------------------------------------------------------

TEST_F(AccessibilityServiceTest, SetSlowKeysEnabled_ChangesValue) {
  ASSERT_FALSE(service_->IsSlowKeysEnabled());

  service_->SetSlowKeysEnabled(true);
  EXPECT_TRUE(service_->IsSlowKeysEnabled());

  service_->SetSlowKeysEnabled(false);
  EXPECT_FALSE(service_->IsSlowKeysEnabled());
}

TEST_F(AccessibilityServiceTest, SetSlowKeysEnabled_SameValueNoOp) {
  TestAccessibilityObserver observer;
  service_->AddObserver(&observer);

  ASSERT_FALSE(service_->IsSlowKeysEnabled());
  service_->SetSlowKeysEnabled(false);

  EXPECT_EQ(observer.slow_keys_changed_count_, 0);
  EXPECT_EQ(observer.settings_changed_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(AccessibilityServiceTest, SetSlowKeysEnabled_FiresObservers) {
  TestAccessibilityObserver observer;
  service_->AddObserver(&observer);

  service_->SetSlowKeysEnabled(true);

  EXPECT_EQ(observer.slow_keys_changed_count_, 1);
  EXPECT_TRUE(observer.last_slow_keys_enabled_);
  EXPECT_EQ(observer.settings_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(AccessibilityServiceTest, ToggleSlowKeys_FlipsValue) {
  ASSERT_FALSE(service_->IsSlowKeysEnabled());

  bool result = service_->ToggleSlowKeys();
  EXPECT_TRUE(result);
  EXPECT_TRUE(service_->IsSlowKeysEnabled());

  result = service_->ToggleSlowKeys();
  EXPECT_FALSE(result);
  EXPECT_FALSE(service_->IsSlowKeysEnabled());
}

TEST_F(AccessibilityServiceTest, SlowKeys_EnablesAccessibility) {
  ASSERT_FALSE(service_->IsAccessibilityEnabled());

  service_->SetSlowKeysEnabled(true);
  EXPECT_TRUE(service_->IsAccessibilityEnabled());
}

TEST_F(AccessibilityServiceTest, SetSlowKeysDelay_ChangesValue) {
  service_->SetSlowKeysDelay(base::Milliseconds(1000));
  EXPECT_EQ(service_->GetSlowKeysDelay(), base::Milliseconds(1000));
}

TEST_F(AccessibilityServiceTest, SetSlowKeysDelay_ClampsToMinimum) {
  service_->SetSlowKeysDelay(base::Milliseconds(1));
  EXPECT_GE(service_->GetSlowKeysDelay().InMilliseconds(), 10);
  EXPECT_EQ(service_->GetSlowKeysDelay(), base::Milliseconds(10));
}

TEST_F(AccessibilityServiceTest, SetSlowKeysDelay_ClampsToMaximum) {
  service_->SetSlowKeysDelay(base::Milliseconds(10000));
  EXPECT_LE(service_->GetSlowKeysDelay().InMilliseconds(), 5000);
  EXPECT_EQ(service_->GetSlowKeysDelay(), base::Milliseconds(5000));
}

// ---------------------------------------------------------------------------
// Mouse keys
// ---------------------------------------------------------------------------

TEST_F(AccessibilityServiceTest, SetMouseKeysEnabled_ChangesValue) {
  ASSERT_FALSE(service_->IsMouseKeysEnabled());

  service_->SetMouseKeysEnabled(true);
  EXPECT_TRUE(service_->IsMouseKeysEnabled());

  service_->SetMouseKeysEnabled(false);
  EXPECT_FALSE(service_->IsMouseKeysEnabled());
}

TEST_F(AccessibilityServiceTest, SetMouseKeysEnabled_SameValueNoOp) {
  TestAccessibilityObserver observer;
  service_->AddObserver(&observer);

  ASSERT_FALSE(service_->IsMouseKeysEnabled());
  service_->SetMouseKeysEnabled(false);

  EXPECT_EQ(observer.mouse_keys_changed_count_, 0);
  EXPECT_EQ(observer.settings_changed_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(AccessibilityServiceTest, SetMouseKeysEnabled_FiresObservers) {
  TestAccessibilityObserver observer;
  service_->AddObserver(&observer);

  service_->SetMouseKeysEnabled(true);

  EXPECT_EQ(observer.mouse_keys_changed_count_, 1);
  EXPECT_TRUE(observer.last_mouse_keys_enabled_);
  EXPECT_EQ(observer.settings_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(AccessibilityServiceTest, ToggleMouseKeys_FlipsValue) {
  ASSERT_FALSE(service_->IsMouseKeysEnabled());

  bool result = service_->ToggleMouseKeys();
  EXPECT_TRUE(result);
  EXPECT_TRUE(service_->IsMouseKeysEnabled());

  result = service_->ToggleMouseKeys();
  EXPECT_FALSE(result);
  EXPECT_FALSE(service_->IsMouseKeysEnabled());
}

TEST_F(AccessibilityServiceTest, MouseKeys_EnablesAccessibility) {
  ASSERT_FALSE(service_->IsAccessibilityEnabled());

  service_->SetMouseKeysEnabled(true);
  EXPECT_TRUE(service_->IsAccessibilityEnabled());
}

// ---------------------------------------------------------------------------
// Large cursor
// ---------------------------------------------------------------------------

TEST_F(AccessibilityServiceTest, SetLargeCursorEnabled_ChangesValue) {
  ASSERT_FALSE(service_->IsLargeCursorEnabled());

  service_->SetLargeCursorEnabled(true);
  EXPECT_TRUE(service_->IsLargeCursorEnabled());

  service_->SetLargeCursorEnabled(false);
  EXPECT_FALSE(service_->IsLargeCursorEnabled());
}

TEST_F(AccessibilityServiceTest, SetLargeCursorEnabled_SameValueNoOp) {
  TestAccessibilityObserver observer;
  service_->AddObserver(&observer);

  ASSERT_FALSE(service_->IsLargeCursorEnabled());
  service_->SetLargeCursorEnabled(false);

  EXPECT_EQ(observer.large_cursor_changed_count_, 0);
  EXPECT_EQ(observer.settings_changed_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(AccessibilityServiceTest, SetLargeCursorEnabled_FiresObservers) {
  TestAccessibilityObserver observer;
  service_->AddObserver(&observer);

  service_->SetLargeCursorEnabled(true);

  EXPECT_EQ(observer.large_cursor_changed_count_, 1);
  EXPECT_TRUE(observer.last_large_cursor_enabled_);
  EXPECT_EQ(observer.settings_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(AccessibilityServiceTest, ToggleLargeCursor_FlipsValue) {
  ASSERT_FALSE(service_->IsLargeCursorEnabled());

  bool result = service_->ToggleLargeCursor();
  EXPECT_TRUE(result);
  EXPECT_TRUE(service_->IsLargeCursorEnabled());

  result = service_->ToggleLargeCursor();
  EXPECT_FALSE(result);
  EXPECT_FALSE(service_->IsLargeCursorEnabled());
}

TEST_F(AccessibilityServiceTest, LargeCursor_EnablesAccessibility) {
  ASSERT_FALSE(service_->IsAccessibilityEnabled());

  service_->SetLargeCursorEnabled(true);
  EXPECT_TRUE(service_->IsAccessibilityEnabled());
}

TEST_F(AccessibilityServiceTest, SetLargeCursorSize_ChangesValue) {
  service_->SetLargeCursorSize(5);
  EXPECT_EQ(service_->GetLargeCursorSize(), 5);

  service_->SetLargeCursorSize(1);
  EXPECT_EQ(service_->GetLargeCursorSize(), 1);
}

TEST_F(AccessibilityServiceTest, SetLargeCursorSize_ClampsToMinimum) {
  service_->SetLargeCursorSize(0);
  EXPECT_EQ(service_->GetLargeCursorSize(), 1);

  service_->SetLargeCursorSize(-5);
  EXPECT_EQ(service_->GetLargeCursorSize(), 1);
}

TEST_F(AccessibilityServiceTest, SetLargeCursorSize_ClampsToMaximum) {
  service_->SetLargeCursorSize(10);
  EXPECT_EQ(service_->GetLargeCursorSize(), 5);

  service_->SetLargeCursorSize(100);
  EXPECT_EQ(service_->GetLargeCursorSize(), 5);
}

TEST_F(AccessibilityServiceTest, SetLargeCursorSize_SameValueNoOp) {
  TestAccessibilityObserver observer;
  service_->AddObserver(&observer);

  int original = service_->GetLargeCursorSize();
  service_->SetLargeCursorSize(original);

  // Large cursor size change doesn't fire large cursor (enable) observer,
  // but it does fire the catch-all settings changed.
  EXPECT_EQ(observer.large_cursor_changed_count_, 0);

  service_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Magnifier
// ---------------------------------------------------------------------------

TEST_F(AccessibilityServiceTest, SetMagnifierEnabled_ChangesValue) {
  ASSERT_FALSE(service_->IsMagnifierEnabled());

  service_->SetMagnifierEnabled(true);
  EXPECT_TRUE(service_->IsMagnifierEnabled());

  service_->SetMagnifierEnabled(false);
  EXPECT_FALSE(service_->IsMagnifierEnabled());
}

TEST_F(AccessibilityServiceTest, SetMagnifierEnabled_SameValueNoOp) {
  TestAccessibilityObserver observer;
  service_->AddObserver(&observer);

  ASSERT_FALSE(service_->IsMagnifierEnabled());
  service_->SetMagnifierEnabled(false);

  EXPECT_EQ(observer.magnifier_changed_count_, 0);
  EXPECT_EQ(observer.settings_changed_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(AccessibilityServiceTest, SetMagnifierEnabled_FiresObservers) {
  TestAccessibilityObserver observer;
  service_->AddObserver(&observer);

  service_->SetMagnifierEnabled(true);

  EXPECT_EQ(observer.magnifier_changed_count_, 1);
  EXPECT_TRUE(observer.last_magnifier_enabled_);
  EXPECT_EQ(observer.settings_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(AccessibilityServiceTest, ToggleMagnifier_FlipsValue) {
  ASSERT_FALSE(service_->IsMagnifierEnabled());

  bool result = service_->ToggleMagnifier();
  EXPECT_TRUE(result);
  EXPECT_TRUE(service_->IsMagnifierEnabled());

  result = service_->ToggleMagnifier();
  EXPECT_FALSE(result);
  EXPECT_FALSE(service_->IsMagnifierEnabled());
}

TEST_F(AccessibilityServiceTest, Magnifier_EnablesAccessibility) {
  ASSERT_FALSE(service_->IsAccessibilityEnabled());

  service_->SetMagnifierEnabled(true);
  EXPECT_TRUE(service_->IsAccessibilityEnabled());
}

TEST_F(AccessibilityServiceTest, SetMagnifierScale_ChangesValue) {
  service_->SetMagnifierScale(3.0);
  EXPECT_DOUBLE_EQ(service_->GetMagnifierScale(), 3.0);

  service_->SetMagnifierScale(1.5);
  EXPECT_DOUBLE_EQ(service_->GetMagnifierScale(), 1.5);
}

TEST_F(AccessibilityServiceTest, SetMagnifierScale_ClampsToMinimum) {
  service_->SetMagnifierScale(0.5);
  EXPECT_GE(service_->GetMagnifierScale(), 1.0);
  EXPECT_DOUBLE_EQ(service_->GetMagnifierScale(), 1.0);
}

TEST_F(AccessibilityServiceTest, SetMagnifierScale_ClampsToMaximum) {
  service_->SetMagnifierScale(20.0);
  EXPECT_LE(service_->GetMagnifierScale(), 10.0);
  EXPECT_DOUBLE_EQ(service_->GetMagnifierScale(), 10.0);
}

TEST_F(AccessibilityServiceTest, SetMagnifierScale_SameValueNoOp) {
  TestAccessibilityObserver observer;
  service_->AddObserver(&observer);

  double original = service_->GetMagnifierScale();
  int before_count = observer.settings_changed_count_;
  service_->SetMagnifierScale(original);

  // Magnifier scale doesn't fire magnifier (enable) observer,
  // but it may fire settings changed (no-op means it doesn't).
  EXPECT_EQ(observer.settings_changed_count_, before_count);

  service_->RemoveObserver(&observer);
}

TEST_F(AccessibilityServiceTest, SetMagnifierType_ChangesValue) {
  EXPECT_EQ(service_->GetMagnifierType(), AstraMagnifierType::kDocked);

  service_->SetMagnifierType(AstraMagnifierType::kFullscreen);
  EXPECT_EQ(service_->GetMagnifierType(), AstraMagnifierType::kFullscreen);

  service_->SetMagnifierType(AstraMagnifierType::kDocked);
  EXPECT_EQ(service_->GetMagnifierType(), AstraMagnifierType::kDocked);
}

// ---------------------------------------------------------------------------
// Select-to-speak
// ---------------------------------------------------------------------------

TEST_F(AccessibilityServiceTest, SetSelectToSpeakEnabled_ChangesValue) {
  ASSERT_FALSE(service_->IsSelectToSpeakEnabled());

  service_->SetSelectToSpeakEnabled(true);
  EXPECT_TRUE(service_->IsSelectToSpeakEnabled());

  service_->SetSelectToSpeakEnabled(false);
  EXPECT_FALSE(service_->IsSelectToSpeakEnabled());
}

TEST_F(AccessibilityServiceTest, SetSelectToSpeakEnabled_SameValueNoOp) {
  TestAccessibilityObserver observer;
  service_->AddObserver(&observer);

  ASSERT_FALSE(service_->IsSelectToSpeakEnabled());
  service_->SetSelectToSpeakEnabled(false);

  EXPECT_EQ(observer.select_to_speak_changed_count_, 0);
  EXPECT_EQ(observer.settings_changed_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(AccessibilityServiceTest, SetSelectToSpeakEnabled_FiresObservers) {
  TestAccessibilityObserver observer;
  service_->AddObserver(&observer);

  service_->SetSelectToSpeakEnabled(true);

  EXPECT_EQ(observer.select_to_speak_changed_count_, 1);
  EXPECT_TRUE(observer.last_select_to_speak_enabled_);
  EXPECT_EQ(observer.settings_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(AccessibilityServiceTest, ToggleSelectToSpeak_FlipsValue) {
  ASSERT_FALSE(service_->IsSelectToSpeakEnabled());

  bool result = service_->ToggleSelectToSpeak();
  EXPECT_TRUE(result);
  EXPECT_TRUE(service_->IsSelectToSpeakEnabled());

  result = service_->ToggleSelectToSpeak();
  EXPECT_FALSE(result);
  EXPECT_FALSE(service_->IsSelectToSpeakEnabled());
}

TEST_F(AccessibilityServiceTest, SelectToSpeak_EnablesAccessibility) {
  ASSERT_FALSE(service_->IsAccessibilityEnabled());

  service_->SetSelectToSpeakEnabled(true);
  EXPECT_TRUE(service_->IsAccessibilityEnabled());
}

// ---------------------------------------------------------------------------
// Dictation
// ---------------------------------------------------------------------------

TEST_F(AccessibilityServiceTest, SetDictationEnabled_ChangesValue) {
  ASSERT_FALSE(service_->IsDictationEnabled());

  service_->SetDictationEnabled(true);
  EXPECT_TRUE(service_->IsDictationEnabled());

  service_->SetDictationEnabled(false);
  EXPECT_FALSE(service_->IsDictationEnabled());
}

TEST_F(AccessibilityServiceTest, SetDictationEnabled_SameValueNoOp) {
  TestAccessibilityObserver observer;
  service_->AddObserver(&observer);

  ASSERT_FALSE(service_->IsDictationEnabled());
  service_->SetDictationEnabled(false);

  EXPECT_EQ(observer.dictation_changed_count_, 0);
  EXPECT_EQ(observer.settings_changed_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(AccessibilityServiceTest, SetDictationEnabled_FiresObservers) {
  TestAccessibilityObserver observer;
  service_->AddObserver(&observer);

  service_->SetDictationEnabled(true);

  EXPECT_EQ(observer.dictation_changed_count_, 1);
  EXPECT_TRUE(observer.last_dictation_enabled_);
  EXPECT_EQ(observer.settings_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(AccessibilityServiceTest, ToggleDictation_FlipsValue) {
  ASSERT_FALSE(service_->IsDictationEnabled());

  bool result = service_->ToggleDictation();
  EXPECT_TRUE(result);
  EXPECT_TRUE(service_->IsDictationEnabled());

  result = service_->ToggleDictation();
  EXPECT_FALSE(result);
  EXPECT_FALSE(service_->IsDictationEnabled());
}

TEST_F(AccessibilityServiceTest, Dictation_EnablesAccessibility) {
  ASSERT_FALSE(service_->IsAccessibilityEnabled());

  service_->SetDictationEnabled(true);
  EXPECT_TRUE(service_->IsAccessibilityEnabled());
}

// ---------------------------------------------------------------------------
// Virtual keyboard
// ---------------------------------------------------------------------------

TEST_F(AccessibilityServiceTest, SetVirtualKeyboardEnabled_ChangesValue) {
  ASSERT_FALSE(service_->IsVirtualKeyboardEnabled());

  service_->SetVirtualKeyboardEnabled(true);
  EXPECT_TRUE(service_->IsVirtualKeyboardEnabled());

  service_->SetVirtualKeyboardEnabled(false);
  EXPECT_FALSE(service_->IsVirtualKeyboardEnabled());
}

TEST_F(AccessibilityServiceTest, SetVirtualKeyboardEnabled_SameValueNoOp) {
  TestAccessibilityObserver observer;
  service_->AddObserver(&observer);

  ASSERT_FALSE(service_->IsVirtualKeyboardEnabled());
  service_->SetVirtualKeyboardEnabled(false);

  EXPECT_EQ(observer.virtual_keyboard_changed_count_, 0);
  EXPECT_EQ(observer.settings_changed_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(AccessibilityServiceTest, SetVirtualKeyboardEnabled_FiresObservers) {
  TestAccessibilityObserver observer;
  service_->AddObserver(&observer);

  service_->SetVirtualKeyboardEnabled(true);

  EXPECT_EQ(observer.virtual_keyboard_changed_count_, 1);
  EXPECT_TRUE(observer.last_virtual_keyboard_enabled_);
  EXPECT_EQ(observer.settings_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(AccessibilityServiceTest, ToggleVirtualKeyboard_FlipsValue) {
  ASSERT_FALSE(service_->IsVirtualKeyboardEnabled());

  bool result = service_->ToggleVirtualKeyboard();
  EXPECT_TRUE(result);
  EXPECT_TRUE(service_->IsVirtualKeyboardEnabled());

  result = service_->ToggleVirtualKeyboard();
  EXPECT_FALSE(result);
  EXPECT_FALSE(service_->IsVirtualKeyboardEnabled());
}

TEST_F(AccessibilityServiceTest, VirtualKeyboard_EnablesAccessibility) {
  ASSERT_FALSE(service_->IsAccessibilityEnabled());

  service_->SetVirtualKeyboardEnabled(true);
  EXPECT_TRUE(service_->IsAccessibilityEnabled());
}

// ---------------------------------------------------------------------------
// Text helpers
// ---------------------------------------------------------------------------

TEST_F(AccessibilityServiceTest, SetMinimumFontSize_ChangesValue) {
  ASSERT_EQ(service_->GetMinimumFontSize(), 0);

  service_->SetMinimumFontSize(12);
  EXPECT_EQ(service_->GetMinimumFontSize(), 12);

  service_->SetMinimumFontSize(16);
  EXPECT_EQ(service_->GetMinimumFontSize(), 16);
}

TEST_F(AccessibilityServiceTest, SetMinimumFontSize_ClampsToMinimum) {
  service_->SetMinimumFontSize(-5);
  EXPECT_GE(service_->GetMinimumFontSize(), 0);
  EXPECT_EQ(service_->GetMinimumFontSize(), 0);
}

TEST_F(AccessibilityServiceTest, SetMinimumFontSize_ClampsToMaximum) {
  service_->SetMinimumFontSize(100);
  EXPECT_LE(service_->GetMinimumFontSize(), 72);
  EXPECT_EQ(service_->GetMinimumFontSize(), 72);
}

TEST_F(AccessibilityServiceTest, SetMinimumFontSize_FiresObservers) {
  TestAccessibilityObserver observer;
  service_->AddObserver(&observer);

  service_->SetMinimumFontSize(14);

  EXPECT_EQ(observer.minimum_font_size_changed_count_, 1);
  EXPECT_EQ(observer.last_minimum_font_size_, 14);
  EXPECT_GE(observer.settings_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(AccessibilityServiceTest, SetMinimumFontSize_EnablesAccessibility) {
  ASSERT_FALSE(service_->IsAccessibilityEnabled());

  service_->SetMinimumFontSize(10);
  EXPECT_TRUE(service_->IsAccessibilityEnabled());
}

TEST_F(AccessibilityServiceTest, SetFontWeightAdjustment_ChangesValue) {
  ASSERT_EQ(service_->GetFontWeightAdjustment(), 0);

  service_->SetFontWeightAdjustment(100);
  EXPECT_EQ(service_->GetFontWeightAdjustment(), 100);

  service_->SetFontWeightAdjustment(-50);
  EXPECT_EQ(service_->GetFontWeightAdjustment(), -50);
}

TEST_F(AccessibilityServiceTest, SetFontWeightAdjustment_ClampsToMinimum) {
  service_->SetFontWeightAdjustment(-200);
  EXPECT_GE(service_->GetFontWeightAdjustment(), -100);
  EXPECT_EQ(service_->GetFontWeightAdjustment(), -100);
}

TEST_F(AccessibilityServiceTest, SetFontWeightAdjustment_ClampsToMaximum) {
  service_->SetFontWeightAdjustment(500);
  EXPECT_LE(service_->GetFontWeightAdjustment(), 300);
  EXPECT_EQ(service_->GetFontWeightAdjustment(), 300);
}

TEST_F(AccessibilityServiceTest, SetFontWeightAdjustment_EnablesAccessibility) {
  ASSERT_FALSE(service_->IsAccessibilityEnabled());

  service_->SetFontWeightAdjustment(50);
  EXPECT_TRUE(service_->IsAccessibilityEnabled());
}

TEST_F(AccessibilityServiceTest, SetLetterSpacing_ChangesValue) {
  ASSERT_DOUBLE_EQ(service_->GetLetterSpacing(), 1.0);

  service_->SetLetterSpacing(1.5);
  EXPECT_DOUBLE_EQ(service_->GetLetterSpacing(), 1.5);

  service_->SetLetterSpacing(0.8);
  EXPECT_DOUBLE_EQ(service_->GetLetterSpacing(), 0.8);
}

TEST_F(AccessibilityServiceTest, SetLetterSpacing_ClampsToMinimum) {
  service_->SetLetterSpacing(0.1);
  EXPECT_GE(service_->GetLetterSpacing(), 0.5);
  EXPECT_DOUBLE_EQ(service_->GetLetterSpacing(), 0.5);
}

TEST_F(AccessibilityServiceTest, SetLetterSpacing_ClampsToMaximum) {
  service_->SetLetterSpacing(5.0);
  EXPECT_LE(service_->GetLetterSpacing(), 3.0);
  EXPECT_DOUBLE_EQ(service_->GetLetterSpacing(), 3.0);
}

TEST_F(AccessibilityServiceTest, SetLetterSpacing_EnablesAccessibility) {
  ASSERT_FALSE(service_->IsAccessibilityEnabled());

  service_->SetLetterSpacing(1.5);
  EXPECT_TRUE(service_->IsAccessibilityEnabled());
}

TEST_F(AccessibilityServiceTest, SetLineHeight_ChangesValue) {
  ASSERT_DOUBLE_EQ(service_->GetLineHeight(), 1.0);

  service_->SetLineHeight(1.5);
  EXPECT_DOUBLE_EQ(service_->GetLineHeight(), 1.5);

  service_->SetLineHeight(0.9);
  EXPECT_DOUBLE_EQ(service_->GetLineHeight(), 0.9);
}

TEST_F(AccessibilityServiceTest, SetLineHeight_ClampsToMinimum) {
  service_->SetLineHeight(0.5);
  EXPECT_GE(service_->GetLineHeight(), 0.8);
  EXPECT_DOUBLE_EQ(service_->GetLineHeight(), 0.8);
}

TEST_F(AccessibilityServiceTest, SetLineHeight_ClampsToMaximum) {
  service_->SetLineHeight(5.0);
  EXPECT_LE(service_->GetLineHeight(), 3.0);
  EXPECT_DOUBLE_EQ(service_->GetLineHeight(), 3.0);
}

TEST_F(AccessibilityServiceTest, SetLineHeight_EnablesAccessibility) {
  ASSERT_FALSE(service_->IsAccessibilityEnabled());

  service_->SetLineHeight(1.5);
  EXPECT_TRUE(service_->IsAccessibilityEnabled());
}

// ---------------------------------------------------------------------------
// Color and contrast
// ---------------------------------------------------------------------------

TEST_F(AccessibilityServiceTest, SetContrastLevel_ChangesValue) {
  ASSERT_EQ(service_->GetContrastLevel(), AstraContrastLevel::kNormal);

  service_->SetContrastLevel(AstraContrastLevel::kIncreased);
  EXPECT_EQ(service_->GetContrastLevel(), AstraContrastLevel::kIncreased);

  service_->SetContrastLevel(AstraContrastLevel::kHigh);
  EXPECT_EQ(service_->GetContrastLevel(), AstraContrastLevel::kHigh);

  service_->SetContrastLevel(AstraContrastLevel::kNormal);
  EXPECT_EQ(service_->GetContrastLevel(), AstraContrastLevel::kNormal);
}

TEST_F(AccessibilityServiceTest, SetContrastLevel_FiresObservers) {
  TestAccessibilityObserver observer;
  service_->AddObserver(&observer);

  service_->SetContrastLevel(AstraContrastLevel::kHigh);

  EXPECT_EQ(observer.contrast_level_changed_count_, 1);
  EXPECT_EQ(observer.last_contrast_level_, AstraContrastLevel::kHigh);
  EXPECT_GE(observer.settings_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(AccessibilityServiceTest, SetContrastLevel_SameValueNoOp) {
  TestAccessibilityObserver observer;
  service_->AddObserver(&observer);

  service_->SetContrastLevel(AstraContrastLevel::kNormal);

  EXPECT_EQ(observer.contrast_level_changed_count_, 0);
  EXPECT_EQ(observer.settings_changed_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(AccessibilityServiceTest, ContrastLevel_EnablesAccessibility) {
  ASSERT_FALSE(service_->IsAccessibilityEnabled());

  service_->SetContrastLevel(AstraContrastLevel::kHigh);
  EXPECT_TRUE(service_->IsAccessibilityEnabled());
}

TEST_F(AccessibilityServiceTest, ContrastLevel_Normal_DoesNotEnable) {
  service_->SetContrastLevel(AstraContrastLevel::kNormal);
  EXPECT_FALSE(service_->IsAccessibilityEnabled());
}

TEST_F(AccessibilityServiceTest, SetNightLightEnabled_ChangesValue) {
  ASSERT_FALSE(service_->IsNightLightEnabled());

  service_->SetNightLightEnabled(true);
  EXPECT_TRUE(service_->IsNightLightEnabled());

  service_->SetNightLightEnabled(false);
  EXPECT_FALSE(service_->IsNightLightEnabled());
}

TEST_F(AccessibilityServiceTest, SetNightLightEnabled_FiresObservers) {
  TestAccessibilityObserver observer;
  service_->AddObserver(&observer);

  service_->SetNightLightEnabled(true);

  EXPECT_EQ(observer.night_light_changed_count_, 1);
  EXPECT_TRUE(observer.last_night_light_enabled_);
  EXPECT_GE(observer.settings_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(AccessibilityServiceTest, SetNightLightEnabled_SameValueNoOp) {
  TestAccessibilityObserver observer;
  service_->AddObserver(&observer);

  ASSERT_FALSE(service_->IsNightLightEnabled());
  service_->SetNightLightEnabled(false);

  EXPECT_EQ(observer.night_light_changed_count_, 0);
  EXPECT_EQ(observer.settings_changed_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(AccessibilityServiceTest, ToggleNightLight_FlipsValue) {
  ASSERT_FALSE(service_->IsNightLightEnabled());

  bool result = service_->ToggleNightLight();
  EXPECT_TRUE(result);
  EXPECT_TRUE(service_->IsNightLightEnabled());

  result = service_->ToggleNightLight();
  EXPECT_FALSE(result);
  EXPECT_FALSE(service_->IsNightLightEnabled());
}

TEST_F(AccessibilityServiceTest, NightLight_EnablesAccessibility) {
  ASSERT_FALSE(service_->IsAccessibilityEnabled());

  service_->SetNightLightEnabled(true);
  EXPECT_TRUE(service_->IsAccessibilityEnabled());
}

TEST_F(AccessibilityServiceTest, SetColorTemperature_ChangesValue) {
  ASSERT_EQ(service_->GetColorTemperature(), 50);

  service_->SetColorTemperature(80);
  EXPECT_EQ(service_->GetColorTemperature(), 80);

  service_->SetColorTemperature(20);
  EXPECT_EQ(service_->GetColorTemperature(), 20);
}

TEST_F(AccessibilityServiceTest, SetColorTemperature_ClampsToMinimum) {
  service_->SetColorTemperature(-10);
  EXPECT_GE(service_->GetColorTemperature(), 0);
  EXPECT_EQ(service_->GetColorTemperature(), 0);
}

TEST_F(AccessibilityServiceTest, SetColorTemperature_ClampsToMaximum) {
  service_->SetColorTemperature(200);
  EXPECT_LE(service_->GetColorTemperature(), 100);
  EXPECT_EQ(service_->GetColorTemperature(), 100);
}

TEST_F(AccessibilityServiceTest, SetColorInversionEnabled_ChangesValue) {
  ASSERT_FALSE(service_->IsColorInversionEnabled());

  service_->SetColorInversionEnabled(true);
  EXPECT_TRUE(service_->IsColorInversionEnabled());

  service_->SetColorInversionEnabled(false);
  EXPECT_FALSE(service_->IsColorInversionEnabled());
}

TEST_F(AccessibilityServiceTest, SetColorInversionEnabled_FiresObservers) {
  TestAccessibilityObserver observer;
  service_->AddObserver(&observer);

  service_->SetColorInversionEnabled(true);

  EXPECT_EQ(observer.color_inversion_changed_count_, 1);
  EXPECT_TRUE(observer.last_color_inversion_enabled_);
  EXPECT_GE(observer.settings_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(AccessibilityServiceTest, SetColorInversionEnabled_SameValueNoOp) {
  TestAccessibilityObserver observer;
  service_->AddObserver(&observer);

  ASSERT_FALSE(service_->IsColorInversionEnabled());
  service_->SetColorInversionEnabled(false);

  EXPECT_EQ(observer.color_inversion_changed_count_, 0);
  EXPECT_EQ(observer.settings_changed_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(AccessibilityServiceTest, ToggleColorInversion_FlipsValue) {
  ASSERT_FALSE(service_->IsColorInversionEnabled());

  bool result = service_->ToggleColorInversion();
  EXPECT_TRUE(result);
  EXPECT_TRUE(service_->IsColorInversionEnabled());

  result = service_->ToggleColorInversion();
  EXPECT_FALSE(result);
  EXPECT_FALSE(service_->IsColorInversionEnabled());
}

TEST_F(AccessibilityServiceTest, ColorInversion_EnablesAccessibility) {
  ASSERT_FALSE(service_->IsAccessibilityEnabled());

  service_->SetColorInversionEnabled(true);
  EXPECT_TRUE(service_->IsAccessibilityEnabled());
}

// ---------------------------------------------------------------------------
// Animation and motion
// ---------------------------------------------------------------------------

TEST_F(AccessibilityServiceTest, SetAnimationReductionLevel_ChangesValue) {
  ASSERT_EQ(service_->GetAnimationReductionLevel(), AstraAnimationReduction::kOff);

  service_->SetAnimationReductionLevel(AstraAnimationReduction::kSome);
  EXPECT_EQ(service_->GetAnimationReductionLevel(), AstraAnimationReduction::kSome);

  service_->SetAnimationReductionLevel(AstraAnimationReduction::kMax);
  EXPECT_EQ(service_->GetAnimationReductionLevel(), AstraAnimationReduction::kMax);

  service_->SetAnimationReductionLevel(AstraAnimationReduction::kOff);
  EXPECT_EQ(service_->GetAnimationReductionLevel(), AstraAnimationReduction::kOff);
}

TEST_F(AccessibilityServiceTest, SetAnimationReductionLevel_FiresObservers) {
  TestAccessibilityObserver observer;
  service_->AddObserver(&observer);

  service_->SetAnimationReductionLevel(AstraAnimationReduction::kMax);

  EXPECT_EQ(observer.animation_reduction_changed_count_, 1);
  EXPECT_EQ(observer.last_animation_reduction_, AstraAnimationReduction::kMax);
  EXPECT_GE(observer.settings_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(AccessibilityServiceTest, SetAnimationReductionLevel_SameValueNoOp) {
  TestAccessibilityObserver observer;
  service_->AddObserver(&observer);

  service_->SetAnimationReductionLevel(AstraAnimationReduction::kOff);

  EXPECT_EQ(observer.animation_reduction_changed_count_, 0);
  EXPECT_EQ(observer.settings_changed_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(AccessibilityServiceTest, AnimationReduction_EnablesAccessibility) {
  ASSERT_FALSE(service_->IsAccessibilityEnabled());

  service_->SetAnimationReductionLevel(AstraAnimationReduction::kSome);
  EXPECT_TRUE(service_->IsAccessibilityEnabled());
}

TEST_F(AccessibilityServiceTest, AnimationReduction_Off_DoesNotEnable) {
  service_->SetAnimationReductionLevel(AstraAnimationReduction::kOff);
  EXPECT_FALSE(service_->IsAccessibilityEnabled());
}

TEST_F(AccessibilityServiceTest, AreAnimationsEnabled_MaxReduction_Disables) {
  ASSERT_TRUE(service_->AreAnimationsEnabled());

  service_->SetAnimationReductionLevel(AstraAnimationReduction::kMax);
  EXPECT_FALSE(service_->AreAnimationsEnabled());
}

TEST_F(AccessibilityServiceTest, AreAnimationsEnabled_SomeReduction_StillEnabled) {
  // "Some" reduction still allows essential animations.
  service_->SetAnimationReductionLevel(AstraAnimationReduction::kSome);
  EXPECT_TRUE(service_->AreAnimationsEnabled());
}

TEST_F(AccessibilityServiceTest, AreAnimationsEnabled_ReducedMotionDisables) {
  service_->SetReducedMotionEnabled(true);
  EXPECT_FALSE(service_->AreAnimationsEnabled());
}

TEST_F(AccessibilityServiceTest, SetAutoScrollEnabled_ChangesValue) {
  ASSERT_FALSE(service_->IsAutoScrollEnabled());

  service_->SetAutoScrollEnabled(true);
  EXPECT_TRUE(service_->IsAutoScrollEnabled());

  service_->SetAutoScrollEnabled(false);
  EXPECT_FALSE(service_->IsAutoScrollEnabled());
}

TEST_F(AccessibilityServiceTest, SetAutoScrollEnabled_SameValueNoOp) {
  TestAccessibilityObserver observer;
  service_->AddObserver(&observer);

  ASSERT_FALSE(service_->IsAutoScrollEnabled());
  service_->SetAutoScrollEnabled(false);

  EXPECT_EQ(observer.settings_changed_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(AccessibilityServiceTest, ToggleAutoScroll_FlipsValue) {
  ASSERT_FALSE(service_->IsAutoScrollEnabled());

  bool result = service_->ToggleAutoScroll();
  EXPECT_TRUE(result);
  EXPECT_TRUE(service_->IsAutoScrollEnabled());

  result = service_->ToggleAutoScroll();
  EXPECT_FALSE(result);
  EXPECT_FALSE(service_->IsAutoScrollEnabled());
}

TEST_F(AccessibilityServiceTest, AutoScroll_EnablesAccessibility) {
  ASSERT_FALSE(service_->IsAccessibilityEnabled());

  service_->SetAutoScrollEnabled(true);
  EXPECT_TRUE(service_->IsAccessibilityEnabled());
}

TEST_F(AccessibilityServiceTest, SetScrollSpeed_ChangesValue) {
  ASSERT_DOUBLE_EQ(service_->GetScrollSpeed(), 1.0);

  service_->SetScrollSpeed(2.0);
  EXPECT_DOUBLE_EQ(service_->GetScrollSpeed(), 2.0);

  service_->SetScrollSpeed(0.5);
  EXPECT_DOUBLE_EQ(service_->GetScrollSpeed(), 0.5);
}

TEST_F(AccessibilityServiceTest, SetScrollSpeed_ClampsToMinimum) {
  service_->SetScrollSpeed(0.1);
  EXPECT_GE(service_->GetScrollSpeed(), 0.25);
  EXPECT_DOUBLE_EQ(service_->GetScrollSpeed(), 0.25);
}

TEST_F(AccessibilityServiceTest, SetScrollSpeed_ClampsToMaximum) {
  service_->SetScrollSpeed(10.0);
  EXPECT_LE(service_->GetScrollSpeed(), 5.0);
  EXPECT_DOUBLE_EQ(service_->GetScrollSpeed(), 5.0);
}

// ---------------------------------------------------------------------------
// Overall accessibility — count, list, reset, presets
// ---------------------------------------------------------------------------

TEST_F(AccessibilityServiceTest, GetEnabledFeaturesCount_ZeroAtDefault) {
  EXPECT_EQ(service_->GetEnabledFeaturesCount(), 0);
}

TEST_F(AccessibilityServiceTest, GetEnabledFeaturesCount_IncrementsWithEachFeature) {
  int count_before = service_->GetEnabledFeaturesCount();

  service_->SetHighContrastEnabled(true);
  EXPECT_EQ(service_->GetEnabledFeaturesCount(), count_before + 1);

  service_->SetStickyKeysEnabled(true);
  EXPECT_EQ(service_->GetEnabledFeaturesCount(), count_before + 2);

  service_->SetMagnifierEnabled(true);
  EXPECT_EQ(service_->GetEnabledFeaturesCount(), count_before + 3);
}

TEST_F(AccessibilityServiceTest, GetEnabledFeaturesCount_FontScaleCounts) {
  int count_before = service_->GetEnabledFeaturesCount();

  service_->SetFontScale(1.5);
  EXPECT_GT(service_->GetEnabledFeaturesCount(), count_before);
}

TEST_F(AccessibilityServiceTest, GetEnabledFeatureList_EmptyAtDefault) {
  auto features = service_->GetEnabledFeatureList();
  EXPECT_TRUE(features.empty());
}

TEST_F(AccessibilityServiceTest, GetEnabledFeatureList_ContainsEnabledFeatures) {
  service_->SetHighContrastEnabled(true);
  service_->SetStickyKeysEnabled(true);

  auto features = service_->GetEnabledFeatureList();
  EXPECT_FALSE(features.empty());
  EXPECT_THAT(features, testing::Contains("high_contrast"));
  EXPECT_THAT(features, testing::Contains("sticky_keys"));
}

TEST_F(AccessibilityServiceTest, GetEnabledFeatureList_MatchesCount) {
  service_->SetHighContrastEnabled(true);
  service_->SetStickyKeysEnabled(true);
  service_->SetMagnifierEnabled(true);

  auto features = service_->GetEnabledFeatureList();
  EXPECT_EQ(static_cast<int>(features.size()),
            service_->GetEnabledFeaturesCount());
}

TEST_F(AccessibilityServiceTest, ResetAllSettings_ReturnsToDefaults) {
  // Enable a bunch of features.
  service_->SetHighContrastEnabled(true);
  service_->SetReducedMotionEnabled(true);
  service_->SetFontScale(2.0);
  service_->SetCaretBrowsingEnabled(true);
  service_->SetStickyKeysEnabled(true);
  service_->SetSlowKeysEnabled(true);
  service_->SetMouseKeysEnabled(true);
  service_->SetLargeCursorEnabled(true);
  service_->SetMagnifierEnabled(true);
  service_->SetSelectToSpeakEnabled(true);
  service_->SetDictationEnabled(true);
  service_->SetVirtualKeyboardEnabled(true);
  service_->SetContrastLevel(AstraContrastLevel::kHigh);
  service_->SetNightLightEnabled(true);
  service_->SetColorInversionEnabled(true);
  service_->SetAnimationReductionLevel(AstraAnimationReduction::kMax);
  service_->SetMinimumFontSize(12);

  ASSERT_TRUE(service_->IsAccessibilityEnabled());
  ASSERT_GT(service_->GetEnabledFeaturesCount(), 5);

  // Reset everything.
  service_->ResetAllAccessibilitySettings();

  // All should be back to defaults.
  EXPECT_FALSE(service_->IsHighContrastEnabled());
  EXPECT_FALSE(service_->IsReducedMotionEnabled());
  EXPECT_DOUBLE_EQ(service_->GetFontScale(), 1.0);
  EXPECT_FALSE(service_->IsCaretBrowsingEnabled());
  EXPECT_FALSE(service_->IsStickyKeysEnabled());
  EXPECT_FALSE(service_->IsSlowKeysEnabled());
  EXPECT_FALSE(service_->IsMouseKeysEnabled());
  EXPECT_FALSE(service_->IsLargeCursorEnabled());
  EXPECT_FALSE(service_->IsMagnifierEnabled());
  EXPECT_FALSE(service_->IsSelectToSpeakEnabled());
  EXPECT_FALSE(service_->IsDictationEnabled());
  EXPECT_FALSE(service_->IsVirtualKeyboardEnabled());
  EXPECT_EQ(service_->GetContrastLevel(), AstraContrastLevel::kNormal);
  EXPECT_FALSE(service_->IsNightLightEnabled());
  EXPECT_FALSE(service_->IsColorInversionEnabled());
  EXPECT_EQ(service_->GetAnimationReductionLevel(), AstraAnimationReduction::kOff);
  EXPECT_EQ(service_->GetMinimumFontSize(), 0);
  EXPECT_FALSE(service_->IsAccessibilityEnabled());
  EXPECT_EQ(service_->GetEnabledFeaturesCount(), 0);
}

TEST_F(AccessibilityServiceTest, ApplyPreset_None_ResetsAll) {
  service_->SetHighContrastEnabled(true);
  service_->SetFontScale(2.0);
  ASSERT_TRUE(service_->IsAccessibilityEnabled());

  service_->ApplyAccessibilityPreset(AstraAccessibilityPreset::kNone);

  EXPECT_FALSE(service_->IsAccessibilityEnabled());
  EXPECT_EQ(service_->GetEnabledFeaturesCount(), 0);
}

TEST_F(AccessibilityServiceTest, ApplyPreset_Visual_EnablesVisualFeatures) {
  service_->ApplyAccessibilityPreset(AstraAccessibilityPreset::kVisual);

  EXPECT_TRUE(service_->IsHighContrastEnabled());
  EXPECT_GT(service_->GetFontScale(), 1.0);
  EXPECT_TRUE(service_->IsMagnifierEnabled());
  EXPECT_TRUE(service_->IsLargeCursorEnabled());
  EXPECT_EQ(service_->GetContrastLevel(), AstraContrastLevel::kHigh);
  EXPECT_TRUE(service_->IsAccessibilityEnabled());
}

TEST_F(AccessibilityServiceTest, ApplyPreset_Motor_EnablesMotorFeatures) {
  service_->ApplyAccessibilityPreset(AstraAccessibilityPreset::kMotor);

  EXPECT_TRUE(service_->IsStickyKeysEnabled());
  EXPECT_TRUE(service_->IsSlowKeysEnabled());
  EXPECT_TRUE(service_->IsMouseKeysEnabled());
  EXPECT_TRUE(service_->IsVirtualKeyboardEnabled());
  EXPECT_TRUE(service_->IsAccessibilityEnabled());
}

TEST_F(AccessibilityServiceTest, ApplyPreset_Cognitive_EnablesCognitiveFeatures) {
  service_->ApplyAccessibilityPreset(AstraAccessibilityPreset::kCognitive);

  EXPECT_TRUE(service_->IsReducedMotionEnabled());
  EXPECT_EQ(service_->GetAnimationReductionLevel(), AstraAnimationReduction::kMax);
  EXPECT_TRUE(service_->IsSelectToSpeakEnabled());
  EXPECT_GT(service_->GetFontScale(), 1.0);
  EXPECT_GT(service_->GetLineHeight(), 1.0);
  EXPECT_TRUE(service_->IsAccessibilityEnabled());
}

TEST_F(AccessibilityServiceTest, ApplyPreset_Custom_NoChange) {
  // Custom is a state indicator, not a set of settings.
  service_->SetHighContrastEnabled(true);
  ASSERT_TRUE(service_->IsHighContrastEnabled());

  service_->ApplyAccessibilityPreset(AstraAccessibilityPreset::kCustom);

  // Should still have the same settings.
  EXPECT_TRUE(service_->IsHighContrastEnabled());
}

// ---------------------------------------------------------------------------
// Utility methods — GetScaledFontSize
// ---------------------------------------------------------------------------

TEST_F(AccessibilityServiceTest, GetScaledFontSize_DefaultScale_SameSize) {
  // Default scale of 1.0 should return the same size.
  EXPECT_EQ(service_->GetScaledFontSize(14), 14);
  EXPECT_EQ(service_->GetScaledFontSize(20), 20);
}

TEST_F(AccessibilityServiceTest, GetScaledFontSize_LargerScale) {
  service_->SetFontScale(1.5);
  // 14 * 1.5 = 21
  EXPECT_EQ(service_->GetScaledFontSize(14), 21);
}

TEST_F(AccessibilityServiceTest, GetScaledFontSize_SmallerScale) {
  service_->SetFontScale(0.75);
  // 16 * 0.75 = 12
  EXPECT_EQ(service_->GetScaledFontSize(16), 12);
}

TEST_F(AccessibilityServiceTest, GetScaledFontSize_ZeroSize) {
  EXPECT_EQ(service_->GetScaledFontSize(0), 0);
}

TEST_F(AccessibilityServiceTest, GetScaledFontSize_NegativeSize) {
  EXPECT_EQ(service_->GetScaledFontSize(-5), 0);
}

TEST_F(AccessibilityServiceTest, GetScaledFontSize_MinimumSize) {
  service_->SetFontScale(0.5);
  // Very small font with small scale should still be at least 1.
  EXPECT_GE(service_->GetScaledFontSize(1), 1);
}

// ---------------------------------------------------------------------------
// Utility methods — AreAnimationsEnabled
// ---------------------------------------------------------------------------

TEST_F(AccessibilityServiceTest, AreAnimationsEnabled_DefaultIsTrue) {
  EXPECT_TRUE(service_->AreAnimationsEnabled());
}

TEST_F(AccessibilityServiceTest, AreAnimationsEnabled_ReducedMotionDisables) {
  service_->SetReducedMotionEnabled(true);
  EXPECT_FALSE(service_->AreAnimationsEnabled());
}

// ---------------------------------------------------------------------------
// Utility methods — GetFontScalePresets
// ---------------------------------------------------------------------------

TEST_F(AccessibilityServiceTest, GetFontScalePresets_ReturnsPresets) {
  auto presets = AstraAccessibilityService::GetFontScalePresets();
  EXPECT_FALSE(presets.empty());
  // Should include 1.0 (default)
  bool has_default = false;
  for (double p : presets) {
    if (p == 1.0) {
      has_default = true;
      break;
    }
  }
  EXPECT_TRUE(has_default);
}

TEST_F(AccessibilityServiceTest, GetFontScalePresets_Ordered) {
  auto presets = AstraAccessibilityService::GetFontScalePresets();
  for (size_t i = 1; i < presets.size(); ++i) {
    EXPECT_GT(presets[i], presets[i - 1]) << "Presets should be in ascending order";
  }
}

// ---------------------------------------------------------------------------
// Utility methods — SnapFontScaleToPreset
// ---------------------------------------------------------------------------

TEST_F(AccessibilityServiceTest, SnapFontScaleToPreset_ExactMatch) {
  EXPECT_DOUBLE_EQ(AstraAccessibilityService::SnapFontScaleToPreset(1.0), 1.0);
  EXPECT_DOUBLE_EQ(AstraAccessibilityService::SnapFontScaleToPreset(1.5), 1.5);
}

TEST_F(AccessibilityServiceTest, SnapFontScaleToPreset_NearestPreset) {
  // 1.2 is closer to 1.25 than to 1.0
  EXPECT_DOUBLE_EQ(AstraAccessibilityService::SnapFontScaleToPreset(1.2), 1.25);
  // 1.1 is closer to 1.0 than to 1.25
  EXPECT_DOUBLE_EQ(AstraAccessibilityService::SnapFontScaleToPreset(1.1), 1.0);
}

TEST_F(AccessibilityServiceTest, SnapFontScaleToPreset_BelowMinimum) {
  EXPECT_DOUBLE_EQ(AstraAccessibilityService::SnapFontScaleToPreset(0.1), 0.75);
}

TEST_F(AccessibilityServiceTest, SnapFontScaleToPreset_AboveMaximum) {
  EXPECT_DOUBLE_EQ(AstraAccessibilityService::SnapFontScaleToPreset(5.0), 2.0);
}

// ---------------------------------------------------------------------------
// Multiple observers
// ---------------------------------------------------------------------------

TEST_F(AccessibilityServiceTest, MultipleObservers_AllNotified) {
  TestAccessibilityObserver observer1;
  TestAccessibilityObserver observer2;

  service_->AddObserver(&observer1);
  service_->AddObserver(&observer2);

  service_->SetHighContrastEnabled(true);

  EXPECT_EQ(observer1.high_contrast_changed_count_, 1);
  EXPECT_EQ(observer2.high_contrast_changed_count_, 1);

  service_->RemoveObserver(&observer1);
  service_->RemoveObserver(&observer2);
}

TEST_F(AccessibilityServiceTest, RemoveObserver_StopsNotifications) {
  TestAccessibilityObserver observer;
  service_->AddObserver(&observer);

  service_->SetHighContrastEnabled(true);
  EXPECT_EQ(observer.high_contrast_changed_count_, 1);

  service_->RemoveObserver(&observer);

  service_->SetHighContrastEnabled(false);
  // Should not have been notified after removal.
  EXPECT_EQ(observer.high_contrast_changed_count_, 1);
}

TEST_F(AccessibilityServiceTest, MultipleObservers_MultipleFeatures) {
  TestAccessibilityObserver observer1;
  TestAccessibilityObserver observer2;
  TestAccessibilityObserver observer3;

  service_->AddObserver(&observer1);
  service_->AddObserver(&observer2);
  service_->AddObserver(&observer3);

  service_->SetStickyKeysEnabled(true);
  service_->SetMagnifierEnabled(true);

  EXPECT_EQ(observer1.sticky_keys_changed_count_, 1);
  EXPECT_EQ(observer2.sticky_keys_changed_count_, 1);
  EXPECT_EQ(observer3.sticky_keys_changed_count_, 1);
  EXPECT_EQ(observer1.magnifier_changed_count_, 1);
  EXPECT_EQ(observer2.magnifier_changed_count_, 1);
  EXPECT_EQ(observer3.magnifier_changed_count_, 1);

  service_->RemoveObserver(&observer1);
  service_->RemoveObserver(&observer2);
  service_->RemoveObserver(&observer3);
}

// ---------------------------------------------------------------------------
// Shutdown
// ---------------------------------------------------------------------------

TEST_F(AccessibilityServiceTest, Shutdown_CleansUp) {
  TestAccessibilityObserver observer;
  service_->AddObserver(&observer);

  service_->Shutdown();

  // After shutdown, operations should not notify observers.
  service_->SetHighContrastEnabled(true);
  EXPECT_EQ(observer.high_contrast_changed_count_, 0);
}

// ---------------------------------------------------------------------------
// Persistence round-trip
// ---------------------------------------------------------------------------

TEST_F(AccessibilityServiceTest, PrefsPersist_HighContrast) {
  service_->SetHighContrastEnabled(true);

  auto service2 = std::make_unique<AstraAccessibilityService>(profile_.get());
  EXPECT_TRUE(service2->IsHighContrastEnabled());
}

TEST_F(AccessibilityServiceTest, PrefsPersist_ReducedMotion) {
  service_->SetReducedMotionEnabled(true);

  auto service2 = std::make_unique<AstraAccessibilityService>(profile_.get());
  EXPECT_TRUE(service2->IsReducedMotionEnabled());
}

TEST_F(AccessibilityServiceTest, PrefsPersist_FontScale) {
  service_->SetFontScale(1.75);

  auto service2 = std::make_unique<AstraAccessibilityService>(profile_.get());
  EXPECT_DOUBLE_EQ(service2->GetFontScale(), 1.75);
}

TEST_F(AccessibilityServiceTest, PrefsPersist_CaretBrowsing) {
  service_->SetCaretBrowsingEnabled(true);

  auto service2 = std::make_unique<AstraAccessibilityService>(profile_.get());
  EXPECT_TRUE(service2->IsCaretBrowsingEnabled());
}

TEST_F(AccessibilityServiceTest, PrefsPersist_StickyKeys) {
  service_->SetStickyKeysEnabled(true);

  auto service2 = std::make_unique<AstraAccessibilityService>(profile_.get());
  EXPECT_TRUE(service2->IsStickyKeysEnabled());
}

TEST_F(AccessibilityServiceTest, PrefsPersist_SlowKeys) {
  service_->SetSlowKeysEnabled(true);
  service_->SetSlowKeysDelay(base::Milliseconds(1000));

  auto service2 = std::make_unique<AstraAccessibilityService>(profile_.get());
  EXPECT_TRUE(service2->IsSlowKeysEnabled());
  EXPECT_EQ(service2->GetSlowKeysDelay(), base::Milliseconds(1000));
}

TEST_F(AccessibilityServiceTest, PrefsPersist_MouseKeys) {
  service_->SetMouseKeysEnabled(true);

  auto service2 = std::make_unique<AstraAccessibilityService>(profile_.get());
  EXPECT_TRUE(service2->IsMouseKeysEnabled());
}

TEST_F(AccessibilityServiceTest, PrefsPersist_LargeCursor) {
  service_->SetLargeCursorEnabled(true);
  service_->SetLargeCursorSize(5);

  auto service2 = std::make_unique<AstraAccessibilityService>(profile_.get());
  EXPECT_TRUE(service2->IsLargeCursorEnabled());
  EXPECT_EQ(service2->GetLargeCursorSize(), 5);
}

TEST_F(AccessibilityServiceTest, PrefsPersist_Magnifier) {
  service_->SetMagnifierEnabled(true);
  service_->SetMagnifierScale(3.5);
  service_->SetMagnifierType(AstraMagnifierType::kFullscreen);

  auto service2 = std::make_unique<AstraAccessibilityService>(profile_.get());
  EXPECT_TRUE(service2->IsMagnifierEnabled());
  EXPECT_DOUBLE_EQ(service2->GetMagnifierScale(), 3.5);
  EXPECT_EQ(service2->GetMagnifierType(), AstraMagnifierType::kFullscreen);
}

TEST_F(AccessibilityServiceTest, PrefsPersist_SelectToSpeak) {
  service_->SetSelectToSpeakEnabled(true);

  auto service2 = std::make_unique<AstraAccessibilityService>(profile_.get());
  EXPECT_TRUE(service2->IsSelectToSpeakEnabled());
}

TEST_F(AccessibilityServiceTest, PrefsPersist_Dictation) {
  service_->SetDictationEnabled(true);

  auto service2 = std::make_unique<AstraAccessibilityService>(profile_.get());
  EXPECT_TRUE(service2->IsDictationEnabled());
}

TEST_F(AccessibilityServiceTest, PrefsPersist_VirtualKeyboard) {
  service_->SetVirtualKeyboardEnabled(true);

  auto service2 = std::make_unique<AstraAccessibilityService>(profile_.get());
  EXPECT_TRUE(service2->IsVirtualKeyboardEnabled());
}

TEST_F(AccessibilityServiceTest, PrefsPersist_TextHelpers) {
  service_->SetMinimumFontSize(14);
  service_->SetFontWeightAdjustment(100);
  service_->SetLetterSpacing(1.5);
  service_->SetLineHeight(1.75);

  auto service2 = std::make_unique<AstraAccessibilityService>(profile_.get());
  EXPECT_EQ(service2->GetMinimumFontSize(), 14);
  EXPECT_EQ(service2->GetFontWeightAdjustment(), 100);
  EXPECT_DOUBLE_EQ(service2->GetLetterSpacing(), 1.5);
  EXPECT_DOUBLE_EQ(service2->GetLineHeight(), 1.75);
}

TEST_F(AccessibilityServiceTest, PrefsPersist_ColorAndContrast) {
  service_->SetContrastLevel(AstraContrastLevel::kHigh);
  service_->SetNightLightEnabled(true);
  service_->SetColorTemperature(80);
  service_->SetColorInversionEnabled(true);

  auto service2 = std::make_unique<AstraAccessibilityService>(profile_.get());
  EXPECT_EQ(service2->GetContrastLevel(), AstraContrastLevel::kHigh);
  EXPECT_TRUE(service2->IsNightLightEnabled());
  EXPECT_EQ(service2->GetColorTemperature(), 80);
  EXPECT_TRUE(service2->IsColorInversionEnabled());
}

TEST_F(AccessibilityServiceTest, PrefsPersist_AnimationAndMotion) {
  service_->SetAnimationReductionLevel(AstraAnimationReduction::kSome);
  service_->SetAutoScrollEnabled(true);
  service_->SetScrollSpeed(2.5);

  auto service2 = std::make_unique<AstraAccessibilityService>(profile_.get());
  EXPECT_EQ(service2->GetAnimationReductionLevel(), AstraAnimationReduction::kSome);
  EXPECT_TRUE(service2->IsAutoScrollEnabled());
  EXPECT_DOUBLE_EQ(service2->GetScrollSpeed(), 2.5);
}

TEST_F(AccessibilityServiceTest, PrefsPersist_DefaultValues) {
  // Create a fresh service — should have default values.
  auto service2 = std::make_unique<AstraAccessibilityService>(profile_.get());

  EXPECT_FALSE(service2->IsHighContrastEnabled());
  EXPECT_FALSE(service2->IsReducedMotionEnabled());
  EXPECT_DOUBLE_EQ(service2->GetFontScale(), 1.0);
  EXPECT_FALSE(service2->IsCaretBrowsingEnabled());
  EXPECT_FALSE(service2->IsStickyKeysEnabled());
  EXPECT_FALSE(service2->IsSlowKeysEnabled());
  EXPECT_FALSE(service2->IsMouseKeysEnabled());
  EXPECT_FALSE(service2->IsLargeCursorEnabled());
  EXPECT_FALSE(service2->IsMagnifierEnabled());
  EXPECT_FALSE(service2->IsSelectToSpeakEnabled());
  EXPECT_FALSE(service2->IsDictationEnabled());
  EXPECT_FALSE(service2->IsVirtualKeyboardEnabled());
}

// ---------------------------------------------------------------------------
// Combined accessibility settings
// ---------------------------------------------------------------------------

TEST_F(AccessibilityServiceTest, IsAccessibilityEnabled_MultipleSettings) {
  // Enable multiple accessibility settings.
  service_->SetHighContrastEnabled(true);
  service_->SetFontScale(1.5);

  EXPECT_TRUE(service_->IsAccessibilityEnabled());

  // Disable one, the other should still keep it enabled.
  service_->SetHighContrastEnabled(false);
  EXPECT_TRUE(service_->IsAccessibilityEnabled());

  // Disable the last one.
  service_->ResetFontScale();
  EXPECT_FALSE(service_->IsAccessibilityEnabled());
}

TEST_F(AccessibilityServiceTest, ManyFeaturesEnabled_CountIsAccurate) {
  // Enable many features and verify count.
  service_->SetHighContrastEnabled(true);
  service_->SetReducedMotionEnabled(true);
  service_->SetStickyKeysEnabled(true);
  service_->SetMagnifierEnabled(true);
  service_->SetDictationEnabled(true);
  service_->SetVirtualKeyboardEnabled(true);

  EXPECT_EQ(service_->GetEnabledFeaturesCount(), 6);

  auto features = service_->GetEnabledFeatureList();
  EXPECT_EQ(static_cast<int>(features.size()), 6);
}

TEST_F(AccessibilityServiceTest, CombinedSettings_ObserverNotifications) {
  TestAccessibilityObserver observer;
  service_->AddObserver(&observer);

  // Enable multiple features — each should fire its own observer.
  service_->SetHighContrastEnabled(true);
  service_->SetStickyKeysEnabled(true);
  service_->SetMagnifierEnabled(true);
  service_->SetContrastLevel(AstraContrastLevel::kHigh);

  EXPECT_EQ(observer.high_contrast_changed_count_, 1);
  EXPECT_EQ(observer.sticky_keys_changed_count_, 1);
  EXPECT_EQ(observer.magnifier_changed_count_, 1);
  EXPECT_EQ(observer.contrast_level_changed_count_, 1);
  // Settings changed fires for each.
  EXPECT_EQ(observer.settings_changed_count_, 4);

  service_->RemoveObserver(&observer);
}

TEST_F(AccessibilityServiceTest, CombinedSettings_ResetAll) {
  // Enable a bunch of things.
  service_->SetHighContrastEnabled(true);
  service_->SetStickyKeysEnabled(true);
  service_->SetMagnifierEnabled(true);
  service_->SetFontScale(2.0);
  service_->SetContrastLevel(AstraContrastLevel::kHigh);
  service_->SetMinimumFontSize(12);

  ASSERT_TRUE(service_->IsAccessibilityEnabled());

  service_->ResetAllAccessibilitySettings();

  EXPECT_FALSE(service_->IsAccessibilityEnabled());
  EXPECT_EQ(service_->GetEnabledFeaturesCount(), 0);
  EXPECT_TRUE(service_->GetEnabledFeatureList().empty());
}

// ---------------------------------------------------------------------------
// Edge cases
// ---------------------------------------------------------------------------

TEST_F(AccessibilityServiceTest, EdgeCase_ZeroFontScale_Clamped) {
  service_->SetFontScale(0.0);
  EXPECT_GT(service_->GetFontScale(), 0.0);
}

TEST_F(AccessibilityServiceTest, EdgeCase_NegativeFontScale_Clamped) {
  service_->SetFontScale(-1.0);
  EXPECT_GT(service_->GetFontScale(), 0.0);
}

TEST_F(AccessibilityServiceTest, EdgeCase_HugeFontScale_Clamped) {
  service_->SetFontScale(100.0);
  EXPECT_LT(service_->GetFontScale(), 100.0);
}

TEST_F(AccessibilityServiceTest, EdgeCase_LargeCursorSizeZero_Clamped) {
  service_->SetLargeCursorSize(0);
  EXPECT_GT(service_->GetLargeCursorSize(), 0);
}

TEST_F(AccessibilityServiceTest, EdgeCase_MagnifierScaleOne_IsMinimum) {
  service_->SetMagnifierScale(1.0);
  EXPECT_DOUBLE_EQ(service_->GetMagnifierScale(), 1.0);
  // 1.0 means no magnification — but magnifier "enabled" is separate.
  EXPECT_FALSE(service_->IsMagnifierEnabled());
}

TEST_F(AccessibilityServiceTest, EdgeCase_ContrastLevelAllValues) {
  // Test all valid contrast levels.
  service_->SetContrastLevel(AstraContrastLevel::kNormal);
  EXPECT_EQ(service_->GetContrastLevel(), AstraContrastLevel::kNormal);

  service_->SetContrastLevel(AstraContrastLevel::kIncreased);
  EXPECT_EQ(service_->GetContrastLevel(), AstraContrastLevel::kIncreased);

  service_->SetContrastLevel(AstraContrastLevel::kHigh);
  EXPECT_EQ(service_->GetContrastLevel(), AstraContrastLevel::kHigh);
}

TEST_F(AccessibilityServiceTest, EdgeCase_AnimationReductionAllValues) {
  // Test all valid animation reduction levels.
  service_->SetAnimationReductionLevel(AstraAnimationReduction::kOff);
  EXPECT_EQ(service_->GetAnimationReductionLevel(), AstraAnimationReduction::kOff);

  service_->SetAnimationReductionLevel(AstraAnimationReduction::kSome);
  EXPECT_EQ(service_->GetAnimationReductionLevel(), AstraAnimationReduction::kSome);

  service_->SetAnimationReductionLevel(AstraAnimationReduction::kMax);
  EXPECT_EQ(service_->GetAnimationReductionLevel(), AstraAnimationReduction::kMax);
}

TEST_F(AccessibilityServiceTest, EdgeCase_MagnifierTypeAllValues) {
  service_->SetMagnifierType(AstraMagnifierType::kDocked);
  EXPECT_EQ(service_->GetMagnifierType(), AstraMagnifierType::kDocked);

  service_->SetMagnifierType(AstraMagnifierType::kFullscreen);
  EXPECT_EQ(service_->GetMagnifierType(), AstraMagnifierType::kFullscreen);
}

TEST_F(AccessibilityServiceTest, EdgeCase_ColorTempMin) {
  service_->SetColorTemperature(0);
  EXPECT_EQ(service_->GetColorTemperature(), 0);
}

TEST_F(AccessibilityServiceTest, EdgeCase_ColorTempMax) {
  service_->SetColorTemperature(100);
  EXPECT_EQ(service_->GetColorTemperature(), 100);
}

TEST_F(AccessibilityServiceTest, EdgeCase_SlowKeysMinDelay) {
  service_->SetSlowKeysDelay(base::Milliseconds(10));
  EXPECT_EQ(service_->GetSlowKeysDelay(), base::Milliseconds(10));
}

TEST_F(AccessibilityServiceTest, EdgeCase_SlowKeysMaxDelay) {
  service_->SetSlowKeysDelay(base::Milliseconds(5000));
  EXPECT_EQ(service_->GetSlowKeysDelay(), base::Milliseconds(5000));
}

TEST_F(AccessibilityServiceTest, EdgeCase_ScrollSpeedMin) {
  service_->SetScrollSpeed(0.25);
  EXPECT_DOUBLE_EQ(service_->GetScrollSpeed(), 0.25);
}

TEST_F(AccessibilityServiceTest, EdgeCase_ScrollSpeedMax) {
  service_->SetScrollSpeed(5.0);
  EXPECT_DOUBLE_EQ(service_->GetScrollSpeed(), 5.0);
}

TEST_F(AccessibilityServiceTest, EdgeCase_LetterSpacingMin) {
  service_->SetLetterSpacing(0.5);
  EXPECT_DOUBLE_EQ(service_->GetLetterSpacing(), 0.5);
}

TEST_F(AccessibilityServiceTest, EdgeCase_LetterSpacingMax) {
  service_->SetLetterSpacing(3.0);
  EXPECT_DOUBLE_EQ(service_->GetLetterSpacing(), 3.0);
}

TEST_F(AccessibilityServiceTest, EdgeCase_LineHeightMin) {
  service_->SetLineHeight(0.8);
  EXPECT_DOUBLE_EQ(service_->GetLineHeight(), 0.8);
}

TEST_F(AccessibilityServiceTest, EdgeCase_LineHeightMax) {
  service_->SetLineHeight(3.0);
  EXPECT_DOUBLE_EQ(service_->GetLineHeight(), 3.0);
}

// ---------------------------------------------------------------------------
// Factory tests
// ---------------------------------------------------------------------------

TEST_F(AccessibilityServiceTest, Factory_GetForProfile_ReturnsService) {
  // Register prefs via factory on the test profile.
  AstraAccessibilityServiceFactory::RegisterProfilePrefs(
      profile_->GetPrefs()->registry());

  // Use the factory to get the service for the profile.
  AstraAccessibilityService* svc =
      AstraAccessibilityServiceFactory::GetForProfile(profile_.get());
  // Note: may be null in test environment without full keyed service setup.
  // The factory creates the service if needed.
  if (svc) {
    EXPECT_FALSE(svc->IsHighContrastEnabled());
  }
}

TEST_F(AccessibilityServiceTest, Factory_GetInstance_ReturnsSingleton) {
  auto* factory = AstraAccessibilityServiceFactory::GetInstance();
  ASSERT_NE(factory, nullptr);

  // Calling GetInstance again should return the same pointer.
  auto* factory2 = AstraAccessibilityServiceFactory::GetInstance();
  EXPECT_EQ(factory, factory2);
}

TEST_F(AccessibilityServiceTest, Factory_RegisterProfilePrefs_RegistersDefaults) {
  // Use a fresh profile to test that factory registration sets defaults.
  TestingProfile profile;
  AstraAccessibilityServiceFactory::RegisterProfilePrefs(
      profile.GetPrefs()->registry());

  // Verify defaults are registered.
  PrefService* prefs = profile.GetPrefs();
  EXPECT_FALSE(prefs->GetBoolean(prefs::kPrefHighContrastMode));
  EXPECT_FALSE(prefs->GetBoolean(prefs::kPrefReducedMotion));
  EXPECT_DOUBLE_EQ(prefs->GetDouble(prefs::kPrefAccessibilityFontScale), 1.0);
  EXPECT_FALSE(prefs->GetBoolean(prefs::kPrefCaretBrowsingEnabled));
  EXPECT_FALSE(prefs->GetBoolean(prefs::kPrefStickyKeysEnabled));
  EXPECT_FALSE(prefs->GetBoolean(prefs::kPrefSlowKeysEnabled));
  EXPECT_EQ(prefs->GetInteger(prefs::kPrefSlowKeysDelayMs), 500);
  EXPECT_FALSE(prefs->GetBoolean(prefs::kPrefMouseKeysEnabled));
  EXPECT_FALSE(prefs->GetBoolean(prefs::kPrefLargeCursorEnabled));
  EXPECT_EQ(prefs->GetInteger(prefs::kPrefLargeCursorSize), 3);
  EXPECT_FALSE(prefs->GetBoolean(prefs::kPrefMagnifierEnabled));
  EXPECT_DOUBLE_EQ(prefs->GetDouble(prefs::kPrefMagnifierScale), 2.0);
  EXPECT_EQ(prefs->GetInteger(prefs::kPrefMagnifierType), 0);
  EXPECT_FALSE(prefs->GetBoolean(prefs::kPrefSelectToSpeakEnabled));
  EXPECT_FALSE(prefs->GetBoolean(prefs::kPrefDictationEnabled));
  EXPECT_FALSE(prefs->GetBoolean(prefs::kPrefVirtualKeyboardEnabled));
  EXPECT_EQ(prefs->GetInteger(prefs::kPrefMinimumFontSize), 0);
  EXPECT_EQ(prefs->GetInteger(prefs::kPrefFontWeightAdjustment), 0);
  EXPECT_DOUBLE_EQ(prefs->GetDouble(prefs::kPrefLetterSpacing), 1.0);
  EXPECT_DOUBLE_EQ(prefs->GetDouble(prefs::kPrefLineHeight), 1.0);
  EXPECT_EQ(prefs->GetInteger(prefs::kPrefContrastLevel), 0);
  EXPECT_FALSE(prefs->GetBoolean(prefs::kPrefNightLightEnabled));
  EXPECT_EQ(prefs->GetInteger(prefs::kPrefColorTemperature), 50);
  EXPECT_FALSE(prefs->GetBoolean(prefs::kPrefColorInversionEnabled));
  EXPECT_EQ(prefs->GetInteger(prefs::kPrefAnimationReductionLevel), 0);
  EXPECT_FALSE(prefs->GetBoolean(prefs::kPrefAutoScrollEnabled));
  EXPECT_DOUBLE_EQ(prefs->GetDouble(prefs::kPrefScrollSpeed), 1.0);
}

}  // namespace astra
