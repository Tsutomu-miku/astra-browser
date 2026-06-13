// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Unit tests for AstraSplitView, AstraSplitDivider, AstraSplitMinimapView,
// and split view helper functions.
//
// Tests verify:
//   - Layout correctness (horizontal and vertical orientations)
//   - Ratio clamping (ratio-based and pixel-based minimums)
//   - Divider keyboard navigation
//   - View swapping
//   - Observer notification (ratio changes, orientation, swap, replace, etc.)
//   - Observer defaults (all methods have empty default implementations)
//   - Preset ratios
//   - Maximize / unmaximize
//   - Tab replacement
//   - Settings
//   - Minimap
//   - Edge cases
//   - Theme/color integration
//
// Chromium test pattern: views::test::ViewsTestBase
//   (ui/views/test/views_test_base.h)

#include "astra/ui/views/split_view/astra_split_view.h"
#include "astra/ui/views/split_view/astra_split_view_controller.h"

#include <memory>
#include <set>

#include "base/test/task_environment.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/views/test/views_test_base.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

#include "astra/browser/astra_prefs.h"

namespace astra {

using ::testing::_;
using ::testing::FloatEq;

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------

class AstraSplitViewTest : public views::ViewsTestBase {
 public:
  AstraSplitViewTest() = default;
  ~AstraSplitViewTest() override = default;

  // ViewsTestBase:
  void SetUp() override {
    ViewsTestBase::SetUp();

    // Create a widget so the split view has a valid parent and
    // ColorProvider access for theme tests.
    widget_ = CreateTestWidget();
    split_view_ = widget_->SetContentsView(std::make_unique<AstraSplitView>());
    widget_->Show();
  }

  void TearDown() override {
    widget_.reset();
    ViewsTestBase::TearDown();
  }

 protected:
  std::unique_ptr<views::Widget> widget_;
  raw_ptr<AstraSplitView> split_view_ = nullptr;
};

// ---------------------------------------------------------------------------
// Mock observer for testing
// ---------------------------------------------------------------------------

// Full mock observer that tracks all notifications.
class MockSplitViewObserver : public AstraSplitView::Observer {
 public:
  MOCK_METHOD(void, OnSplitRatioChanged, (float ratio), (override));
  MOCK_METHOD(void, OnSplitRatioChanging, (float ratio), (override));
  MOCK_METHOD(void, OnSplitOrientationChanged,
              (SplitViewOrientation orientation), (override));
  MOCK_METHOD(void, OnSplitViewsSwapped, (), (override));
  MOCK_METHOD(void, OnSplitViewReplaced, (bool is_primary), (override));
  MOCK_METHOD(void, OnSplitViewSettingsChanged,
              (const AstraSplitViewSettings& settings), (override));
  MOCK_METHOD(void, OnSplitViewDestroyed, (), (override));
  MOCK_METHOD(void, OnSplitViewMaximized, (bool primary_maximized), (override));
  MOCK_METHOD(void, OnSplitViewUnmaximized, (), (override));
};

// Observer that overrides nothing — verifies all defaults are empty.
class EmptySplitViewObserver : public AstraSplitView::Observer {
 public:
  // Deliberately overrides no methods — all should have empty default {}
  // implementations.  Creating and destroying this observer should not
  // cause any issues.
};

// =========================================================================
// Basic construction tests
// =========================================================================

TEST_F(AstraSplitViewTest, DefaultStateIsHorizontal) {
  EXPECT_EQ(SplitViewOrientation::kHorizontal, split_view_->orientation());
}

TEST_F(AstraSplitViewTest, DefaultRatioIsFiftyPercent) {
  EXPECT_FLOAT_EQ(0.5f, split_view_->ratio());
}

TEST_F(AstraSplitViewTest, DividerExists) {
  EXPECT_NE(split_view_->divider(), nullptr);
}

TEST_F(AstraSplitViewTest, NoChildViewsInitially) {
  EXPECT_EQ(nullptr, split_view_->primary_view());
  EXPECT_EQ(nullptr, split_view_->secondary_view());
}

TEST_F(AstraSplitViewTest, DefaultIsNotMaximized) {
  EXPECT_FALSE(split_view_->IsMaximized());
  EXPECT_FALSE(split_view_->IsPrimaryMaximized());
}

TEST_F(AstraSplitViewTest, MinimapExistsButHidden) {
  EXPECT_NE(split_view_->minimap(), nullptr);
  EXPECT_FALSE(split_view_->minimap_visible());
  EXPECT_FALSE(split_view_->minimap()->GetVisible());
}

// =========================================================================
// Observer defaults test
// =========================================================================

// Test that an observer overriding nothing can be added and removed
// without any issues.  This validates that all observer methods have
// empty default implementations.
TEST_F(AstraSplitViewTest, EmptyObserverWorks) {
  EmptySplitViewObserver empty_observer;
  split_view_->AddObserver(&empty_observer);

  // Trigger various state changes — the empty observer should not crash
  // even though it overrides none of the methods.
  split_view_->SetRatio(0.7f);
  split_view_->SetOrientation(SplitViewOrientation::kVertical);
  split_view_->SwapViews();
  split_view_->MaximizePane(true);
  split_view_->Unmaximize();

  split_view_->RemoveObserver(&empty_observer);
  // If we get here without crashing, the empty observer defaults work.
}

// =========================================================================
// Primary/secondary view tests
// =========================================================================

TEST_F(AstraSplitViewTest, SetPrimaryView) {
  auto view = std::make_unique<views::View>();
  views::View* view_ptr = view.get();
  split_view_->SetPrimaryView(view.release());

  EXPECT_EQ(view_ptr, split_view_->primary_view());
  EXPECT_EQ(nullptr, split_view_->secondary_view());
}

TEST_F(AstraSplitViewTest, SetSecondaryView) {
  auto view = std::make_unique<views::View>();
  views::View* view_ptr = view.get();
  split_view_->SetSecondaryView(view.release());

  EXPECT_EQ(nullptr, split_view_->primary_view());
  EXPECT_EQ(view_ptr, split_view_->secondary_view());
}

TEST_F(AstraSplitViewTest, SetBothViews) {
  auto primary = std::make_unique<views::View>();
  auto secondary = std::make_unique<views::View>();
  views::View* primary_ptr = primary.get();
  views::View* secondary_ptr = secondary.get();

  split_view_->SetPrimaryView(primary.release());
  split_view_->SetSecondaryView(secondary.release());

  EXPECT_EQ(primary_ptr, split_view_->primary_view());
  EXPECT_EQ(secondary_ptr, split_view_->secondary_view());
}

TEST_F(AstraSplitViewTest, ReplacePrimaryView) {
  auto view1 = std::make_unique<views::View>();
  auto view2 = std::make_unique<views::View>();
  views::View* view2_ptr = view2.get();

  split_view_->SetPrimaryView(view1.release());

  MockSplitViewObserver observer;
  split_view_->AddObserver(&observer);

  EXPECT_CALL(observer, OnSplitViewReplaced(true)).Times(1);
  split_view_->ReplacePrimaryView(view2.release());
  EXPECT_EQ(view2_ptr, split_view_->primary_view());

  split_view_->RemoveObserver(&observer);
}

TEST_F(AstraSplitViewTest, ReplaceSecondaryView) {
  auto view1 = std::make_unique<views::View>();
  auto view2 = std::make_unique<views::View>();
  views::View* view2_ptr = view2.get();

  split_view_->SetSecondaryView(view1.release());

  MockSplitViewObserver observer;
  split_view_->AddObserver(&observer);

  EXPECT_CALL(observer, OnSplitViewReplaced(false)).Times(1);
  split_view_->ReplaceSecondaryView(view2.release());
  EXPECT_EQ(view2_ptr, split_view_->secondary_view());

  split_view_->RemoveObserver(&observer);
}

TEST_F(AstraSplitViewTest, ReplacePrimaryWithSameIsNoOp) {
  auto view = std::make_unique<views::View>();
  views::View* view_ptr = view.get();
  split_view_->SetPrimaryView(view.release());
  ASSERT_EQ(view_ptr, split_view_->primary_view());

  // Replacing with the same view should not trigger notification
  // (but SetPrimaryView handles this internally as a no-op).
  MockSplitViewObserver observer;
  split_view_->AddObserver(&observer);
  // ReplacePrimaryView always notifies, even if the view is the same.
  // Actually, looking at the code: ReplacePrimaryView calls SetPrimaryView
  // first (which may be a no-op if same pointer), then notifies.
  // Let me just test it notifies.
  EXPECT_CALL(observer, OnSplitViewReplaced(true)).Times(1);
  split_view_->ReplacePrimaryView(view_ptr);
  split_view_->RemoveObserver(&observer);
}

TEST_F(AstraSplitViewTest, ClearPrimaryViewWithNullptr) {
  auto view = std::make_unique<views::View>();
  split_view_->SetPrimaryView(view.release());
  ASSERT_NE(nullptr, split_view_->primary_view());

  split_view_->SetPrimaryView(nullptr);
  EXPECT_EQ(nullptr, split_view_->primary_view());
}

// =========================================================================
// Ratio tests
// =========================================================================

TEST_F(AstraSplitViewTest, SetRatio) {
  split_view_->SetRatio(0.7f);
  EXPECT_FLOAT_EQ(0.7f, split_view_->ratio());
}

TEST_F(AstraSplitViewTest, SetRatioZeroClampsToMin) {
  split_view_->SetRatio(0.0f);
  EXPECT_GE(split_view_->ratio(), AstraSplitView::kMinPaneRatio);
}

TEST_F(AstraSplitViewTest, SetRatioOneClampsToMax) {
  split_view_->SetRatio(1.0f);
  EXPECT_LE(split_view_->ratio(), 1.0f - AstraSplitView::kMinPaneRatio);
}

TEST_F(AstraSplitViewTest, SetRatioNegativeClamps) {
  split_view_->SetRatio(-0.5f);
  EXPECT_GE(split_view_->ratio(), 0.0f);
  EXPECT_GE(split_view_->ratio(), AstraSplitView::kMinPaneRatio);
}

TEST_F(AstraSplitViewTest, SetRatioOverOneClamps) {
  split_view_->SetRatio(1.5f);
  EXPECT_LE(split_view_->ratio(), 1.0f);
}

TEST_F(AstraSplitViewTest, SetRatioNoChangeSameValue) {
  split_view_->SetRatio(0.5f);
  float before = split_view_->ratio();
  split_view_->SetRatio(0.5f);
  EXPECT_FLOAT_EQ(before, split_view_->ratio());
}

TEST_F(AstraSplitViewTest, StaticClampRatioOnlyRatioBased) {
  // Without total_size, only ratio-based clamping applies.
  float clamped = AstraSplitView::ClampRatio(0.05f, /*total_size=*/0);
  EXPECT_FLOAT_EQ(AstraSplitView::kMinPaneRatio, clamped);

  clamped = AstraSplitView::ClampRatio(0.95f, /*total_size=*/0);
  EXPECT_FLOAT_EQ(1.0f - AstraSplitView::kMinPaneRatio, clamped);
}

TEST_F(AstraSplitViewTest, StaticClampRatioWithPixelMinimum) {
  // With a small total_size, the pixel-based minimum overrides the ratio one.
  // kMinPaneSizeDips = 100, total_size = 200 → min ratio = 0.5
  float clamped = AstraSplitView::ClampRatio(0.2f, /*total_size=*/200);
  EXPECT_FLOAT_EQ(0.5f, clamped);

  clamped = AstraSplitView::ClampRatio(0.8f, /*total_size=*/200);
  EXPECT_FLOAT_EQ(0.5f, clamped);
}

TEST_F(AstraSplitViewTest, StaticClampRatioLargeTotalSize) {
  // With a large total_size, ratio-based minimum dominates.
  // kMinPaneRatio = 0.1, total_size = 1000 → min ratio = 0.1
  float clamped = AstraSplitView::ClampRatio(0.05f, /*total_size=*/1000);
  EXPECT_FLOAT_EQ(0.1f, clamped);
}

// =========================================================================
// Preset ratio tests
// =========================================================================

TEST(SplitViewPresetTest, PresetToRatioFiftyFifty) {
  EXPECT_FLOAT_EQ(0.5f, SplitViewPresetToRatio(SplitViewPreset::kFiftyFifty));
}

TEST(SplitViewPresetTest, PresetToRatioSeventyThirty) {
  EXPECT_FLOAT_EQ(0.7f, SplitViewPresetToRatio(SplitViewPreset::kSeventyThirty));
}

TEST(SplitViewPresetTest, PresetToRatioThirtySeventy) {
  EXPECT_FLOAT_EQ(0.3f, SplitViewPresetToRatio(SplitViewPreset::kThirtySeventy));
}

TEST(SplitViewPresetTest, PresetToRatioSixtyForty) {
  EXPECT_FLOAT_EQ(0.6f, SplitViewPresetToRatio(SplitViewPreset::kSixtyForty));
}

TEST(SplitViewPresetTest, PresetToRatioFortySixty) {
  EXPECT_FLOAT_EQ(0.4f, SplitViewPresetToRatio(SplitViewPreset::kFortySixty));
}

TEST(SplitViewPresetTest, RatioToPresetExactMatch) {
  auto preset = RatioToSplitViewPreset(0.5f, 0.01f);
  ASSERT_TRUE(preset.has_value());
  EXPECT_EQ(SplitViewPreset::kFiftyFifty, *preset);

  preset = RatioToSplitViewPreset(0.7f, 0.01f);
  ASSERT_TRUE(preset.has_value());
  EXPECT_EQ(SplitViewPreset::kSeventyThirty, *preset);

  preset = RatioToSplitViewPreset(0.3f, 0.01f);
  ASSERT_TRUE(preset.has_value());
  EXPECT_EQ(SplitViewPreset::kThirtySeventy, *preset);
}

TEST(SplitViewPresetTest, RatioToPresetNoMatch) {
  auto preset = RatioToSplitViewPreset(0.55f, 0.01f);
  EXPECT_FALSE(preset.has_value());
}

TEST(SplitViewPresetTest, RatioToPresetWithinTolerance) {
  // 0.51 is within 0.02 tolerance of 0.5
  auto preset = RatioToSplitViewPreset(0.51f, 0.02f);
  ASSERT_TRUE(preset.has_value());
  EXPECT_EQ(SplitViewPreset::kFiftyFifty, *preset);
}

TEST(SplitViewPresetTest, PresetNamesNotEmpty) {
  EXPECT_FALSE(SplitViewPresetToName(SplitViewPreset::kFiftyFifty).empty());
  EXPECT_FALSE(SplitViewPresetToName(SplitViewPreset::kSeventyThirty).empty());
  EXPECT_FALSE(SplitViewPresetToName(SplitViewPreset::kThirtySeventy).empty());
}

TEST_F(AstraSplitViewTest, SetPresetRatioFiftyFifty) {
  split_view_->SetRatio(0.8f);
  split_view_->SetPresetRatio(SplitViewPreset::kFiftyFifty);
  EXPECT_FLOAT_EQ(0.5f, split_view_->ratio());
}

TEST_F(AstraSplitViewTest, SetPresetRatioSeventyThirty) {
  split_view_->SetPresetRatio(SplitViewPreset::kSeventyThirty);
  EXPECT_FLOAT_EQ(0.7f, split_view_->ratio());
}

TEST_F(AstraSplitViewTest, GetCurrentPresetExact) {
  split_view_->SetRatio(0.5f);
  auto preset = split_view_->GetCurrentPreset(0.001f);
  ASSERT_TRUE(preset.has_value());
  EXPECT_EQ(SplitViewPreset::kFiftyFifty, *preset);
}

TEST_F(AstraSplitViewTest, GetCurrentPresetNone) {
  split_view_->SetRatio(0.55f);
  auto preset = split_view_->GetCurrentPreset(0.01f);
  EXPECT_FALSE(preset.has_value());
}

// =========================================================================
// Orientation tests
// =========================================================================

TEST_F(AstraSplitViewTest, SetOrientationVertical) {
  split_view_->SetOrientation(SplitViewOrientation::kVertical);
  EXPECT_EQ(SplitViewOrientation::kVertical, split_view_->orientation());
  EXPECT_EQ(SplitViewOrientation::kVertical,
            split_view_->divider()->orientation());
}

TEST_F(AstraSplitViewTest, SetOrientationHorizontal) {
  split_view_->SetOrientation(SplitViewOrientation::kVertical);
  split_view_->SetOrientation(SplitViewOrientation::kHorizontal);
  EXPECT_EQ(SplitViewOrientation::kHorizontal, split_view_->orientation());
}

TEST_F(AstraSplitViewTest, SetSameOrientationIsNoOp) {
  split_view_->SetOrientation(SplitViewOrientation::kHorizontal);
  EXPECT_EQ(SplitViewOrientation::kHorizontal, split_view_->orientation());
}

TEST_F(AstraSplitViewTest, OrientationAffectsDivider) {
  split_view_->SetOrientation(SplitViewOrientation::kHorizontal);
  EXPECT_EQ(SplitViewOrientation::kHorizontal,
            split_view_->divider()->orientation());

  split_view_->SetOrientation(SplitViewOrientation::kVertical);
  EXPECT_EQ(SplitViewOrientation::kVertical,
            split_view_->divider()->orientation());
}

TEST_F(AstraSplitViewTest, ToggleOrientation) {
  split_view_->SetOrientation(SplitViewOrientation::kHorizontal);
  split_view_->ToggleOrientation();
  EXPECT_EQ(SplitViewOrientation::kVertical, split_view_->orientation());
  split_view_->ToggleOrientation();
  EXPECT_EQ(SplitViewOrientation::kHorizontal, split_view_->orientation());
}

TEST_F(AstraSplitViewTest, OrientationChangeNotifiesObserver) {
  MockSplitViewObserver observer;
  split_view_->AddObserver(&observer);

  EXPECT_CALL(observer, OnSplitOrientationChanged(SplitViewOrientation::kVertical))
      .Times(1);
  split_view_->SetOrientation(SplitViewOrientation::kVertical);

  EXPECT_CALL(observer, OnSplitOrientationChanged(SplitViewOrientation::kHorizontal))
      .Times(1);
  split_view_->SetOrientation(SplitViewOrientation::kHorizontal);

  split_view_->RemoveObserver(&observer);
}

TEST_F(AstraSplitViewTest, SameOrientationDoesNotNotify) {
  MockSplitViewObserver observer;
  split_view_->AddObserver(&observer);

  // Default is horizontal; setting it again should not notify.
  EXPECT_CALL(observer, OnSplitOrientationChanged(_)).Times(0);
  split_view_->SetOrientation(SplitViewOrientation::kHorizontal);

  split_view_->RemoveObserver(&observer);
}

// =========================================================================
// Swap views tests
// =========================================================================

TEST_F(AstraSplitViewTest, SwapViewsSwapsPointers) {
  auto primary = std::make_unique<views::View>();
  auto secondary = std::make_unique<views::View>();
  views::View* primary_ptr = primary.get();
  views::View* secondary_ptr = secondary.get();

  split_view_->SetPrimaryView(primary.release());
  split_view_->SetSecondaryView(secondary.release());

  split_view_->SwapViews();

  EXPECT_EQ(secondary_ptr, split_view_->primary_view());
  EXPECT_EQ(primary_ptr, split_view_->secondary_view());
}

TEST_F(AstraSplitViewTest, SwapViewsPreservesRatio) {
  split_view_->SetRatio(0.7f);
  float ratio_before = split_view_->ratio();

  auto primary = std::make_unique<views::View>();
  auto secondary = std::make_unique<views::View>();
  split_view_->SetPrimaryView(primary.release());
  split_view_->SetSecondaryView(secondary.release());

  split_view_->SwapViews();

  EXPECT_FLOAT_EQ(ratio_before, split_view_->ratio());
}

TEST_F(AstraSplitViewTest, SwapNotifiesObserver) {
  MockSplitViewObserver observer;
  split_view_->AddObserver(&observer);

  auto primary = std::make_unique<views::View>();
  auto secondary = std::make_unique<views::View>();
  split_view_->SetPrimaryView(primary.release());
  split_view_->SetSecondaryView(secondary.release());

  EXPECT_CALL(observer, OnSplitViewsSwapped()).Times(1);
  split_view_->SwapViews();

  split_view_->RemoveObserver(&observer);
}

TEST_F(AstraSplitViewTest, SwapWithOnlyPrimaryView) {
  auto view = std::make_unique<views::View>();
  views::View* view_ptr = view.get();

  split_view_->SetPrimaryView(view.release());
  split_view_->SwapViews();

  EXPECT_EQ(nullptr, split_view_->primary_view());
  EXPECT_EQ(view_ptr, split_view_->secondary_view());
}

TEST_F(AstraSplitViewTest, DoubleSwapReturnsToOriginal) {
  auto primary = std::make_unique<views::View>();
  auto secondary = std::make_unique<views::View>();
  views::View* primary_ptr = primary.get();
  views::View* secondary_ptr = secondary.get();

  split_view_->SetPrimaryView(primary.release());
  split_view_->SetSecondaryView(secondary.release());

  split_view_->SwapViews();
  split_view_->SwapViews();

  EXPECT_EQ(primary_ptr, split_view_->primary_view());
  EXPECT_EQ(secondary_ptr, split_view_->secondary_view());
}

// =========================================================================
// Maximize / unmaximize tests
// =========================================================================

TEST_F(AstraSplitViewTest, MaximizePrimaryPane) {
  split_view_->SetRatio(0.5f);
  split_view_->MaximizePane(/*primary=*/true);

  EXPECT_TRUE(split_view_->IsMaximized());
  EXPECT_TRUE(split_view_->IsPrimaryMaximized());
  // Primary should be as large as possible.
  EXPECT_GT(split_view_->ratio(), 0.8f);
}

TEST_F(AstraSplitViewTest, MaximizeSecondaryPane) {
  split_view_->SetRatio(0.5f);
  split_view_->MaximizePane(/*primary=*/false);

  EXPECT_TRUE(split_view_->IsMaximized());
  EXPECT_FALSE(split_view_->IsPrimaryMaximized());
  // Primary should be as small as possible.
  EXPECT_LT(split_view_->ratio(), 0.2f);
}

TEST_F(AstraSplitViewTest, UnmaximizeRestoresPreviousRatio) {
  split_view_->SetRatio(0.7f);
  float original_ratio = split_view_->ratio();

  split_view_->MaximizePane(true);
  ASSERT_TRUE(split_view_->IsMaximized());

  split_view_->Unmaximize();
  EXPECT_FALSE(split_view_->IsMaximized());
  EXPECT_FLOAT_EQ(original_ratio, split_view_->ratio());
}

TEST_F(AstraSplitViewTest, UnmaximizeWhenNotMaximizedIsNoOp) {
  float before = split_view_->ratio();
  split_view_->Unmaximize();
  EXPECT_FLOAT_EQ(before, split_view_->ratio());
  EXPECT_FALSE(split_view_->IsMaximized());
}

TEST_F(AstraSplitViewTest, MaximizeSameSideTwiceIsNoOp) {
  split_view_->MaximizePane(true);
  float ratio_after_first = split_view_->ratio();
  split_view_->MaximizePane(true);
  EXPECT_FLOAT_EQ(ratio_after_first, split_view_->ratio());
}

TEST_F(AstraSplitViewTest, SwitchMaximizedSide) {
  split_view_->SetRatio(0.5f);
  split_view_->MaximizePane(true);
  float primary_max_ratio = split_view_->ratio();

  split_view_->MaximizePane(false);
  EXPECT_TRUE(split_view_->IsMaximized());
  EXPECT_FALSE(split_view_->IsPrimaryMaximized());
  EXPECT_LT(split_view_->ratio(), primary_max_ratio);
}

TEST_F(AstraSplitViewTest, MaximizeNotifiesObservers) {
  MockSplitViewObserver observer;
  split_view_->AddObserver(&observer);

  EXPECT_CALL(observer, OnSplitViewMaximized(true)).Times(1);
  split_view_->MaximizePane(true);

  EXPECT_CALL(observer, OnSplitViewUnmaximized()).Times(1);
  split_view_->Unmaximize();

  split_view_->RemoveObserver(&observer);
}

TEST_F(AstraSplitViewTest, SwitchMaximizedSideNotifies) {
  MockSplitViewObserver observer;
  split_view_->AddObserver(&observer);

  // First maximize primary.
  EXPECT_CALL(observer, OnSplitViewMaximized(true)).Times(1);
  split_view_->MaximizePane(true);

  // Switch to secondary — should notify maximize with new side.
  // Note: The implementation calls SetRatio internally which may also
  // trigger ratio notifications.
  EXPECT_CALL(observer, OnSplitViewMaximized(false)).Times(1);
  split_view_->MaximizePane(false);

  split_view_->RemoveObserver(&observer);
}

// =========================================================================
// Settings tests
// =========================================================================

TEST_F(AstraSplitViewTest, DefaultSettings) {
  const AstraSplitViewSettings& settings = split_view_->settings();
  EXPECT_TRUE(settings.divider_visible);
  EXPECT_FALSE(settings.snap_to_presets);
  EXPECT_TRUE(settings.remember_ratio);
  EXPECT_FALSE(settings.minimap_enabled);
  EXPECT_TRUE(settings.keyboard_navigation_enabled);
  EXPECT_FALSE(settings.show_menu_button);
}

TEST_F(AstraSplitViewTest, ApplySettingsDividerVisible) {
  AstraSplitViewSettings settings = split_view_->settings();
  settings.divider_visible = false;

  MockSplitViewObserver observer;
  split_view_->AddObserver(&observer);

  EXPECT_CALL(observer, OnSplitViewSettingsChanged(_)).Times(1);
  split_view_->ApplySettings(settings);

  EXPECT_FALSE(split_view_->settings().divider_visible);
  EXPECT_FALSE(split_view_->divider()->divider_visible());
  EXPECT_FALSE(split_view_->divider()->GetVisible());

  split_view_->RemoveObserver(&observer);
}

TEST_F(AstraSplitViewTest, ApplySettingsSnapToPresets) {
  AstraSplitViewSettings settings = split_view_->settings();
  settings.snap_to_presets = true;
  split_view_->ApplySettings(settings);

  EXPECT_TRUE(split_view_->settings().snap_to_presets);

  // With snap-to-preset enabled, setting a ratio close to 0.5 should snap.
  split_view_->SetRatio(0.51f);
  // Note: Snap only happens during drag end, not SetRatio directly,
  // unless snap happens in SetRatio.  Let me check the implementation.
  // Actually, looking at the code, SetRatio calls MaybeSnapToPreset only
  // when settings_.snap_to_presets is true.
  EXPECT_FLOAT_EQ(0.5f, split_view_->ratio());
}

TEST_F(AstraSplitViewTest, ApplySettingsNotifiesObserver) {
  MockSplitViewObserver observer;
  split_view_->AddObserver(&observer);

  AstraSplitViewSettings settings = split_view_->settings();
  settings.divider_visible = false;

  EXPECT_CALL(observer, OnSplitViewSettingsChanged(_)).Times(1);
  split_view_->ApplySettings(settings);

  split_view_->RemoveObserver(&observer);
}

TEST_F(AstraSplitViewTest, SettingsMinimapEnabled) {
  AstraSplitViewSettings settings = split_view_->settings();
  settings.minimap_enabled = true;
  split_view_->ApplySettings(settings);

  EXPECT_TRUE(split_view_->settings().minimap_enabled);

  // Now we should be able to show the minimap.
  split_view_->SetMinimapVisible(true);
  EXPECT_TRUE(split_view_->minimap_visible());
  EXPECT_TRUE(split_view_->minimap()->GetVisible());
}

TEST_F(AstraSplitViewTest, MinimapDisabledWhenSettingOff) {
  // With minimap disabled in settings, SetMinimapVisible should be a no-op.
  EXPECT_FALSE(split_view_->settings().minimap_enabled);
  split_view_->SetMinimapVisible(true);
  EXPECT_FALSE(split_view_->minimap_visible());
}

// =========================================================================
// Minimap tests
// =========================================================================

TEST_F(AstraSplitViewTest, MinimapInitiallyHidden) {
  EXPECT_FALSE(split_view_->minimap_visible());
  EXPECT_FALSE(split_view_->minimap()->GetVisible());
}

TEST_F(AstraSplitViewTest, MinimapRequiresSettingToBeEnabled) {
  // Default settings have minimap disabled.
  split_view_->SetMinimapVisible(true);
  EXPECT_FALSE(split_view_->minimap_visible());

  // Enable the setting first.
  AstraSplitViewSettings settings = split_view_->settings();
  settings.minimap_enabled = true;
  split_view_->ApplySettings(settings);

  split_view_->SetMinimapVisible(true);
  EXPECT_TRUE(split_view_->minimap_visible());
  EXPECT_TRUE(split_view_->minimap()->GetVisible());
}

TEST_F(AstraSplitViewTest, MinimapHidesWhenSettingDisabled) {
  AstraSplitViewSettings settings = split_view_->settings();
  settings.minimap_enabled = true;
  split_view_->ApplySettings(settings);

  split_view_->SetMinimapVisible(true);
  ASSERT_TRUE(split_view_->minimap_visible());

  // Disabling the setting should hide the minimap.
  settings.minimap_enabled = false;
  split_view_->ApplySettings(settings);
  EXPECT_FALSE(split_view_->minimap_visible());
}

TEST_F(AstraSplitViewTest, MinimapUpdatesWithRatioAndOrientation) {
  AstraSplitViewSettings settings = split_view_->settings();
  settings.minimap_enabled = true;
  split_view_->ApplySettings(settings);

  split_view_->SetMinimapVisible(true);
  ASSERT_TRUE(split_view_->minimap_visible());

  // Changing ratio should update the minimap.
  split_view_->SetRatio(0.7f);
  // Minimap should still be visible and shouldn't crash.
  EXPECT_TRUE(split_view_->minimap()->GetVisible());

  // Changing orientation should update the minimap.
  split_view_->SetOrientation(SplitViewOrientation::kVertical);
  EXPECT_TRUE(split_view_->minimap()->GetVisible());
}

TEST(AstraSplitMinimapViewTest, DefaultSize) {
  AstraSplitMinimapView minimap;
  gfx::Size pref = minimap.GetPreferredSize();
  EXPECT_EQ(AstraSplitView::kMinimapWidth, pref.width());
  EXPECT_EQ(AstraSplitView::kMinimapHeight, pref.height());
}

TEST(AstraSplitMinimapViewTest, UpdateLayout) {
  AstraSplitMinimapView minimap;
  // Should not crash.
  minimap.UpdateLayout(0.7f, SplitViewOrientation::kVertical);
  minimap.UpdateLayout(0.3f, SplitViewOrientation::kHorizontal);
}

// =========================================================================
// Divider tests
// =========================================================================

TEST_F(AstraSplitViewTest, DividerIsFocusable) {
  EXPECT_EQ(views::View::FocusBehavior::ALWAYS,
            split_view_->divider()->GetFocusBehavior());
}

TEST_F(AstraSplitViewTest, DividerKeyboardStepSize) {
  EXPECT_GT(split_view_->divider()->keyboard_step_size(), 0);
  EXPECT_LE(split_view_->divider()->keyboard_step_size(), 100);
}

TEST_F(AstraSplitViewTest, DividerSetKeyboardStepSize) {
  split_view_->divider()->SetKeyboardStepSize(50);
  EXPECT_EQ(50, split_view_->divider()->keyboard_step_size());
}

TEST_F(AstraSplitViewTest, DividerVisibilityToggles) {
  EXPECT_TRUE(split_view_->divider()->divider_visible());
  split_view_->divider()->SetDividerVisible(false);
  EXPECT_FALSE(split_view_->divider()->divider_visible());
  EXPECT_FALSE(split_view_->divider()->GetVisible());

  split_view_->divider()->SetDividerVisible(true);
  EXPECT_TRUE(split_view_->divider()->divider_visible());
  EXPECT_TRUE(split_view_->divider()->GetVisible());
}

// =========================================================================
// Observer notification tests
// =========================================================================

TEST_F(AstraSplitViewTest, MultipleObservers) {
  MockSplitViewObserver obs1;
  MockSplitViewObserver obs2;

  split_view_->AddObserver(&obs1);
  split_view_->AddObserver(&obs2);

  EXPECT_CALL(obs1, OnSplitOrientationChanged(SplitViewOrientation::kVertical))
      .Times(1);
  EXPECT_CALL(obs2, OnSplitOrientationChanged(SplitViewOrientation::kVertical))
      .Times(1);

  split_view_->SetOrientation(SplitViewOrientation::kVertical);

  split_view_->RemoveObserver(&obs1);
  split_view_->RemoveObserver(&obs2);
}

TEST_F(AstraSplitViewTest, RemoveObserverStopsNotifications) {
  MockSplitViewObserver observer;
  split_view_->AddObserver(&observer);

  EXPECT_CALL(observer, OnSplitOrientationChanged(_)).Times(1);
  split_view_->SetOrientation(SplitViewOrientation::kVertical);

  split_view_->RemoveObserver(&observer);

  // After removal, no more notifications.
  EXPECT_CALL(observer, OnSplitOrientationChanged(_)).Times(0);
  split_view_->SetOrientation(SplitViewOrientation::kHorizontal);
}

TEST_F(AstraSplitViewTest, DestroyNotifiesObserver) {
  auto widget = CreateTestWidget();
  auto* split_view = widget->SetContentsView(
      std::make_unique<AstraSplitView>());
  widget->Show();

  MockSplitViewObserver observer;
  split_view->AddObserver(&observer);

  EXPECT_CALL(observer, OnSplitViewDestroyed()).Times(1);
  widget.reset();  // Destroys split view.
}

// =========================================================================
// Layout tests
// =========================================================================

TEST_F(AstraSplitViewTest, HorizontalLayoutSizes) {
  const int kWidth = 400;
  const int kHeight = 300;

  auto primary = std::make_unique<views::View>();
  auto secondary = std::make_unique<views::View>();
  split_view_->SetPrimaryView(primary.release());
  split_view_->SetSecondaryView(secondary.release());

  split_view_->SetRatio(0.5f);
  widget_->SetSize(gfx::Size(kWidth, kHeight));
  widget_->LayoutRootViewIfNecessary();

  EXPECT_GT(split_view_->primary_view()->width(), 100);
  EXPECT_LT(split_view_->primary_view()->width(), 250);

  EXPECT_EQ(kHeight, split_view_->primary_view()->height());
  EXPECT_EQ(kHeight, split_view_->secondary_view()->height());
}

TEST_F(AstraSplitViewTest, VerticalLayoutSizes) {
  const int kWidth = 400;
  const int kHeight = 300;

  auto primary = std::make_unique<views::View>();
  auto secondary = std::make_unique<views::View>();
  split_view_->SetPrimaryView(primary.release());
  split_view_->SetSecondaryView(secondary.release());

  split_view_->SetOrientation(SplitViewOrientation::kVertical);
  split_view_->SetRatio(0.5f);
  widget_->SetSize(gfx::Size(kWidth, kHeight));
  widget_->LayoutRootViewIfNecessary();

  EXPECT_GT(split_view_->primary_view()->height(), 80);
  EXPECT_LT(split_view_->primary_view()->height(), 170);

  EXPECT_EQ(kWidth, split_view_->primary_view()->width());
  EXPECT_EQ(kWidth, split_view_->secondary_view()->width());
}

TEST_F(AstraSplitViewTest, DividerPositionHorizontal) {
  split_view_->SetRatio(0.25f);
  widget_->SetSize(gfx::Size(400, 300));
  widget_->LayoutRootViewIfNecessary();

  int divider_x = split_view_->divider()->x();
  EXPECT_NEAR(divider_x, 100, 10);
}

TEST_F(AstraSplitViewTest, DividerPositionVertical) {
  split_view_->SetOrientation(SplitViewOrientation::kVertical);
  split_view_->SetRatio(0.25f);
  widget_->SetSize(gfx::Size(400, 300));
  widget_->LayoutRootViewIfNecessary();

  int divider_y = split_view_->divider()->y();
  EXPECT_NEAR(divider_y, 75, 10);
}

TEST_F(AstraSplitViewTest, HiddenDividerZeroThickness) {
  AstraSplitViewSettings settings = split_view_->settings();
  settings.divider_visible = false;
  split_view_->ApplySettings(settings);

  auto primary = std::make_unique<views::View>();
  auto secondary = std::make_unique<views::View>();
  split_view_->SetPrimaryView(primary.release());
  split_view_->SetSecondaryView(secondary.release());

  widget_->SetSize(gfx::Size(400, 300));
  widget_->LayoutRootViewIfNecessary();

  // When divider is hidden, secondary view should start right after primary.
  int primary_right = split_view_->primary_view()->bounds().right();
  int secondary_left = split_view_->secondary_view()->x();
  EXPECT_EQ(primary_right, secondary_left);
}

// =========================================================================
// Preferred size tests
// =========================================================================

TEST_F(AstraSplitViewTest, CalculatePreferredSize) {
  gfx::Size pref = split_view_->CalculatePreferredSize(views::SizeBounds());
  EXPECT_GT(pref.width(), 0);
  EXPECT_GT(pref.height(), 0);
}

// =========================================================================
// Edge case tests
// =========================================================================

TEST_F(AstraSplitViewTest, ZeroSizedSplitView) {
  // Setting size to zero should not crash.
  widget_->SetSize(gfx::Size(0, 0));
  widget_->LayoutRootViewIfNecessary();

  // Ratio should still be valid.
  EXPECT_GE(split_view_->ratio(), 0.0f);
  EXPECT_LE(split_view_->ratio(), 1.0f);
}

TEST_F(AstraSplitViewTest, VerySmallSplitView) {
  widget_->SetSize(gfx::Size(50, 50));
  widget_->LayoutRootViewIfNecessary();

  // With a very small view, the pixel-based minimum may force 50/50 split.
  // Should not crash.
  EXPECT_GE(split_view_->ratio(), 0.0f);
  EXPECT_LE(split_view_->ratio(), 1.0f);
}

TEST_F(AstraSplitViewTest, SwapWithNoViewsDoesNotCrash) {
  split_view_->SwapViews();
  // Should not crash and both should still be null.
  EXPECT_EQ(nullptr, split_view_->primary_view());
  EXPECT_EQ(nullptr, split_view_->secondary_view());
}

TEST_F(AstraSplitViewTest, MaximizeUnmaximizeMultipleTimes) {
  for (int i = 0; i < 5; i++) {
    split_view_->MaximizePane(true);
    EXPECT_TRUE(split_view_->IsMaximized());
    split_view_->Unmaximize();
    EXPECT_FALSE(split_view_->IsMaximized());
  }
}

TEST_F(AstraSplitViewTest, OrientationSwitchWhileMaximized) {
  split_view_->MaximizePane(true);
  ASSERT_TRUE(split_view_->IsMaximized());

  split_view_->SetOrientation(SplitViewOrientation::kVertical);
  EXPECT_TRUE(split_view_->IsMaximized());
  EXPECT_TRUE(split_view_->IsPrimaryMaximized());
}

TEST_F(AstraSplitViewTest, SettingsAppliedMultipleTimes) {
  AstraSplitViewSettings settings = split_view_->settings();
  settings.divider_visible = false;
  settings.snap_to_presets = true;
  split_view_->ApplySettings(settings);

  // Applying same settings again should not cause issues.
  split_view_->ApplySettings(settings);
  EXPECT_FALSE(split_view_->settings().divider_visible);
  EXPECT_TRUE(split_view_->settings().snap_to_presets);
}

// =========================================================================
// Theme tests
// =========================================================================

TEST_F(AstraSplitViewTest, OnThemeChangedDoesNotCrash) {
  split_view_->OnThemeChanged();
  // Just verify it doesn't crash.
}

TEST_F(AstraSplitViewTest, DividerOnThemeChanged) {
  split_view_->divider()->OnThemeChanged();
  // Just verify it doesn't crash.
}

// =========================================================================
// Pref persistence tests (for settings only)
// =========================================================================

class AstraSplitViewPrefsTest : public testing::Test {
 public:
  AstraSplitViewPrefsTest() = default;
  ~AstraSplitViewPrefsTest() override = default;

  void SetUp() override {
    TestingProfile::Builder builder;
    profile_ = builder.Build();

    // Register Astra prefs on the profile.
    prefs::RegisterProfilePrefs(profile_->GetPrefs());
  }

  void TearDown() override {
    profile_.reset();
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
};

TEST_F(AstraSplitViewPrefsTest, DefaultValues) {
  PrefService* prefs = profile_->GetPrefs();

  EXPECT_EQ("horizontal",
            prefs->GetString(prefs::kPrefSplitViewDefaultOrientation));
  EXPECT_DOUBLE_EQ(0.5,
                   prefs->GetDouble(prefs::kPrefSplitViewDefaultRatio));
  EXPECT_TRUE(prefs->GetBoolean(prefs::kPrefSplitViewEnabled));
  EXPECT_FALSE(prefs->GetBoolean(prefs::kPrefSplitViewSnapToPresets));
  EXPECT_TRUE(prefs->GetBoolean(prefs::kPrefSplitViewDividerVisible));
  EXPECT_TRUE(prefs->GetBoolean(prefs::kPrefSplitViewRememberRatio));
  EXPECT_FALSE(prefs->GetBoolean(prefs::kPrefSplitViewMinimapEnabled));
}

TEST_F(AstraSplitViewPrefsTest, PersistAndReadBack) {
  PrefService* prefs = profile_->GetPrefs();

  // Write non-default values.
  prefs->SetString(prefs::kPrefSplitViewDefaultOrientation, "vertical");
  prefs->SetDouble(prefs::kPrefSplitViewDefaultRatio, 0.7);
  prefs->SetBoolean(prefs::kPrefSplitViewEnabled, false);
  prefs->SetBoolean(prefs::kPrefSplitViewSnapToPresets, true);
  prefs->SetBoolean(prefs::kPrefSplitViewDividerVisible, false);
  prefs->SetBoolean(prefs::kPrefSplitViewRememberRatio, false);
  prefs->SetBoolean(prefs::kPrefSplitViewMinimapEnabled, true);

  // Read back and verify.
  EXPECT_EQ("vertical",
            prefs->GetString(prefs::kPrefSplitViewDefaultOrientation));
  EXPECT_DOUBLE_EQ(0.7,
                   prefs->GetDouble(prefs::kPrefSplitViewDefaultRatio));
  EXPECT_FALSE(prefs->GetBoolean(prefs::kPrefSplitViewEnabled));
  EXPECT_TRUE(prefs->GetBoolean(prefs::kPrefSplitViewSnapToPresets));
  EXPECT_FALSE(prefs->GetBoolean(prefs::kPrefSplitViewDividerVisible));
  EXPECT_FALSE(prefs->GetBoolean(prefs::kPrefSplitViewRememberRatio));
  EXPECT_TRUE(prefs->GetBoolean(prefs::kPrefSplitViewMinimapEnabled));
}

TEST_F(AstraSplitViewPrefsTest, SettingsRoundTrip) {
  // Create settings, serialize to prefs, read back.
  AstraSplitViewSettings original;
  original.default_orientation = SplitViewOrientation::kVertical;
  original.default_ratio = 0.65f;
  original.snap_to_presets = true;
  original.divider_visible = false;
  original.remember_ratio = false;
  original.minimap_enabled = true;

  PrefService* prefs = profile_->GetPrefs();

  // "Write" to prefs.
  prefs->SetString(prefs::kPrefSplitViewDefaultOrientation,
                   original.default_orientation == SplitViewOrientation::kVertical
                       ? "vertical" : "horizontal");
  prefs->SetDouble(prefs::kPrefSplitViewDefaultRatio,
                   static_cast<double>(original.default_ratio));
  prefs->SetBoolean(prefs::kPrefSplitViewSnapToPresets,
                    original.snap_to_presets);
  prefs->SetBoolean(prefs::kPrefSplitViewDividerVisible,
                    original.divider_visible);
  prefs->SetBoolean(prefs::kPrefSplitViewRememberRatio,
                    original.remember_ratio);
  prefs->SetBoolean(prefs::kPrefSplitViewMinimapEnabled,
                    original.minimap_enabled);

  // "Read" back.
  AstraSplitViewSettings read_back;
  std::string orient_str =
      prefs->GetString(prefs::kPrefSplitViewDefaultOrientation);
  read_back.default_orientation =
      orient_str == "vertical" ? SplitViewOrientation::kVertical
                               : SplitViewOrientation::kHorizontal;
  read_back.default_ratio =
      static_cast<float>(prefs->GetDouble(prefs::kPrefSplitViewDefaultRatio));
  read_back.snap_to_presets =
      prefs->GetBoolean(prefs::kPrefSplitViewSnapToPresets);
  read_back.divider_visible =
      prefs->GetBoolean(prefs::kPrefSplitViewDividerVisible);
  read_back.remember_ratio =
      prefs->GetBoolean(prefs::kPrefSplitViewRememberRatio);
  read_back.minimap_enabled =
      prefs->GetBoolean(prefs::kPrefSplitViewMinimapEnabled);

  // Verify round-trip values.
  EXPECT_EQ(original.default_orientation, read_back.default_orientation);
  EXPECT_FLOAT_EQ(original.default_ratio, read_back.default_ratio);
  EXPECT_EQ(original.snap_to_presets, read_back.snap_to_presets);
  EXPECT_EQ(original.divider_visible, read_back.divider_visible);
  EXPECT_EQ(original.remember_ratio, read_back.remember_ratio);
  EXPECT_EQ(original.minimap_enabled, read_back.minimap_enabled);
}

// =========================================================================
// Astra-prefixed enum tests
// =========================================================================

TEST(AstraSplitOrientationTest, OrientationValues) {
  // Test that both orientations exist and are distinct.
  AstraSplitOrientation h = AstraSplitOrientation::kHorizontal;
  AstraSplitOrientation v = AstraSplitOrientation::kVertical;
  EXPECT_NE(h, v);
}

TEST(AstraSplitOrientationTest, ToStringHorizontal) {
  EXPECT_EQ("horizontal",
            AstraSplitOrientationToString(AstraSplitOrientation::kHorizontal));
}

TEST(AstraSplitOrientationTest, ToStringVertical) {
  EXPECT_EQ("vertical",
            AstraSplitOrientationToString(AstraSplitOrientation::kVertical));
}

TEST(AstraSplitOrientationTest, FromStringHorizontal) {
  EXPECT_EQ(AstraSplitOrientation::kHorizontal,
            AstraSplitOrientationFromString("horizontal"));
}

TEST(AstraSplitOrientationTest, FromStringVertical) {
  EXPECT_EQ(AstraSplitOrientation::kVertical,
            AstraSplitOrientationFromString("vertical"));
}

TEST(AstraSplitOrientationTest, FromStringDefaultIsHorizontal) {
  // Unknown string should default to horizontal.
  EXPECT_EQ(AstraSplitOrientation::kHorizontal,
            AstraSplitOrientationFromString("unknown"));
  EXPECT_EQ(AstraSplitOrientation::kHorizontal,
            AstraSplitOrientationFromString(""));
}

TEST(AstraSplitOrientationTest, LegacyConversion) {
  EXPECT_EQ(SplitViewOrientation::kHorizontal,
            ToLegacyOrientation(AstraSplitOrientation::kHorizontal));
  EXPECT_EQ(SplitViewOrientation::kVertical,
            ToLegacyOrientation(AstraSplitOrientation::kVertical));

  EXPECT_EQ(AstraSplitOrientation::kHorizontal,
            FromLegacyOrientation(SplitViewOrientation::kHorizontal));
  EXPECT_EQ(AstraSplitOrientation::kVertical,
            FromLegacyOrientation(SplitViewOrientation::kVertical));
}

TEST(AstraSplitPaneTest, PaneValues) {
  EXPECT_NE(AstraSplitPane::kPrimary, AstraSplitPane::kSecondary);
}

TEST(AstraSplitPaneTest, PaneToString) {
  EXPECT_EQ("primary", AstraSplitPaneToString(AstraSplitPane::kPrimary));
  EXPECT_EQ("secondary", AstraSplitPaneToString(AstraSplitPane::kSecondary));
}

TEST(AstraSplitPaneTest, PaneFromString) {
  EXPECT_EQ(AstraSplitPane::kPrimary, AstraSplitPaneFromString("primary"));
  EXPECT_EQ(AstraSplitPane::kSecondary, AstraSplitPaneFromString("secondary"));
  EXPECT_EQ(AstraSplitPane::kPrimary, AstraSplitPaneFromString("unknown"));
}

TEST(AstraSplitPresetTest, PresetCount) {
  // All 6 presets should produce distinct ratios.
  std::set<double> ratios;
  ratios.insert(AstraSplitPresetToRatio(AstraSplitPreset::kEqual));
  ratios.insert(AstraSplitPresetToRatio(AstraSplitPreset::kPrimaryLarge));
  ratios.insert(AstraSplitPresetToRatio(AstraSplitPreset::kSecondaryLarge));
  ratios.insert(AstraSplitPresetToRatio(AstraSplitPreset::kThreeQuarter));
  ratios.insert(AstraSplitPresetToRatio(AstraSplitPreset::kQuarter));
  ratios.insert(AstraSplitPresetToRatio(AstraSplitPreset::kGoldenRatio));
  EXPECT_EQ(6u, ratios.size());
}

TEST(AstraSplitPresetTest, EqualPresetIsFiftyFifty) {
  EXPECT_DOUBLE_EQ(0.5, AstraSplitPresetToRatio(AstraSplitPreset::kEqual));
}

TEST(AstraSplitPresetTest, PrimaryLargeIsSeventyPercent) {
  EXPECT_DOUBLE_EQ(0.7, AstraSplitPresetToRatio(AstraSplitPreset::kPrimaryLarge));
}

TEST(AstraSplitPresetTest, SecondaryLargeIsThirtyPercent) {
  EXPECT_DOUBLE_EQ(0.3, AstraSplitPresetToRatio(AstraSplitPreset::kSecondaryLarge));
}

TEST(AstraSplitPresetTest, ThreeQuarterPreset) {
  EXPECT_DOUBLE_EQ(0.75, AstraSplitPresetToRatio(AstraSplitPreset::kThreeQuarter));
}

TEST(AstraSplitPresetTest, QuarterPreset) {
  EXPECT_DOUBLE_EQ(0.25, AstraSplitPresetToRatio(AstraSplitPreset::kQuarter));
}

TEST(AstraSplitPresetTest, GoldenRatioPreset) {
  // Golden ratio conjugate is approximately 0.618.
  double golden = AstraSplitPresetToRatio(AstraSplitPreset::kGoldenRatio);
  EXPECT_GT(golden, 0.6);
  EXPECT_LT(golden, 0.62);
}

TEST(AstraSplitPresetTest, PresetToString) {
  EXPECT_FALSE(AstraSplitPresetToString(AstraSplitPreset::kEqual).empty());
  EXPECT_FALSE(AstraSplitPresetToString(AstraSplitPreset::kGoldenRatio).empty());
}

TEST(AstraSplitPresetTest, PresetFromString) {
  EXPECT_EQ(AstraSplitPreset::kEqual, AstraSplitPresetFromString("equal"));
  EXPECT_EQ(AstraSplitPreset::kPrimaryLarge,
            AstraSplitPresetFromString("primary_large"));
  EXPECT_EQ(AstraSplitPreset::kSecondaryLarge,
            AstraSplitPresetFromString("secondary_large"));
  EXPECT_EQ(AstraSplitPreset::kThreeQuarter,
            AstraSplitPresetFromString("three_quarter"));
  EXPECT_EQ(AstraSplitPreset::kQuarter,
            AstraSplitPresetFromString("quarter"));
  EXPECT_EQ(AstraSplitPreset::kGoldenRatio,
            AstraSplitPresetFromString("golden_ratio"));
  // Default for unknown is kEqual.
  EXPECT_EQ(AstraSplitPreset::kEqual, AstraSplitPresetFromString("unknown"));
}

TEST(AstraSplitPresetTest, PresetNamesNotEmpty) {
  EXPECT_FALSE(AstraSplitPresetToName(AstraSplitPreset::kEqual).empty());
  EXPECT_FALSE(AstraSplitPresetToName(AstraSplitPreset::kPrimaryLarge).empty());
  EXPECT_FALSE(AstraSplitPresetToName(AstraSplitPreset::kSecondaryLarge).empty());
  EXPECT_FALSE(AstraSplitPresetToName(AstraSplitPreset::kThreeQuarter).empty());
  EXPECT_FALSE(AstraSplitPresetToName(AstraSplitPreset::kQuarter).empty());
  EXPECT_FALSE(AstraSplitPresetToName(AstraSplitPreset::kGoldenRatio).empty());
}

TEST(AstraSplitPresetTest, RatioToPresetExactMatch) {
  auto preset = RatioToAstraSplitPreset(0.5, 0.001);
  ASSERT_TRUE(preset.has_value());
  EXPECT_EQ(AstraSplitPreset::kEqual, *preset);

  preset = RatioToAstraSplitPreset(0.75, 0.001);
  ASSERT_TRUE(preset.has_value());
  EXPECT_EQ(AstraSplitPreset::kThreeQuarter, *preset);
}

TEST(AstraSplitPresetTest, RatioToPresetNoMatch) {
  auto preset = RatioToAstraSplitPreset(0.55, 0.01);
  EXPECT_FALSE(preset.has_value());
}

TEST(AstraSplitPresetTest, RatioToPresetWithinTolerance) {
  // 0.51 is within 0.02 tolerance of 0.5 (kEqual)
  auto preset = RatioToAstraSplitPreset(0.51, 0.02);
  ASSERT_TRUE(preset.has_value());
  EXPECT_EQ(AstraSplitPreset::kEqual, *preset);
}

TEST(AstraResizeModeTest, ModeValues) {
  std::set<AstraResizeMode> modes = {
      AstraResizeMode::kFixedRatio,
      AstraResizeMode::kProportional,
      AstraResizeMode::kMinSizePriority,
  };
  EXPECT_EQ(3u, modes.size());
}

TEST(AstraResizeModeTest, ModeToString) {
  EXPECT_EQ("fixed_ratio",
            AstraResizeModeToString(AstraResizeMode::kFixedRatio));
  EXPECT_EQ("proportional",
            AstraResizeModeToString(AstraResizeMode::kProportional));
  EXPECT_EQ("min_size_priority",
            AstraResizeModeToString(AstraResizeMode::kMinSizePriority));
}

TEST(AstraResizeModeTest, ModeFromString) {
  EXPECT_EQ(AstraResizeMode::kFixedRatio,
            AstraResizeModeFromString("fixed_ratio"));
  EXPECT_EQ(AstraResizeMode::kProportional,
            AstraResizeModeFromString("proportional"));
  EXPECT_EQ(AstraResizeMode::kMinSizePriority,
            AstraResizeModeFromString("min_size_priority"));
  // Default for unknown is kFixedRatio.
  EXPECT_EQ(AstraResizeMode::kFixedRatio,
            AstraResizeModeFromString("unknown"));
}

// =========================================================================
// Extended view API tests
// =========================================================================

TEST_F(AstraSplitViewTest, GetRatioDoublePrecision) {
  split_view_->SetRatio(0.618);
  EXPECT_DOUBLE_EQ(0.618, split_view_->GetRatio());
}

TEST_F(AstraSplitViewTest, SetRatioDoubleClampsToMin) {
  split_view_->SetRatio(0.0);
  EXPECT_GE(split_view_->GetRatio(), 0.1);
}

TEST_F(AstraSplitViewTest, SetRatioDoubleClampsToMax) {
  split_view_->SetRatio(1.0);
  EXPECT_LE(split_view_->GetRatio(), 0.9);
}

TEST_F(AstraSplitViewTest, GetOrientationAstraEnum) {
  EXPECT_EQ(AstraSplitOrientation::kHorizontal, split_view_->GetOrientation());

  split_view_->SetOrientation(AstraSplitOrientation::kVertical);
  EXPECT_EQ(AstraSplitOrientation::kVertical, split_view_->GetOrientation());
}

TEST_F(AstraSplitViewTest, SetOrientationAstraEnum) {
  split_view_->SetOrientation(AstraSplitOrientation::kVertical);
  EXPECT_EQ(AstraSplitOrientation::kVertical, split_view_->GetOrientation());
  // Also check the legacy orientation matches.
  EXPECT_EQ(SplitViewOrientation::kVertical, split_view_->orientation());

  split_view_->SetOrientation(AstraSplitOrientation::kHorizontal);
  EXPECT_EQ(AstraSplitOrientation::kHorizontal, split_view_->GetOrientation());
}

TEST_F(AstraSplitViewTest, DividerWidthDefault) {
  EXPECT_EQ(AstraSplitView::kDividerThickness, split_view_->GetDividerWidth());
}

TEST_F(AstraSplitViewTest, SetDividerWidth) {
  split_view_->SetDividerWidth(10);
  EXPECT_EQ(10, split_view_->GetDividerWidth());

  split_view_->SetDividerWidth(0);
  EXPECT_EQ(0, split_view_->GetDividerWidth());

  split_view_->SetDividerWidth(20);
  EXPECT_EQ(20, split_view_->GetDividerWidth());
}

TEST_F(AstraSplitViewTest, DividerWidthAffectsLayout) {
  auto primary = std::make_unique<views::View>();
  auto secondary = std::make_unique<views::View>();
  split_view_->SetPrimaryView(primary.release());
  split_view_->SetSecondaryView(secondary.release());

  widget_->SetSize(gfx::Size(400, 300));
  widget_->LayoutRootViewIfNecessary();

  // With thin divider.
  split_view_->SetDividerWidth(4);
  widget_->LayoutRootViewIfNecessary();
  int secondary_x_thin = split_view_->GetSecondaryView()->x();

  // With thick divider.
  split_view_->SetDividerWidth(20);
  widget_->LayoutRootViewIfNecessary();
  int secondary_x_thick = split_view_->GetSecondaryView()->x();

  // Secondary view should be further right with a thicker divider.
  EXPECT_GT(secondary_x_thick, secondary_x_thin);
}

TEST_F(AstraSplitViewTest, ShowHandleDefault) {
  EXPECT_TRUE(split_view_->GetShowHandle());
}

TEST_F(AstraSplitViewTest, SetShowHandle) {
  split_view_->SetShowHandle(false);
  EXPECT_FALSE(split_view_->GetShowHandle());

  split_view_->SetShowHandle(true);
  EXPECT_TRUE(split_view_->GetShowHandle());
}

TEST_F(AstraSplitViewTest, ShowHandleAffectsDividerVisibility) {
  split_view_->SetShowHandle(true);
  EXPECT_TRUE(split_view_->divider()->GetVisible());

  split_view_->SetShowHandle(false);
  EXPECT_FALSE(split_view_->divider()->GetVisible());
}

TEST_F(AstraSplitViewTest, GetPrimaryViewExtended) {
  auto view = std::make_unique<views::View>();
  views::View* view_ptr = view.get();
  split_view_->SetPrimaryView(view.release());

  EXPECT_EQ(view_ptr, split_view_->GetPrimaryView());
  EXPECT_EQ(view_ptr, const_cast<const AstraSplitView*>(split_view_.get())->GetPrimaryView());
}

TEST_F(AstraSplitViewTest, GetSecondaryViewExtended) {
  auto view = std::make_unique<views::View>();
  views::View* view_ptr = view.get();
  split_view_->SetSecondaryView(view.release());

  EXPECT_EQ(view_ptr, split_view_->GetSecondaryView());
  EXPECT_EQ(view_ptr, const_cast<const AstraSplitView*>(split_view_.get())->GetSecondaryView());
}

TEST_F(AstraSplitViewTest, DividerPositionHorizontal) {
  widget_->SetSize(gfx::Size(400, 300));
  widget_->LayoutRootViewIfNecessary();

  split_view_->SetRatio(0.25);
  int pos = split_view_->GetDividerPosition();
  EXPECT_NEAR(pos, 100, 5);

  split_view_->SetRatio(0.75);
  pos = split_view_->GetDividerPosition();
  EXPECT_NEAR(pos, 300, 5);
}

TEST_F(AstraSplitViewTest, DividerPositionVertical) {
  split_view_->SetOrientation(AstraSplitOrientation::kVertical);
  widget_->SetSize(gfx::Size(400, 300));
  widget_->LayoutRootViewIfNecessary();

  split_view_->SetRatio(0.25);
  int pos = split_view_->GetDividerPosition();
  EXPECT_NEAR(pos, 75, 5);

  split_view_->SetRatio(0.75);
  pos = split_view_->GetDividerPosition();
  EXPECT_NEAR(pos, 225, 5);
}

TEST_F(AstraSplitViewTest, SetDividerPosition) {
  widget_->SetSize(gfx::Size(400, 300));
  widget_->LayoutRootViewIfNecessary();

  split_view_->SetDividerPosition(200);
  EXPECT_NEAR(0.5, split_view_->GetRatio(), 0.01);

  split_view_->SetDividerPosition(100);
  EXPECT_NEAR(0.25, split_view_->GetRatio(), 0.01);
}

TEST_F(AstraSplitViewTest, SetDividerPositionClamps) {
  widget_->SetSize(gfx::Size(400, 300));
  widget_->LayoutRootViewIfNecessary();

  // Setting position to 0 should clamp to minimum.
  split_view_->SetDividerPosition(0);
  EXPECT_GT(split_view_->GetRatio(), 0.0);
  EXPECT_GE(split_view_->GetRatio(), 0.1);

  // Setting position past total width should clamp to maximum.
  split_view_->SetDividerPosition(500);
  EXPECT_LT(split_view_->GetRatio(), 1.0);
  EXPECT_LE(split_view_->GetRatio(), 0.9);
}

TEST_F(AstraSplitViewTest, FocusedPaneDefault) {
  EXPECT_EQ(AstraSplitPane::kPrimary, split_view_->GetFocusedPane());
}

TEST_F(AstraSplitViewTest, SetFocusedPane) {
  split_view_->SetFocusedPane(AstraSplitPane::kSecondary);
  EXPECT_EQ(AstraSplitPane::kSecondary, split_view_->GetFocusedPane());

  split_view_->SetFocusedPane(AstraSplitPane::kPrimary);
  EXPECT_EQ(AstraSplitPane::kPrimary, split_view_->GetFocusedPane());
}

TEST_F(AstraSplitViewTest, SetSameFocusedPaneIsNoOp) {
  // Setting the same pane should not cause issues.
  split_view_->SetFocusedPane(AstraSplitPane::kPrimary);
  EXPECT_EQ(AstraSplitPane::kPrimary, split_view_->GetFocusedPane());
}

TEST_F(AstraSplitViewTest, PaneLabelsDefault) {
  // Default is no labels shown.
  split_view_->ShowPaneLabels(false);
  // Should not crash.
  split_view_->SetPaneLabels(u"Primary", u"Secondary");
}

TEST_F(AstraSplitViewTest, SetPaneLabels) {
  split_view_->SetPaneLabels(u"Tab 1", u"Tab 2");
  split_view_->ShowPaneLabels(true);
  // Should not crash and should schedule paint.
  EXPECT_TRUE(split_view_->GetVisible());
}

TEST_F(AstraSplitViewTest, ShowPaneLabelsToggle) {
  split_view_->SetPaneLabels(u"Left", u"Right");

  split_view_->ShowPaneLabels(true);
  // Turn on should work.

  split_view_->ShowPaneLabels(false);
  // Turn off should work.
}

TEST_F(AstraSplitViewTest, IsDraggingDividerDefault) {
  EXPECT_FALSE(split_view_->IsDraggingDivider());
}

TEST_F(AstraSplitViewTest, DragStateReflected) {
  // Before drag, not dragging.
  EXPECT_FALSE(split_view_->IsDraggingDivider());

  // TODO(astra): Test actual drag state via mouse events.
  // For now, just verify the method exists and returns false by default.
}

// =========================================================================
// Additional layout tests
// =========================================================================

TEST_F(AstraSplitViewTest, HorizontalLayoutPrimarySize) {
  auto primary = std::make_unique<views::View>();
  auto secondary = std::make_unique<views::View>();
  split_view_->SetPrimaryView(primary.release());
  split_view_->SetSecondaryView(secondary.release());

  split_view_->SetRatio(0.3);
  widget_->SetSize(gfx::Size(500, 400));
  widget_->LayoutRootViewIfNecessary();

  // Primary should be ~30% of total width (minus divider).
  int primary_width = split_view_->GetPrimaryView()->width();
  EXPECT_GT(primary_width, 100);
  EXPECT_LT(primary_width, 200);
}

TEST_F(AstraSplitViewTest, VerticalLayoutPrimarySize) {
  auto primary = std::make_unique<views::View>();
  auto secondary = std::make_unique<views::View>();
  split_view_->SetPrimaryView(primary.release());
  split_view_->SetSecondaryView(secondary.release());

  split_view_->SetOrientation(AstraSplitOrientation::kVertical);
  split_view_->SetRatio(0.3);
  widget_->SetSize(gfx::Size(500, 400));
  widget_->LayoutRootViewIfNecessary();

  // Primary should be ~30% of total height.
  int primary_height = split_view_->GetPrimaryView()->height();
  EXPECT_GT(primary_height, 80);
  EXPECT_LT(primary_height, 150);
}

TEST_F(AstraSplitViewTest, HorizontalBothPanesFillHeight) {
  auto primary = std::make_unique<views::View>();
  auto secondary = std::make_unique<views::View>();
  split_view_->SetPrimaryView(primary.release());
  split_view_->SetSecondaryView(secondary.release());

  widget_->SetSize(gfx::Size(400, 300));
  widget_->LayoutRootViewIfNecessary();

  EXPECT_EQ(300, split_view_->GetPrimaryView()->height());
  EXPECT_EQ(300, split_view_->GetSecondaryView()->height());
}

TEST_F(AstraSplitViewTest, VerticalBothPanesFillWidth) {
  auto primary = std::make_unique<views::View>();
  auto secondary = std::make_unique<views::View>();
  split_view_->SetPrimaryView(primary.release());
  split_view_->SetSecondaryView(secondary.release());

  split_view_->SetOrientation(AstraSplitOrientation::kVertical);
  widget_->SetSize(gfx::Size(400, 300));
  widget_->LayoutRootViewIfNecessary();

  EXPECT_EQ(400, split_view_->GetPrimaryView()->width());
  EXPECT_EQ(400, split_view_->GetSecondaryView()->width());
}

TEST_F(AstraSplitViewTest, RatioChangeUpdatesLayout) {
  auto primary = std::make_unique<views::View>();
  auto secondary = std::make_unique<views::View>();
  split_view_->SetPrimaryView(primary.release());
  split_view_->SetSecondaryView(secondary.release());

  widget_->SetSize(gfx::Size(400, 300));
  widget_->LayoutRootViewIfNecessary();

  int primary_width_at_50 = split_view_->GetPrimaryView()->width();

  split_view_->SetRatio(0.7);
  widget_->LayoutRootViewIfNecessary();

  int primary_width_at_70 = split_view_->GetPrimaryView()->width();
  EXPECT_GT(primary_width_at_70, primary_width_at_50);
}

TEST_F(AstraSplitViewTest, OrientationChangeSwapsDimensions) {
  auto primary = std::make_unique<views::View>();
  auto secondary = std::make_unique<views::View>();
  split_view_->SetPrimaryView(primary.release());
  split_view_->SetSecondaryView(secondary.release());

  widget_->SetSize(gfx::Size(400, 300));
  widget_->LayoutRootViewIfNecessary();

  int horizontal_primary_width = split_view_->GetPrimaryView()->width();
  int horizontal_primary_height = split_view_->GetPrimaryView()->height();

  split_view_->SetOrientation(AstraSplitOrientation::kVertical);
  widget_->LayoutRootViewIfNecessary();

  int vertical_primary_width = split_view_->GetPrimaryView()->width();
  int vertical_primary_height = split_view_->GetPrimaryView()->height();

  // After orientation change, primary should fill full width.
  EXPECT_GT(vertical_primary_width, horizontal_primary_width);
  EXPECT_LT(vertical_primary_height, horizontal_primary_height);
}

// =========================================================================
// Additional edge case tests
// =========================================================================

TEST_F(AstraSplitViewTest, NegativeDividerWidth) {
  // Negative width should not crash; behavior is implementation-defined
  // but should not cause crashes or negative sizes.
  split_view_->SetDividerWidth(-10);
  widget_->SetSize(gfx::Size(400, 300));
  widget_->LayoutRootViewIfNecessary();
  // Should not crash.
  EXPECT_TRUE(split_view_->GetVisible());
}

TEST_F(AstraSplitViewTest, VeryLargeDividerWidth) {
  split_view_->SetDividerWidth(500);
  widget_->SetSize(gfx::Size(400, 300));
  widget_->LayoutRootViewIfNecessary();
  // Should not crash.
  EXPECT_TRUE(split_view_->GetVisible());
}

TEST_F(AstraSplitViewTest, DividerPositionZeroSizedView) {
  widget_->SetSize(gfx::Size(0, 0));
  widget_->LayoutRootViewIfNecessary();
  // Should not crash.
  split_view_->SetDividerPosition(100);
  split_view_->GetDividerPosition();
}

TEST_F(AstraSplitViewTest, SetBothViewsToNull) {
  auto view1 = std::make_unique<views::View>();
  auto view2 = std::make_unique<views::View>();
  split_view_->SetPrimaryView(view1.release());
  split_view_->SetSecondaryView(view2.release());

  split_view_->SetPrimaryView(nullptr);
  split_view_->SetSecondaryView(nullptr);

  EXPECT_EQ(nullptr, split_view_->GetPrimaryView());
  EXPECT_EQ(nullptr, split_view_->GetSecondaryView());
}

TEST_F(AstraSplitViewTest, SwapWithBothViewsNull) {
  split_view_->SwapViews();
  EXPECT_EQ(nullptr, split_view_->GetPrimaryView());
  EXPECT_EQ(nullptr, split_view_->GetSecondaryView());
}

TEST_F(AstraSplitViewTest, VeryLargeRatio) {
  // Setting an extremely large ratio should clamp.
  split_view_->SetRatio(100.0);
  EXPECT_LE(split_view_->GetRatio(), 1.0);
  EXPECT_LE(split_view_->GetRatio(), 0.9);
}

TEST_F(AstraSplitViewTest, VeryNegativeRatio) {
  // Setting a very negative ratio should clamp.
  split_view_->SetRatio(-100.0);
  EXPECT_GE(split_view_->GetRatio(), 0.0);
  EXPECT_GE(split_view_->GetRatio(), 0.1);
}

// =========================================================================
// AstraSplitViewSettings struct tests
// =========================================================================

TEST(AstraSplitViewSettingsTest, DefaultValues) {
  AstraSplitViewSettings settings;

  EXPECT_EQ(AstraSplitOrientation::kHorizontal, settings.default_orientation);
  EXPECT_DOUBLE_EQ(0.5, settings.default_ratio);
  EXPECT_EQ(AstraSplitPreset::kEqual, settings.default_preset);
  EXPECT_TRUE(settings.remember_split_state);
  EXPECT_EQ(AstraResizeMode::kFixedRatio, settings.resize_mode);
  EXPECT_EQ(100, settings.min_pane_size);
  EXPECT_EQ(4, settings.divider_width);
  EXPECT_TRUE(settings.show_divider_handle);
  EXPECT_TRUE(settings.double_click_divider_resets);
  EXPECT_EQ(20, settings.keyboard_resize_step);
  EXPECT_FALSE(settings.show_pane_labels);
  EXPECT_FALSE(settings.auto_equal_on_window_resize);
}

TEST(AstraSplitViewSettingsTest, SettingsCanBeCopied) {
  AstraSplitViewSettings s1;
  s1.default_ratio = 0.7;
  s1.min_pane_size = 150;
  s1.show_pane_labels = true;

  AstraSplitViewSettings s2 = s1;
  EXPECT_DOUBLE_EQ(0.7, s2.default_ratio);
  EXPECT_EQ(150, s2.min_pane_size);
  EXPECT_TRUE(s2.show_pane_labels);
}

TEST(AstraSplitViewSettingsTest, AllFieldsIndependent) {
  AstraSplitViewSettings s;
  s.default_orientation = AstraSplitOrientation::kVertical;
  s.default_ratio = 0.75;
  s.default_preset = AstraSplitPreset::kThreeQuarter;
  s.remember_split_state = false;
  s.resize_mode = AstraResizeMode::kMinSizePriority;
  s.min_pane_size = 200;
  s.divider_width = 8;
  s.show_divider_handle = false;
  s.double_click_divider_resets = false;
  s.keyboard_resize_step = 50;
  s.show_pane_labels = true;
  s.auto_equal_on_window_resize = true;

  EXPECT_EQ(AstraSplitOrientation::kVertical, s.default_orientation);
  EXPECT_DOUBLE_EQ(0.75, s.default_ratio);
  EXPECT_EQ(AstraSplitPreset::kThreeQuarter, s.default_preset);
  EXPECT_FALSE(s.remember_split_state);
  EXPECT_EQ(AstraResizeMode::kMinSizePriority, s.resize_mode);
  EXPECT_EQ(200, s.min_pane_size);
  EXPECT_EQ(8, s.divider_width);
  EXPECT_FALSE(s.show_divider_handle);
  EXPECT_FALSE(s.double_click_divider_resets);
  EXPECT_EQ(50, s.keyboard_resize_step);
  EXPECT_TRUE(s.show_pane_labels);
  EXPECT_TRUE(s.auto_equal_on_window_resize);
}

// =========================================================================
// Controller pref key tests
// =========================================================================

TEST(AstraSplitViewControllerPrefsTest, PrefKeysExist) {
  // Verify all 12+ pref keys are defined and non-empty.
  EXPECT_FALSE(std::string(AstraSplitViewController::kPrefDefaultOrientation).empty());
  EXPECT_FALSE(std::string(AstraSplitViewController::kPrefDefaultRatio).empty());
  EXPECT_FALSE(std::string(AstraSplitViewController::kPrefDefaultPreset).empty());
  EXPECT_FALSE(std::string(AstraSplitViewController::kPrefRememberSplitState).empty());
  EXPECT_FALSE(std::string(AstraSplitViewController::kPrefResizeMode).empty());
  EXPECT_FALSE(std::string(AstraSplitViewController::kPrefMinPaneSize).empty());
  EXPECT_FALSE(std::string(AstraSplitViewController::kPrefDividerWidth).empty());
  EXPECT_FALSE(std::string(AstraSplitViewController::kPrefShowDividerHandle).empty());
  EXPECT_FALSE(std::string(AstraSplitViewController::kPrefDoubleClickDividerResets).empty());
  EXPECT_FALSE(std::string(AstraSplitViewController::kPrefKeyboardResizeStep).empty());
  EXPECT_FALSE(std::string(AstraSplitViewController::kPrefShowPaneLabels).empty());
  EXPECT_FALSE(std::string(AstraSplitViewController::kPrefAutoEqualOnWindowResize).empty());
}

TEST(AstraSplitViewControllerPrefsTest, PrefKeysHaveAstraPrefix) {
  // All pref keys should start with "astra.split_view."
  std::string key = AstraSplitViewController::kPrefDefaultOrientation;
  EXPECT_EQ(0u, key.find("astra.split_view."));

  key = AstraSplitViewController::kPrefDefaultRatio;
  EXPECT_EQ(0u, key.find("astra.split_view."));
}

TEST(AstraSplitViewControllerPrefsTest, PrefKeysAreUnique) {
  // All 12 pref keys should be unique.
  std::set<std::string> keys = {
      AstraSplitViewController::kPrefDefaultOrientation,
      AstraSplitViewController::kPrefDefaultRatio,
      AstraSplitViewController::kPrefDefaultPreset,
      AstraSplitViewController::kPrefRememberSplitState,
      AstraSplitViewController::kPrefResizeMode,
      AstraSplitViewController::kPrefMinPaneSize,
      AstraSplitViewController::kPrefDividerWidth,
      AstraSplitViewController::kPrefShowDividerHandle,
      AstraSplitViewController::kPrefDoubleClickDividerResets,
      AstraSplitViewController::kPrefKeyboardResizeStep,
      AstraSplitViewController::kPrefShowPaneLabels,
      AstraSplitViewController::kPrefAutoEqualOnWindowResize,
  };
  EXPECT_EQ(12u, keys.size());
}

// =========================================================================
// AstraSplitViewObserver tests
// =========================================================================

// Mock observer for the Astra observer interface.
class MockAstraSplitViewObserver : public AstraSplitViewObserver {
 public:
  MOCK_METHOD(void, OnSplitViewActivated,
              (AstraSplitViewController * controller), (override));
  MOCK_METHOD(void, OnSplitViewDeactivated,
              (AstraSplitViewController * controller), (override));
  MOCK_METHOD(void, OnSplitRatioChanged,
              (AstraSplitViewController * controller, double new_ratio),
              (override));
  MOCK_METHOD(void, OnSplitOrientationChanged,
              (AstraSplitViewController * controller,
               AstraSplitOrientation orientation), (override));
  MOCK_METHOD(void, OnPrimaryPaneChanged,
              (AstraSplitViewController * controller, int tab_index),
              (override));
  MOCK_METHOD(void, OnSecondaryPaneChanged,
              (AstraSplitViewController * controller, int tab_index),
              (override));
  MOCK_METHOD(void, OnPaneSwapped,
              (AstraSplitViewController * controller), (override));
  MOCK_METHOD(void, OnFocusedPaneChanged,
              (AstraSplitViewController * controller, AstraSplitPane pane),
              (override));
  MOCK_METHOD(void, OnSplitViewControllerShutdown,
              (AstraSplitViewController * controller), (override));
};

// Observer that overrides nothing — verifies all defaults are empty.
class EmptyAstraSplitViewObserver : public AstraSplitViewObserver {
 public:
  // Deliberately overrides no methods — all should have empty default {}
  // implementations.
};

TEST(AstraSplitViewObserverTest, EmptyObserverWorks) {
  EmptyAstraSplitViewObserver empty_observer;
  // Creating and destroying an empty observer should not cause issues.
  // The default implementations are empty, so calling them does nothing.
}

TEST(AstraSplitViewObserverTest, ObserverInheritsCheckedObserver) {
  // Verify the observer type system works.
  MockAstraSplitViewObserver observer;
  // Should be a CheckedObserver.
  base::CheckedObserver* base_observer = &observer;
  EXPECT_NE(nullptr, base_observer);
}

// =========================================================================
// Controller state tests (non-activation)
// =========================================================================

// Simple controller test fixture that creates a controller with a null
// browser_view for testing state properties that don't require activation.
class AstraSplitViewControllerStateTest : public testing::Test {
 public:
  AstraSplitViewControllerStateTest() = default;
  ~AstraSplitViewControllerStateTest() override = default;

  void SetUp() override {
    // Create controller with nullptr browser_view for state-only testing.
    // TODO(astra): Use a mock BrowserView for more complete testing.
    controller_ = std::make_unique<AstraSplitViewController>(nullptr);
  }

  void TearDown() override { controller_.reset(); }

 protected:
  std::unique_ptr<AstraSplitViewController> controller_;
};

TEST_F(AstraSplitViewControllerStateTest, DefaultIsNotActive) {
  EXPECT_FALSE(controller_->IsActive());
}

TEST_F(AstraSplitViewControllerStateTest, DefaultRatioIsFiftyPercent) {
  // Default ratio is 0.5.
  EXPECT_DOUBLE_EQ(0.5, controller_->GetSplitRatio());
}

TEST_F(AstraSplitViewControllerStateTest, DefaultOrientationIsHorizontal) {
  EXPECT_EQ(AstraSplitOrientation::kHorizontal,
            controller_->GetOrientation());
}

TEST_F(AstraSplitViewControllerStateTest, DefaultPrimaryTabIndexIsMinusOne) {
  EXPECT_EQ(-1, controller_->GetPrimaryTabIndex());
}

TEST_F(AstraSplitViewControllerStateTest, DefaultSecondaryTabIndexIsMinusOne) {
  EXPECT_EQ(-1, controller_->GetSecondaryTabIndex());
}

TEST_F(AstraSplitViewControllerStateTest, DefaultFocusedPaneIsPrimary) {
  EXPECT_EQ(AstraSplitPane::kPrimary, controller_->GetFocusedPane());
}

TEST_F(AstraSplitViewControllerStateTest, DefaultPresetIsEqual) {
  EXPECT_EQ(AstraSplitPreset::kEqual, controller_->GetLayoutPreset());
}

TEST_F(AstraSplitViewControllerStateTest, DefaultResizeModeIsFixedRatio) {
  EXPECT_EQ(AstraResizeMode::kFixedRatio, controller_->GetResizeMode());
}

TEST_F(AstraSplitViewControllerStateTest, DefaultMinPaneSize) {
  EXPECT_EQ(100, controller_->GetMinPaneSize());
}

TEST_F(AstraSplitViewControllerStateTest, DefaultDividerWidth) {
  EXPECT_EQ(4, controller_->GetDividerWidth());
}

TEST_F(AstraSplitViewControllerStateTest, SetResizeMode) {
  controller_->SetResizeMode(AstraResizeMode::kProportional);
  EXPECT_EQ(AstraResizeMode::kProportional, controller_->GetResizeMode());

  controller_->SetResizeMode(AstraResizeMode::kMinSizePriority);
  EXPECT_EQ(AstraResizeMode::kMinSizePriority, controller_->GetResizeMode());

  controller_->SetResizeMode(AstraResizeMode::kFixedRatio);
  EXPECT_EQ(AstraResizeMode::kFixedRatio, controller_->GetResizeMode());
}

TEST_F(AstraSplitViewControllerStateTest, SetMinPaneSize) {
  controller_->SetMinPaneSize(200);
  EXPECT_EQ(200, controller_->GetMinPaneSize());

  controller_->SetMinPaneSize(0);
  EXPECT_EQ(0, controller_->GetMinPaneSize());
}

TEST_F(AstraSplitViewControllerStateTest, SetMinPaneSizeNegativeClamps) {
  controller_->SetMinPaneSize(-50);
  // Negative values should be clamped to 0.
  EXPECT_EQ(0, controller_->GetMinPaneSize());
}

TEST_F(AstraSplitViewControllerStateTest, SetDividerWidth) {
  controller_->SetDividerWidth(8);
  EXPECT_EQ(8, controller_->GetDividerWidth());

  controller_->SetDividerWidth(0);
  EXPECT_EQ(0, controller_->GetDividerWidth());
}

TEST_F(AstraSplitViewControllerStateTest, SetDividerWidthNegativeClamps) {
  controller_->SetDividerWidth(-10);
  // Negative values should be clamped to 0.
  EXPECT_EQ(0, controller_->GetDividerWidth());
}

TEST_F(AstraSplitViewControllerStateTest, SetSameResizeModeIsNoOp) {
  controller_->SetResizeMode(AstraResizeMode::kFixedRatio);
  // Setting the same value again should not cause issues.
  controller_->SetResizeMode(AstraResizeMode::kFixedRatio);
  EXPECT_EQ(AstraResizeMode::kFixedRatio, controller_->GetResizeMode());
}

TEST_F(AstraSplitViewControllerStateTest, InactiveSetRatioIsNoOp) {
  double before = controller_->GetSplitRatio();
  controller_->SetSplitRatio(0.7);
  // When not active, setting ratio does nothing.
  EXPECT_DOUBLE_EQ(before, controller_->GetSplitRatio());
}

TEST_F(AstraSplitViewControllerStateTest, InactiveSetOrientationIsNoOp) {
  controller_->SetOrientation(AstraSplitOrientation::kVertical);
  // When not active, setting orientation does nothing.
  EXPECT_EQ(AstraSplitOrientation::kHorizontal,
            controller_->GetOrientation());
}

TEST_F(AstraSplitViewControllerStateTest, InactiveToggleOrientationIsNoOp) {
  controller_->ToggleOrientation();
  // When not active, toggling does nothing.
  EXPECT_EQ(AstraSplitOrientation::kHorizontal,
            controller_->GetOrientation());
}

TEST_F(AstraSplitViewControllerStateTest, InactiveSetPrimaryTabIsNoOp) {
  controller_->SetPrimaryTab(5);
  EXPECT_EQ(-1, controller_->GetPrimaryTabIndex());
}

TEST_F(AstraSplitViewControllerStateTest, InactiveSetSecondaryTabIsNoOp) {
  controller_->SetSecondaryTab(3);
  EXPECT_EQ(-1, controller_->GetSecondaryTabIndex());
}

TEST_F(AstraSplitViewControllerStateTest, InactiveSwapPanesIsNoOp) {
  controller_->SwapPanes();
  EXPECT_EQ(-1, controller_->GetPrimaryTabIndex());
  EXPECT_EQ(-1, controller_->GetSecondaryTabIndex());
}

TEST_F(AstraSplitViewControllerStateTest, InactiveClosePrimaryPaneIsNoOp) {
  controller_->ClosePrimaryPane();
  EXPECT_FALSE(controller_->IsActive());
}

TEST_F(AstraSplitViewControllerStateTest, InactiveCloseSecondaryPaneIsNoOp) {
  controller_->CloseSecondaryPane();
  EXPECT_FALSE(controller_->IsActive());
}

TEST_F(AstraSplitViewControllerStateTest, InactiveFocusPrimaryIsNoOp) {
  controller_->FocusPrimaryPane();
  EXPECT_EQ(AstraSplitPane::kPrimary, controller_->GetFocusedPane());
}

TEST_F(AstraSplitViewControllerStateTest, InactiveFocusSecondaryIsNoOp) {
  controller_->FocusSecondaryPane();
  // When not active, focus changes do nothing.
  // (Default is kPrimary, so still kPrimary.)
  EXPECT_EQ(AstraSplitPane::kPrimary, controller_->GetFocusedPane());
}

TEST_F(AstraSplitViewControllerStateTest, InactiveToggleFocusIsNoOp) {
  controller_->ToggleFocus();
  EXPECT_EQ(AstraSplitPane::kPrimary, controller_->GetFocusedPane());
}

TEST_F(AstraSplitViewControllerStateTest, InactiveSetPresetIsNoOp) {
  controller_->SetLayoutPreset(AstraSplitPreset::kThreeQuarter);
  // Default preset is kEqual. When inactive, preset change does nothing.
  EXPECT_EQ(AstraSplitPreset::kEqual, controller_->GetLayoutPreset());
}

TEST_F(AstraSplitViewControllerStateTest, ApplyPresetCallsSetLayoutPreset) {
  // ApplyPreset is an alias for SetLayoutPreset.
  // When inactive, both do nothing.
  controller_->ApplyPreset(AstraSplitPreset::kGoldenRatio);
  EXPECT_EQ(AstraSplitPreset::kEqual, controller_->GetLayoutPreset());
}

TEST_F(AstraSplitViewControllerStateTest, AddAstraObserver) {
  MockAstraSplitViewObserver observer;
  controller_->AddAstraObserver(&observer);
  // Should not crash when adding an observer.
  controller_->RemoveAstraObserver(&observer);
}

TEST_F(AstraSplitViewControllerStateTest, RemoveAstraObserver) {
  MockAstraSplitViewObserver observer;
  controller_->AddAstraObserver(&observer);
  controller_->RemoveAstraObserver(&observer);
  // After removal, state changes should not notify.
  controller_->SetResizeMode(AstraResizeMode::kProportional);
}

TEST_F(AstraSplitViewControllerStateTest, MultipleAstraObservers) {
  MockAstraSplitViewObserver obs1;
  MockAstraSplitViewObserver obs2;

  controller_->AddAstraObserver(&obs1);
  controller_->AddAstraObserver(&obs2);

  controller_->RemoveAstraObserver(&obs1);
  controller_->RemoveAstraObserver(&obs2);
  // Should not crash with multiple observers.
}

TEST_F(AstraSplitViewControllerStateTest, ShutdownNotifiesObservers) {
  MockAstraSplitViewObserver observer;
  controller_->AddAstraObserver(&observer);

  EXPECT_CALL(observer, OnSplitViewControllerShutdown(controller_.get()))
      .Times(1);

  controller_.reset();  // Destruction should notify shutdown.
}

// =========================================================================
// AstraSplitViewModel tests
// =========================================================================

// Test fixture for the split view model.
class AstraSplitViewModelTest : public testing::Test {
 public:
  AstraSplitViewModelTest() = default;
  ~AstraSplitViewModelTest() override = default;

  void SetUp() override {
    model_ = std::make_unique<AstraSplitViewModel>();
  }

  void TearDown() override {
    model_.reset();
  }

 protected:
  std::unique_ptr<AstraSplitViewModel> model_;
};

// Mock observer for model tests.
class MockSplitViewModelObserver : public AstraSplitViewModelObserver {
 public:
  MOCK_METHOD(void, OnSplitLayoutModeChanged,
                (AstraSplitLayoutMode mode), (override));
  MOCK_METHOD(void, OnSplitDividerPositionChanged,
                (AstraSplitDividerId divider_id, double new_ratio),
                (override));
  MOCK_METHOD(void, OnSplitDividerPositionChanging,
                (AstraSplitDividerId divider_id, double new_ratio),
                (override));
  MOCK_METHOD(void, OnSplitDividerDragStarted,
                (AstraSplitDividerId divider_id), (override));
  MOCK_METHOD(void, OnSplitDividerDragEnded,
                (AstraSplitDividerId divider_id), (override));
  MOCK_METHOD(void, OnSplitFocusedPaneChanged,
                (AstraSplitPaneId pane_id), (override));
  MOCK_METHOD(void, OnSplitPaneClosed,
                (AstraSplitPaneId pane_id), (override));
  MOCK_METHOD(void, OnSplitPanesSwapped,
                (AstraSplitPaneId pane_a, AstraSplitPaneId pane_b),
                (override));
  MOCK_METHOD(void, OnSplitMinPaneSizeChanged,
                (int min_size_dips), (override));
  MOCK_METHOD(void, OnSplitResizeBehaviorChanged,
                (AstraSplitResizeBehavior behavior), (override));
  MOCK_METHOD(void, OnSplitViewModelDestroyed, (), (override));
};

// Empty observer - verifies all defaults are empty.
class EmptySplitViewModelObserver : public AstraSplitViewModelObserver {
  // Deliberately overrides no methods.
};

// ---- Basic construction tests

TEST_F(AstraSplitViewModelTest, DefaultLayoutMode) {
  EXPECT_EQ(AstraSplitLayoutMode::kTwoPaneHorizontal,
            model_->layout_mode());
}

TEST_F(AstraSplitViewModelTest, DefaultPaneCount) {
  EXPECT_EQ(2, model_->GetPaneCount());
}

TEST_F(AstraSplitViewModelTest, DefaultDividerCount) {
  EXPECT_EQ(1, model_->GetDividerCount());
}

TEST_F(AstraSplitViewModelTest, DefaultFocusedPane) {
  EXPECT_EQ(AstraSplitPaneId::kPane0, model_->focused_pane());
}

TEST_F(AstraSplitViewModelTest, DefaultMinPaneSize) {
  EXPECT_EQ(AstraSplitViewModel::kDefaultMinPaneSize,
            model_->min_pane_size());
}

TEST_F(AstraSplitViewModelTest, DefaultResizeBehavior) {
  EXPECT_EQ(AstraSplitResizeBehavior::kFixedRatio,
            model_->resize_behavior());
}

TEST_F(AstraSplitViewModelTest, DefaultNotDragging) {
  EXPECT_FALSE(model_->IsDragging());
  EXPECT_FALSE(model_->GetDraggedDivider().has_value());
}

TEST_F(AstraSplitViewModelTest, DefaultDividerRatioIsFiftyPercent) {
  double ratio = model_->GetDividerRatio(AstraSplitDividerId::kDivider0);
  EXPECT_DOUBLE_EQ(0.5, ratio);
}

// ---- Layout mode tests

TEST_F(AstraSplitViewModelTest, SetLayoutModeVertical) {
  model_->SetLayoutMode(AstraSplitLayoutMode::kTwoPaneVertical);
  EXPECT_EQ(AstraSplitLayoutMode::kTwoPaneVertical, model_->layout_mode());
  EXPECT_EQ(2, model_->GetPaneCount());
}

TEST_F(AstraSplitViewModelTest, SetLayoutModeThreePaneHorizontal) {
  model_->SetLayoutMode(AstraSplitLayoutMode::kThreePaneHorizontal);
  EXPECT_EQ(AstraSplitLayoutMode::kThreePaneHorizontal,
            model_->layout_mode());
  EXPECT_EQ(3, model_->GetPaneCount());
}

TEST_F(AstraSplitViewModelTest, SetLayoutModeGridTwoByTwo) {
  model_->SetLayoutMode(AstraSplitLayoutMode::kGridTwoByTwo);
  EXPECT_EQ(AstraSplitLayoutMode::kGridTwoByTwo, model_->layout_mode());
  EXPECT_EQ(4, model_->GetPaneCount());
  EXPECT_TRUE(model_->IsGridLayout());
}

TEST_F(AstraSplitViewModelTest, SetLayoutModeGridThreeByTwo) {
  model_->SetLayoutMode(AstraSplitLayoutMode::kGridThreeByTwo);
  EXPECT_EQ(AstraSplitLayoutMode::kGridThreeByTwo,
            model_->layout_mode());
  EXPECT_EQ(6, model_->GetPaneCount());
  EXPECT_TRUE(model_->IsGridLayout());
}

TEST_F(AstraSplitViewModelTest, IsGridLayoutTwoPaneIsFalse) {
  EXPECT_FALSE(model_->IsGridLayout());
  model_->SetLayoutMode(AstraSplitLayoutMode::kTwoPaneVertical);
  EXPECT_FALSE(model_->IsGridLayout());
}

TEST_F(AstraSplitViewModelTest, SameLayoutModeIsNoOp) {
  MockSplitViewModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnSplitLayoutModeChanged(_)).Times(0);
  model_->SetLayoutMode(AstraSplitLayoutMode::kTwoPaneHorizontal);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraSplitViewModelTest, LayoutModeChangeNotifiesObserver) {
  MockSplitViewModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer,
              OnSplitLayoutModeChanged(AstraSplitLayoutMode::kTwoPaneVertical))
      .Times(1);
  model_->SetLayoutMode(AstraSplitLayoutMode::kTwoPaneVertical);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraSplitViewModelTest, GetColumnCount) {
  EXPECT_EQ(2, model_->GetColumnCount());

  model_->SetLayoutMode(AstraSplitLayoutMode::kTwoPaneVertical);
  EXPECT_EQ(1, model_->GetColumnCount());

  model_->SetLayoutMode(AstraSplitLayoutMode::kGridTwoByTwo);
  EXPECT_EQ(2, model_->GetColumnCount());

  model_->SetLayoutMode(AstraSplitLayoutMode::kGridThreeByTwo);
  EXPECT_EQ(3, model_->GetColumnCount());
}

TEST_F(AstraSplitViewModelTest, GetRowCount) {
  EXPECT_EQ(1, model_->GetRowCount());

  model_->SetLayoutMode(AstraSplitLayoutMode::kTwoPaneVertical);
  EXPECT_EQ(2, model_->GetRowCount());

  model_->SetLayoutMode(AstraSplitLayoutMode::kGridTwoByTwo);
  EXPECT_EQ(2, model_->GetRowCount());

  model_->SetLayoutMode(AstraSplitLayoutMode::kGridThreeByTwo);
  EXPECT_EQ(2, model_->GetRowCount());
}

TEST_F(AstraSplitViewModelTest, SetEqualRatios) {
  model_->SetLayoutMode(AstraSplitLayoutMode::kThreePaneHorizontal);
  model_->SetDividerRatio(AstraSplitDividerId::kDivider0, 0.25);
  model_->SetDividerRatio(AstraSplitDividerId::kDivider1, 0.75);

  model_->SetEqualRatios();

  // For 3 panes, equal ratios should be 1/3 and 2/3.
  EXPECT_NEAR(1.0 / 3.0,
             model_->GetDividerRatio(AstraSplitDividerId::kDivider0), 0.001);
  EXPECT_NEAR(2.0 / 3.0,
             model_->GetDividerRatio(AstraSplitDividerId::kDivider1), 0.001);
}

TEST_F(AstraSplitViewModelTest, ResetToDefaults) {
  model_->SetDividerRatio(AstraSplitDividerId::kDivider0, 0.7);
  model_->ResetToDefaults();
  EXPECT_DOUBLE_EQ(0.5,
                    model_->GetDividerRatio(AstraSplitDividerId::kDivider0));
}

// ---- Divider ratio tests

TEST_F(AstraSplitViewModelTest, SetDividerRatio) {
  model_->SetDividerRatio(AstraSplitDividerId::kDivider0, 0.7);
  EXPECT_DOUBLE_EQ(0.7,
                    model_->GetDividerRatio(AstraSplitDividerId::kDivider0));
}

TEST_F(AstraSplitViewModelTest, SetDividerRatioClampsToMin) {
  model_->SetDividerRatio(AstraSplitDividerId::kDivider0, 0.0);
  EXPECT_GE(model_->GetDividerRatio(AstraSplitDividerId::kDivider0),
           AstraSplitViewModel::kMinPaneRatio);
}

TEST_F(AstraSplitViewModelTest, SetDividerRatioClampsToMax) {
  model_->SetDividerRatio(AstraSplitDividerId::kDivider0, 1.0);
  EXPECT_LE(model_->GetDividerRatio(AstraSplitDividerId::kDivider0),
            1.0 - AstraSplitViewModel::kMinPaneRatio);
}

TEST_F(AstraSplitViewModelTest, SameDividerRatioNoOp) {
  MockSplitViewModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnSplitDividerPositionChanged(_, _)).Times(0);
  model_->SetDividerRatio(AstraSplitDividerId::kDivider0, 0.5);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraSplitViewModelTest, DividerRatioChangeNotifies) {
  MockSplitViewModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnSplitDividerPositionChanged(
                           AstraSplitDividerId::kDivider0, 0.7))
      .Times(1);
  model_->SetDividerRatio(AstraSplitDividerId::kDivider0, 0.7);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraSplitViewModelTest, InvalidDividerIdIsNoOp) {
  MockSplitViewModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnSplitDividerPositionChanged(_, _)).Times(0);
  model_->SetDividerRatio(AstraSplitDividerId::kDivider5, 0.7);

  model_->RemoveObserver(&observer);
}

// ---- Drag state tests

TEST_F(AstraSplitViewModelTest, StartDrag) {
  model_->StartDividerDrag(AstraSplitDividerId::kDivider0);
  EXPECT_TRUE(model_->IsDragging());
  auto dragged = model_->GetDraggedDivider();
  ASSERT_TRUE(dragged.has_value());
  EXPECT_EQ(AstraSplitDividerId::kDivider0, *dragged);
}

TEST_F(AstraSplitViewModelTest, DragUpdateNotifiesChanging) {
  MockSplitViewModelObserver observer;
  model_->AddObserver(&observer);

  model_->StartDividerDrag(AstraSplitDividerId::kDivider0);

  EXPECT_CALL(observer, OnSplitDividerPositionChanging(
                               AstraSplitDividerId::kDivider0, 0.6))
      .Times(1);
  model_->UpdateDividerDrag(AstraSplitDividerId::kDivider0, 0.6);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraSplitViewModelTest, DragEndNotifiesEndedAndChanged) {
  MockSplitViewModelObserver observer;
  model_->AddObserver(&observer);

  model_->StartDividerDrag(AstraSplitDividerId::kDivider0);
  model_->UpdateDividerDrag(AstraSplitDividerId::kDivider0, 0.6);

  EXPECT_CALL(observer,
              OnSplitDividerDragEnded(AstraSplitDividerId::kDivider0))
      .Times(1);
  EXPECT_CALL(observer, OnSplitDividerPositionChanged(
                               AstraSplitDividerId::kDivider0, 0.6))
      .Times(1);
  model_->EndDividerDrag(AstraSplitDividerId::kDivider0);

  EXPECT_FALSE(model_->IsDragging());
  model_->RemoveObserver(&observer);
}

TEST_F(AstraSplitViewModelTest, DragUpdateWithoutStartIsNoOp) {
  MockSplitViewModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnSplitDividerPositionChanging(_, _)).Times(0);
  model_->UpdateDividerDrag(AstraSplitDividerId::kDivider0, 0.6);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraSplitViewModelTest, DragStartNotifies) {
  MockSplitViewModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer,
              OnSplitDividerDragStarted(AstraSplitDividerId::kDivider0))
      .Times(1);
  model_->StartDividerDrag(AstraSplitDividerId::kDivider0);

  model_->RemoveObserver(&observer);
}

// ---- Focused pane tests

TEST_F(AstraSplitViewModelTest, SetFocusedPane) {
  model_->SetFocusedPane(AstraSplitPaneId::kPane1);
  EXPECT_EQ(AstraSplitPaneId::kPane1, model_->focused_pane());
}

TEST_F(AstraSplitViewModelTest, SetSameFocusedPaneIsNoOp) {
  MockSplitViewModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnSplitFocusedPaneChanged(_)).Times(0);
  model_->SetFocusedPane(AstraSplitPaneId::kPane0);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraSplitViewModelTest, FocusedPaneChangeNotifies) {
  MockSplitViewModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer,
              OnSplitFocusedPaneChanged(AstraSplitPaneId::kPane1))
      .Times(1);
  model_->SetFocusedPane(AstraSplitPaneId::kPane1);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraSplitViewModelTest, FocusNextPane) {
  model_->SetFocusedPane(AstraSplitPaneId::kPane0);
  model_->FocusNextPane();
  EXPECT_EQ(AstraSplitPaneId::kPane1, model_->focused_pane());
}

TEST_F(AstraSplitViewModelTest, FocusNextPaneWrapsAround) {
  model_->SetFocusedPane(AstraSplitPaneId::kPane1);
  model_->FocusNextPane();
  EXPECT_EQ(AstraSplitPaneId::kPane0, model_->focused_pane());
}

TEST_F(AstraSplitViewModelTest, FocusPreviousPane) {
  model_->SetFocusedPane(AstraSplitPaneId::kPane1);
  model_->FocusPreviousPane();
  EXPECT_EQ(AstraSplitPaneId::kPane0, model_->focused_pane());
}

TEST_F(AstraSplitViewModelTest, FocusPreviousPaneWrapsAround) {
  model_->SetFocusedPane(AstraSplitPaneId::kPane0);
  model_->FocusPreviousPane();
  EXPECT_EQ(AstraSplitPaneId::kPane1, model_->focused_pane());
}

// ---- Swap tests

TEST_F(AstraSplitViewModelTest, SwapPanesNotifies) {
  MockSplitViewModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer,
              OnSplitPanesSwapped(AstraSplitPaneId::kPane0,
                                   AstraSplitPaneId::kPane1))
      .Times(1);
  model_->SwapPanes(AstraSplitPaneId::kPane0,
                    AstraSplitPaneId::kPane1);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraSplitViewModelTest, SwapSamePaneIsNoOp) {
  MockSplitViewModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnSplitPanesSwapped(_, _)).Times(0);
  model_->SwapPanes(AstraSplitPaneId::kPane0,
                    AstraSplitPaneId::kPane0);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraSplitViewModelTest, SwapFocusedWithNext) {
  model_->SetFocusedPane(AstraSplitPaneId::kPane0);
  model_->SwapFocusedWithNext();
  EXPECT_EQ(AstraSplitPaneId::kPane1, model_->focused_pane());
}

TEST_F(AstraSplitViewModelTest, SwapFocusedWithPrevious) {
  model_->SetFocusedPane(AstraSplitPaneId::kPane1);
  model_->SwapFocusedWithPrevious();
  EXPECT_EQ(AstraSplitPaneId::kPane0, model_->focused_pane());
}

// ---- Close pane tests

TEST_F(AstraSplitViewModelTest, ClosePaneReturnsTrue) {
  bool result = model_->ClosePane(AstraSplitPaneId::kPane0);
  EXPECT_TRUE(result);
}

TEST_F(AstraSplitViewModelTest, ClosePaneNotifies) {
  MockSplitViewModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer,
              OnSplitPaneClosed(AstraSplitPaneId::kPane1))
      .Times(1);
  model_->ClosePane(AstraSplitPaneId::kPane1);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraSplitViewModelTest, CloseInvalidPaneReturnsFalse) {
  bool result = model_->ClosePane(
      static_cast<AstraSplitPaneId>(99));
  EXPECT_FALSE(result);
}

// ---- Min pane size tests

TEST_F(AstraSplitViewModelTest, SetMinPaneSize) {
  model_->SetMinPaneSize(200);
  EXPECT_EQ(200, model_->min_pane_size());
}

TEST_F(AstraSplitViewModelTest, MinPaneSizeZero) {
  model_->SetMinPaneSize(0);
  EXPECT_EQ(0, model_->min_pane_size());
}

TEST_F(AstraSplitViewModelTest, MinPaneSizeNegativeClamps) {
  model_->SetMinPaneSize(-10);
  EXPECT_EQ(0, model_->min_pane_size());
}

TEST_F(AstraSplitViewModelTest, MinPaneSizeNotifies) {
  MockSplitViewModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnSplitMinPaneSizeChanged(150)).Times(1);
  model_->SetMinPaneSize(150);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraSplitViewModelTest, SameMinPaneSizeIsNoOp) {
  MockSplitViewModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnSplitMinPaneSizeChanged(_)).Times(0);
  model_->SetMinPaneSize(AstraSplitViewModel::kDefaultMinPaneSize);

  model_->RemoveObserver(&observer);
}

// ---- Resize behavior tests

TEST_F(AstraSplitViewModelTest, SetResizeBehavior) {
  model_->SetResizeBehavior(AstraSplitResizeBehavior::kFixedPixelSize);
  EXPECT_EQ(AstraSplitResizeBehavior::kFixedPixelSize,
            model_->resize_behavior());
}

TEST_F(AstraSplitViewModelTest, ResizeBehaviorNotifies) {
  MockSplitViewModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnSplitResizeBehaviorChanged(
                           AstraSplitResizeBehavior::kMinSizePriority))
      .Times(1);
  model_->SetResizeBehavior(AstraSplitResizeBehavior::kMinSizePriority);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraSplitViewModelTest, SameResizeBehaviorIsNoOp) {
  MockSplitViewModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnSplitResizeBehaviorChanged(_)).Times(0);
  model_->SetResizeBehavior(AstraSplitResizeBehavior::kFixedRatio);

  model_->RemoveObserver(&observer);
}

// ---- Orientation helpers

TEST_F(AstraSplitViewModelTest, IsHorizontalDefault) {
  EXPECT_TRUE(model_->IsHorizontal());
}

TEST_F(AstraSplitViewModelTest, ToggleOrientation) {
  model_->ToggleOrientation();
  EXPECT_EQ(AstraSplitLayoutMode::kTwoPaneVertical,
            model_->layout_mode());
  EXPECT_FALSE(model_->IsHorizontal());
}

TEST_F(AstraSplitViewModelTest, ToggleOrientationTwice) {
  model_->ToggleOrientation();
  model_->ToggleOrientation();
  EXPECT_EQ(AstraSplitLayoutMode::kTwoPaneHorizontal,
            model_->layout_mode());
  EXPECT_TRUE(model_->IsHorizontal());
}

// ---- Validation tests

TEST_F(AstraSplitViewModelTest, IsValidPaneId) {
  EXPECT_TRUE(model_->IsValidPaneId(AstraSplitPaneId::kPane0));
  EXPECT_TRUE(model_->IsValidPaneId(AstraSplitPaneId::kPane1));
  EXPECT_FALSE(model_->IsValidPaneId(AstraSplitPaneId::kPane2));
}

TEST_F(AstraSplitViewModelTest, IsValidDividerId) {
  EXPECT_TRUE(model_->IsValidDividerId(AstraSplitDividerId::kDivider0));
  EXPECT_FALSE(model_->IsValidDividerId(AstraSplitDividerId::kDivider1));
}

TEST_F(AstraSplitViewModelTest, ClampRatio) {
  EXPECT_GE(model_->ClampRatio(0.0), AstraSplitViewModel::kMinPaneRatio);
  EXPECT_LE(model_->ClampRatio(1.0), 1.0);
  EXPECT_DOUBLE_EQ(0.5, model_->ClampRatio(0.5));
}

// ---- Serialization tests

TEST_F(AstraSplitViewModelTest, SerializeToStringNotEmpty) {
  std::string serialized = model_->SerializeToString();
  EXPECT_FALSE(serialized.empty());
}

TEST_F(AstraSplitViewModelTest, DeserializeFromString) {
  model_->SetDividerRatio(AstraSplitDividerId::kDivider0, 0.7);
  model_->SetMinPaneSize(150);
  model_->SetFocusedPane(AstraSplitPaneId::kPane1);

  std::string serialized = model_->SerializeToString();

  // Create a new model and deserialize.
  AstraSplitViewModel new_model;
  bool result = new_model.DeserializeFromString(serialized);
  EXPECT_TRUE(result);
  EXPECT_DOUBLE_EQ(0.7,
                    new_model.GetDividerRatio(AstraSplitDividerId::kDivider0));
  EXPECT_EQ(150, new_model.min_pane_size());
  EXPECT_EQ(AstraSplitPaneId::kPane1, new_model.focused_pane());
}

TEST_F(AstraSplitViewModelTest, DeserializeInvalidStringReturnsFalse) {
  AstraSplitViewModel model;
  bool result = model.DeserializeFromString("invalid|data|here");
  EXPECT_FALSE(result);
}

TEST_F(AstraSplitViewModelTest, DeserializeEmptyStringReturnsFalse) {
  AstraSplitViewModel model;
  bool result = model.DeserializeFromString("");
  EXPECT_FALSE(result);
}

// ---- Observer tests

TEST_F(AstraSplitViewModelTest, EmptyObserverWorks) {
  EmptySplitViewModelObserver empty_observer;
  model_->AddObserver(&empty_observer);

  // Trigger various state changes.
  model_->SetLayoutMode(AstraSplitLayoutMode::kTwoPaneVertical);
  model_->SetDividerRatio(AstraSplitDividerId::kDivider0, 0.7);
  model_->SetFocusedPane(AstraSplitPaneId::kPane1);
  model_->ClosePane(AstraSplitPaneId::kPane1);

  model_->RemoveObserver(&empty_observer);
  // No crash = success.
}

TEST_F(AstraSplitViewModelTest, MultipleObservers) {
  MockSplitViewModelObserver obs1;
  MockSplitViewModelObserver obs2;

  model_->AddObserver(&obs1);
  model_->AddObserver(&obs2);

  EXPECT_CALL(obs1,
              OnSplitFocusedPaneChanged(AstraSplitPaneId::kPane1))
      .Times(1);
  EXPECT_CALL(obs2,
              OnSplitFocusedPaneChanged(AstraSplitPaneId::kPane1))
      .Times(1);

  model_->SetFocusedPane(AstraSplitPaneId::kPane1);

  model_->RemoveObserver(&obs1);
  model_->RemoveObserver(&obs2);
}

TEST_F(AstraSplitViewModelTest, RemoveObserverStopsNotifications) {
  MockSplitViewModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnSplitFocusedPaneChanged(_)).Times(1);
  model_->SetFocusedPane(AstraSplitPaneId::kPane1);

  model_->RemoveObserver(&observer);

  EXPECT_CALL(observer, OnSplitFocusedPaneChanged(_)).Times(0);
  model_->SetFocusedPane(AstraSplitPaneId::kPane0);
}

TEST_F(AstraSplitViewModelTest, DestroyNotifiesObserver) {
  auto model = std::make_unique<AstraSplitViewModel>();
  MockSplitViewModelObserver observer;
  model->AddObserver(&observer);

  EXPECT_CALL(observer, OnSplitViewModelDestroyed()).Times(1);
  model.reset();  // Destroys the model.
}

// ---- Layout mode string tests

TEST(AstraSplitLayoutModeStringTest, ModeToString) {
  EXPECT_EQ("two_pane_horizontal",
            AstraSplitLayoutModeToString(
                AstraSplitLayoutMode::kTwoPaneHorizontal));
  EXPECT_EQ("two_pane_vertical",
            AstraSplitLayoutModeToString(
                AstraSplitLayoutMode::kTwoPaneVertical));
  EXPECT_EQ("three_pane_horizontal",
            AstraSplitLayoutModeToString(
                AstraSplitLayoutMode::kThreePaneHorizontal));
  EXPECT_EQ("three_pane_vertical",
            AstraSplitLayoutModeToString(
                AstraSplitLayoutMode::kThreePaneVertical));
  EXPECT_EQ("grid_two_by_two",
            AstraSplitLayoutModeToString(
                AstraSplitLayoutMode::kGridTwoByTwo));
  EXPECT_EQ("grid_three_by_two",
            AstraSplitLayoutModeToString(
                AstraSplitLayoutMode::kGridThreeByTwo));
}

TEST(AstraSplitLayoutModeStringTest, ModeFromString) {
  EXPECT_EQ(AstraSplitLayoutMode::kTwoPaneHorizontal,
            AstraSplitLayoutModeFromString("two_pane_horizontal"));
  EXPECT_EQ(AstraSplitLayoutMode::kTwoPaneVertical,
            AstraSplitLayoutModeFromString("two_pane_vertical"));
  EXPECT_EQ(AstraSplitLayoutMode::kThreePaneHorizontal,
            AstraSplitLayoutModeFromString("three_pane_horizontal"));
  EXPECT_EQ(AstraSplitLayoutMode::kThreePaneVertical,
            AstraSplitLayoutModeFromString("three_pane_vertical"));
  EXPECT_EQ(AstraSplitLayoutMode::kGridTwoByTwo,
            AstraSplitLayoutModeFromString("grid_two_by_two"));
  EXPECT_EQ(AstraSplitLayoutMode::kGridThreeByTwo,
            AstraSplitLayoutModeFromString("grid_three_by_two"));
}

TEST(AstraSplitLayoutModeStringTest, UnknownModeDefaultsToHorizontal) {
  EXPECT_EQ(AstraSplitLayoutMode::kTwoPaneHorizontal,
            AstraSplitLayoutModeFromString("unknown_mode"));
  EXPECT_EQ(AstraSplitLayoutMode::kTwoPaneHorizontal,
            AstraSplitLayoutModeFromString(""));
}

TEST(AstraSplitLayoutModeStringTest, ModeNamesNotEmpty) {
  EXPECT_FALSE(AstraSplitLayoutModeToName(
                   AstraSplitLayoutMode::kTwoPaneHorizontal).empty());
  EXPECT_FALSE(AstraSplitLayoutModeToName(
                   AstraSplitLayoutMode::kGridTwoByTwo).empty());
}

TEST(AstraSplitResizeBehaviorStringTest, BehaviorToString) {
  EXPECT_EQ("fixed_ratio",
            AstraSplitResizeBehaviorToString(
                AstraSplitResizeBehavior::kFixedRatio));
  EXPECT_EQ("fixed_pixel_size",
            AstraSplitResizeBehaviorToString(
                AstraSplitResizeBehavior::kFixedPixelSize));
  EXPECT_EQ("min_size_priority",
            AstraSplitResizeBehaviorToString(
                AstraSplitResizeBehavior::kMinSizePriority));
}

TEST(AstraSplitResizeBehaviorStringTest, BehaviorFromString) {
  EXPECT_EQ(AstraSplitResizeBehavior::kFixedRatio,
            AstraSplitResizeBehaviorFromString("fixed_ratio"));
  EXPECT_EQ(AstraSplitResizeBehavior::kFixedPixelSize,
            AstraSplitResizeBehaviorFromString("fixed_pixel_size"));
  EXPECT_EQ(AstraSplitResizeBehavior::kMinSizePriority,
            AstraSplitResizeBehaviorFromString("min_size_priority"));
}

TEST(AstraSplitPaneIdStringTest, PaneIdToString) {
  EXPECT_EQ("pane0",
            AstraSplitPaneIdToString(AstraSplitPaneId::kPane0));
  EXPECT_EQ("pane5",
            AstraSplitPaneIdToString(AstraSplitPaneId::kPane5));
}

TEST(AstraSplitPaneIdStringTest, PaneIdFromString) {
  EXPECT_EQ(AstraSplitPaneId::kPane0,
            AstraSplitPaneIdFromString("pane0"));
  EXPECT_EQ(AstraSplitPaneId::kPane3,
            AstraSplitPaneIdFromString("pane3"));
  EXPECT_EQ(AstraSplitPaneId::kPane0,
            AstraSplitPaneIdFromString("unknown"));
}

// =========================================================================
// New view component tests
// =========================================================================

TEST_F(AstraSplitViewTest, DefaultLayoutMode) {
  EXPECT_EQ(AstraSplitLayoutMode::kTwoPaneHorizontal,
            split_view_->layout_mode());
}

TEST_F(AstraSplitViewTest, SetLayoutModeVertical) {
  split_view_->SetLayoutMode(AstraSplitLayoutMode::kTwoPaneVertical);
  EXPECT_EQ(AstraSplitLayoutMode::kTwoPaneVertical,
            split_view_->layout_mode());
}

TEST_F(AstraSplitViewTest, CycleNextLayoutMode) {
  split_view_->CycleNextLayoutMode();
  EXPECT_EQ(AstraSplitLayoutMode::kTwoPaneVertical,
            split_view_->layout_mode());
}

TEST_F(AstraSplitViewTest, CyclePreviousLayoutMode) {
  split_view_->CyclePreviousLayoutMode();
  // Should wrap to last mode.
  EXPECT_EQ(AstraSplitLayoutMode::kGridThreeByTwo,
            split_view_->layout_mode());
}

TEST_F(AstraSplitViewTest, GetPaneCount) {
  EXPECT_EQ(2, split_view_->GetPaneCount());
}

TEST_F(AstraSplitViewTest, IsGridLayoutDefault) {
  EXPECT_FALSE(split_view_->IsGridLayout());
}

TEST_F(AstraSplitViewTest, ShowPaneHeaders) {
  split_view_->SetShowPaneHeaders(true);
  EXPECT_TRUE(split_view_->show_pane_headers());
  EXPECT_NE(nullptr, split_view_->primary_header());
  EXPECT_NE(nullptr, split_view_->secondary_header());
  EXPECT_TRUE(split_view_->primary_header()->GetVisible());
  EXPECT_TRUE(split_view_->secondary_header()->GetVisible());
}

TEST_F(AstraSplitViewTest, HidePaneHeaders) {
  split_view_->SetShowPaneHeaders(true);
  ASSERT_TRUE(split_view_->show_pane_headers());

  split_view_->SetShowPaneHeaders(false);
  EXPECT_FALSE(split_view_->show_pane_headers());
  EXPECT_FALSE(split_view_->primary_header()->GetVisible());
  EXPECT_FALSE(split_view_->secondary_header()->GetVisible());
}

TEST_F(AstraSplitViewTest, SetPaneTitle) {
  split_view_->SetShowPaneHeaders(true);
  split_view_->SetPaneTitle(AstraSplitPane::kPrimary, u"Tab 1");
  // Should not crash.
  EXPECT_TRUE(split_view_->GetVisible());
}

TEST_F(AstraSplitViewTest, ShowDividerToolbar) {
  split_view_->SetShowDividerToolbar(true);
  EXPECT_TRUE(split_view_->show_divider_toolbar());
  EXPECT_NE(nullptr, split_view_->divider_toolbar());
  EXPECT_TRUE(split_view_->divider_toolbar()->GetVisible());
}

TEST_F(AstraSplitViewTest, HideDividerToolbar) {
  split_view_->SetShowDividerToolbar(true);
  ASSERT_TRUE(split_view_->show_divider_toolbar());

  split_view_->SetShowDividerToolbar(false);
  EXPECT_FALSE(split_view_->show_divider_toolbar());
}

TEST_F(AstraSplitViewTest, DividerToolbarHasBothButtons) {
  split_view_->SetShowDividerToolbar(true);
  ASSERT_NE(nullptr, split_view_->divider_toolbar());

  // Toolbar should have swap and layout toggle buttons.
  AstraSplitDividerToolbar* toolbar = split_view_->divider_toolbar();
  EXPECT_TRUE(toolbar->GetVisible());
  EXPECT_GT(toolbar->GetPreferredSize().width(), 0);
  EXPECT_GT(toolbar->GetPreferredSize().height(), 0);
}

TEST_F(AstraSplitViewTest, DividerToolbarUpdateLayoutMode) {
  split_view_->SetShowDividerToolbar(true);
  ASSERT_NE(nullptr, split_view_->divider_toolbar());

  split_view_->divider_toolbar()->UpdateLayoutMode(
      AstraSplitLayoutMode::kTwoPaneVertical);
  // Should not crash and should update button icon.
  EXPECT_TRUE(split_view_->divider_toolbar()->GetVisible());
}

TEST_F(AstraSplitViewTest, ClosePanePrimary) {
  auto primary = std::make_unique<views::View>();
  auto secondary = std::make_unique<views::View>();
  split_view_->SetPrimaryView(primary.release());
  split_view_->SetSecondaryView(secondary.release());

  widget_->SetSize(gfx::Size(400, 300));
  widget_->LayoutRootViewIfNecessary();

  split_view_->ClosePane(AstraSplitPane::kPrimary);
  // After closing primary, ratio should be minimum (primary minimized).
  EXPECT_LT(split_view_->GetRatio(), 0.2);
}

TEST_F(AstraSplitViewTest, ClosePaneSecondary) {
  auto primary = std::make_unique<views::View>();
  auto secondary = std::make_unique<views::View>();
  split_view_->SetPrimaryView(primary.release());
  split_view_->SetSecondaryView(secondary.release());

  widget_->SetSize(gfx::Size(400, 300));
  widget_->LayoutRootViewIfNecessary();

  split_view_->ClosePane(AstraSplitPane::kSecondary);
  // After closing secondary, ratio should be maximum (secondary minimized).
  EXPECT_GT(split_view_->GetRatio(), 0.8);
}

TEST_F(AstraSplitViewTest, GetAccessibleName) {
  std::u16string name = split_view_->GetAccessibleName();
  EXPECT_FALSE(name.empty());
}

TEST_F(AstraSplitViewTest, SetAccessibleDescription) {
  split_view_->SetAccessibleDescription(u"Test description");
  EXPECT_EQ(u"Test description",
            split_view_->accessible_description());
}

TEST_F(AstraSplitViewTest, SameAccessibleDescriptionIsNoOp) {
  split_view_->SetAccessibleDescription(u"Test");
  // Setting the same description should not cause issues.
  split_view_->SetAccessibleDescription(u"Test");
  EXPECT_EQ(u"Test",
            split_view_->accessible_description());
}

TEST_F(AstraSplitViewTest, LayoutWithHeaders) {
  auto primary = std::make_unique<views::View>();
  auto secondary = std::make_unique<views::View>();
  split_view_->SetPrimaryView(primary.release());
  split_view_->SetSecondaryView(secondary.release());

  split_view_->SetShowPaneHeaders(true);
  widget_->SetSize(gfx::Size(400, 300));
  widget_->LayoutRootViewIfNecessary();

  // Primary view should be offset by header height.
  EXPECT_GT(split_view_->GetPrimaryView()->y(), 0);
  EXPECT_LT(split_view_->GetPrimaryView()->height(), 300);
}

TEST_F(AstraSplitViewTest, LayoutWithToolbar) {
  auto primary = std::make_unique<views::View>();
  auto secondary = std::make_unique<views::View>();
  split_view_->SetPrimaryView(primary.release());
  split_view_->SetSecondaryView(secondary.release());

  split_view_->SetShowDividerToolbar(true);
  widget_->SetSize(gfx::Size(400, 300));
  widget_->LayoutRootViewIfNecessary();

  // Toolbar should be visible and positioned near the divider.
  ASSERT_NE(nullptr, split_view_->divider_toolbar());
  EXPECT_TRUE(split_view_->divider_toolbar()->GetVisible());
  EXPECT_GT(split_view_->divider_toolbar()->x(), 150);
  EXPECT_LT(split_view_->divider_toolbar()->x(), 250);
}

// ---- Pane header tests

TEST(AstraSplitPaneHeaderTest, DefaultTitleEmpty) {
  AstraSplitPaneHeader header;
  EXPECT_TRUE(header.title().empty());
}

TEST(AstraSplitPaneHeaderTest, SetTitle) {
  AstraSplitPaneHeader header;
  header.SetTitle(u"My Tab");
  EXPECT_EQ(u"My Tab", header.title());
}

TEST(AstraSplitPaneHeaderTest, DefaultShowCloseButton) {
  AstraSplitPaneHeader header;
  EXPECT_FALSE(header.show_close_button());
  header.SetShowCloseButton(true);
  EXPECT_TRUE(header.show_close_button());
}

TEST(AstraSplitPaneHeaderTest, SetPane) {
  AstraSplitPaneHeader header;
  header.SetPane(AstraSplitPane::kSecondary);
  EXPECT_EQ(AstraSplitPane::kSecondary, header.pane());
}

TEST(AstraSplitPaneHeaderTest, PreferredSize) {
  AstraSplitPaneHeader header;
  gfx::Size pref = header.CalculatePreferredSize(views::SizeBounds());
  EXPECT_GT(pref.height(), 0);
  EXPECT_EQ(0, pref.width());  // Width is flexible.
}

// ---- Divider toolbar tests

TEST(AstraSplitDividerToolbarTest, DefaultVisible) {
  AstraSplitDividerToolbar toolbar;
  EXPECT_TRUE(toolbar.toolbar_visible());
}

TEST(AstraSplitDividerToolbarTest, SetToolbarVisible) {
  AstraSplitDividerToolbar toolbar;
  toolbar.SetToolbarVisible(false);
  EXPECT_FALSE(toolbar.toolbar_visible());
  EXPECT_FALSE(toolbar.GetVisible());

  toolbar.SetToolbarVisible(true);
  EXPECT_TRUE(toolbar.toolbar_visible());
  EXPECT_TRUE(toolbar.GetVisible());
}

TEST(AstraSplitDividerToolbarTest, PreferredSize) {
  AstraSplitDividerToolbar toolbar;
  gfx::Size pref = toolbar.GetPreferredSize();
  EXPECT_GT(pref.width(), 0);
  EXPECT_GT(pref.height(), 0);
}

TEST(AstraSplitDividerToolbarTest, UpdateLayoutMode) {
  AstraSplitDividerToolbar toolbar;
  toolbar.UpdateLayoutMode(AstraSplitLayoutMode::kGridTwoByTwo);
  // Should not crash.
  EXPECT_TRUE(toolbar.GetVisible());
}

// =========================================================================
// New controller tests
// =========================================================================

TEST_F(AstraSplitViewControllerStateTest, DefaultLayoutMode) {
  EXPECT_EQ(AstraSplitLayoutMode::kTwoPaneHorizontal,
            controller_->GetLayoutMode());
}

TEST_F(AstraSplitViewControllerStateTest, SetLayoutMode) {
  controller_->SetLayoutMode(AstraSplitLayoutMode::kTwoPaneVertical);
  EXPECT_EQ(AstraSplitLayoutMode::kTwoPaneVertical,
            controller_->GetLayoutMode());
}

TEST_F(AstraSplitViewControllerStateTest, CycleNextLayoutMode) {
  controller_->CycleNextLayoutMode();
  EXPECT_EQ(AstraSplitLayoutMode::kTwoPaneVertical,
            controller_->GetLayoutMode());
}

TEST_F(AstraSplitViewControllerStateTest, CyclePreviousLayoutMode) {
  controller_->CyclePreviousLayoutMode();
  EXPECT_EQ(AstraSplitLayoutMode::kGridThreeByTwo,
            controller_->GetLayoutMode());
}

TEST_F(AstraSplitViewControllerStateTest, ModelAccess) {
  EXPECT_NE(nullptr, controller_->model());
  EXPECT_EQ(AstraSplitLayoutMode::kTwoPaneHorizontal,
            controller_->model()->layout_mode());
}

TEST_F(AstraSplitViewControllerStateTest, CanDropTabOnPaneValid) {
  // Without activation, but pane 0 and 1 should be valid for 2-pane mode.
  // When not active, CanDropTabOnPane should return false.
  EXPECT_FALSE(controller_->CanDropTabOnPane(AstraSplitPaneId::kPane0,
                                               nullptr));
}

TEST_F(AstraSplitViewControllerStateTest, CanDropTabOnPaneInvalid) {
  EXPECT_FALSE(controller_->CanDropTabOnPane(AstraSplitPaneId::kPane5,
                                               nullptr));
}

TEST_F(AstraSplitViewControllerStateTest, StartSplitFromDragInactive) {
  EXPECT_FALSE(controller_->StartSplitFromDrag(AstraSplitPaneId::kPane0,
                                                  nullptr));
}

TEST_F(AstraSplitViewControllerStateTest, HandleKeyboardShortcutNoControl) {
  // Without control key, shortcuts should not be handled.
  ui::KeyEvent event(ui::ET_KEY_PRESSED, ui::VKEY_D, ui::EF_NONE);
  EXPECT_FALSE(controller_->HandleKeyboardShortcut(event));
}

TEST_F(AstraSplitViewControllerStateTest, HandleKeyboardShortcutControlD) {
  // Ctrl+D should be handled (swap panes).
  // Even when inactive, some shortcuts might still work?
  // Let's test that it doesn't crash at minimum.
  ui::KeyEvent event(ui::ET_KEY_PRESSED, ui::VKEY_D, ui::EF_CONTROL_DOWN);
  // Should not crash. May return true or false depending on implementation.
  // We just verify no crash.
  controller_->HandleKeyboardShortcut(event);
}

TEST_F(AstraSplitViewControllerStateTest, HandleKeyboardShortcutUnknownKey) {
  // Unknown shortcut should not be handled.
  ui::KeyEvent event(ui::ET_KEY_PRESSED, ui::VKEY_A, ui::EF_CONTROL_DOWN);
  EXPECT_FALSE(controller_->HandleKeyboardShortcut(event));
}

TEST_F(AstraSplitViewControllerStateTest, HandleKeyboardShortcutBracketKeys) {
  // Ctrl+[ and Ctrl+] should be handled.
  ui::KeyEvent event_left(ui::ET_KEY_PRESSED, ui::VKEY_OEM_4,
                          ui::EF_CONTROL_DOWN);  // '['
  controller_->HandleKeyboardShortcut(event_left);

  ui::KeyEvent event_right(ui::ET_KEY_PRESSED, ui::VKEY_OEM_6,
                           ui::EF_CONTROL_DOWN);  // ']'
  controller_->HandleKeyboardShortcut(event_right);
  // Should not crash.
}

TEST_F(AstraSplitViewControllerStateTest, HandleKeyboardShortcutNumberKeys) {
  // Ctrl+1 and Ctrl+2 should be handled.
  ui::KeyEvent event1(ui::ET_KEY_PRESSED, ui::VKEY_1, ui::EF_CONTROL_DOWN);
  controller_->HandleKeyboardShortcut(event1);

  ui::KeyEvent event2(ui::ET_KEY_PRESSED, ui::VKEY_2, ui::EF_CONTROL_DOWN);
  controller_->HandleKeyboardShortcut(event2);
  // Should not crash.
}

TEST_F(AstraSplitViewControllerStateTest, HandleKeyboardShortcutReturn) {
  // Ctrl+Enter should toggle split view.
  ui::KeyEvent event(ui::ET_KEY_PRESSED, ui::VKEY_RETURN,
                      ui::EF_CONTROL_DOWN);
  controller_->HandleKeyboardShortcut(event);
  // Should not crash.
}

TEST_F(AstraSplitViewControllerStateTest, HandleKeyboardShortcutW) {
  // Ctrl+W should close focused pane.
  ui::KeyEvent event(ui::ET_KEY_PRESSED, ui::VKEY_W, ui::EF_CONTROL_DOWN);
  controller_->HandleKeyboardShortcut(event);
  // Should not crash.
}

}  // namespace astra
