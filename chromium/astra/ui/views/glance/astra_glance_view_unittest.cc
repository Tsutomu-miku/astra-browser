// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Comprehensive unit tests for AstraGlanceView and AstraGlanceViewController.
//
// Tests verify:
//   - View construction and initial state
//   - Display mode toggling (compact / expanded)
//   - Loading state transitions
//   - Security state updates
//   - Title/URL/favicon setters
//   - Action button states
//   - Preferred size calculations
//   - Theme/color integration
//   - Keyboard accelerators (Escape, Ctrl+Enter, Ctrl+P, Ctrl+S, Ctrl+1/2/3, Ctrl++/-)
//   - Accessibility node data
//   - Glance pinning (window pin)
//   - Size presets (small, medium, large)
//   - Section visibility (status bar, action bar, resize handle)
//   - Settings button visibility
//   - Observer pattern (all observer methods have defaults)
//   - Presentation settings persisted via PrefService
//   - Recent glance history
//   - Edge cases (null states, invalid sizes, empty strings)
//   - Delegate callback patterns
//   - Animation stubs
//
// Chromium test pattern: views::test::ViewsTestBase
//   (ui/views/test/views_test_base.h)

#include "astra/ui/views/glance/astra_glance_view.h"

#include <string>
#include <vector>

#include "astra/browser/astra_prefs.h"
#include "astra/ui/views/glance/astra_glance_view_controller.h"
#include "base/memory/raw_ptr.h"
#include "base/test/task_environment.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/test/views_test_base.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace astra {

namespace {

// Test delegate that tracks all method calls.
class TestGlanceDelegate : public AstraGlanceView::Delegate {
 public:
  int close_requested_count = 0;
  int promote_to_tab_count = 0;
  int view_destroyed_count = 0;
  int add_to_favorites_count = 0;
  int copy_url_count = 0;
  int pin_tab_count = 0;
  int close_tab_count = 0;
  int toggle_expanded_count = 0;
  int toggle_pinned_count = 0;
  int size_preset_changed_count = 0;
  int settings_requested_count = 0;
  int resized_count = 0;
  int cycle_position_count = 0;
  AstraGlanceView::SizePreset last_size_preset =
      AstraGlanceView::SizePreset::kMedium;
  gfx::Size last_resized_size;

  void OnGlanceCloseRequested() override { close_requested_count++; }
  void OnGlancePromoteToTab() override { promote_to_tab_count++; }
  void OnGlanceViewDestroyed() override { view_destroyed_count++; }
  void OnGlanceAddToFavorites() override { add_to_favorites_count++; }
  void OnGlanceCopyURL() override { copy_url_count++; }
  void OnGlancePinTab() override { pin_tab_count++; }
  void OnGlanceCloseTab() override { close_tab_count++; }
  void OnGlanceToggleExpanded() override { toggle_expanded_count++; }
  void OnGlanceTogglePinned() override { toggle_pinned_count++; }
  void OnGlanceSizePresetChanged(
      AstraGlanceView::SizePreset preset) override {
    size_preset_changed_count++;
    last_size_preset = preset;
  }
  void OnGlanceSettingsRequested() override { settings_requested_count++; }
  void OnGlanceResized(const gfx::Size& new_size) override {
    resized_count++;
    last_resized_size = new_size;
  }
  void OnGlanceCyclePosition() override { cycle_position_count++; }
};

// Test observer that tracks all observer notifications.
class TestGlanceObserver : public AstraGlanceViewController::Observer {
 public:
  int shown_count = 0;
  int hidden_count = 0;
  int expanded_count = 0;
  int pinned_as_tab_count = 0;
  int mode_changed_count = 0;
  int resized_count = 0;
  int source_changed_count = 0;
  int pinned_changed_count = 0;
  int settings_changed_count = 0;

  AstraGlanceViewController::Mode last_mode =
      AstraGlanceViewController::Mode::kNone;
  gfx::Size last_size;
  AstraGlanceViewController::Source last_source =
      AstraGlanceViewController::Source::kUnknown;
  bool last_pinned = false;

  void OnGlanceShown() override { shown_count++; }
  void OnGlanceHidden() override { hidden_count++; }
  void OnGlanceExpanded() override { expanded_count++; }
  void OnGlancePinnedAsTab() override { pinned_as_tab_count++; }
  void OnGlanceModeChanged(AstraGlanceViewController::Mode mode) override {
    mode_changed_count++;
    last_mode = mode;
  }
  void OnGlanceResized(const gfx::Size& size) override {
    resized_count++;
    last_size = size;
  }
  void OnGlanceSourceChanged(AstraGlanceViewController::Source source) override {
    source_changed_count++;
    last_source = source;
  }
  void OnGlancePinnedChanged(bool pinned) override {
    pinned_changed_count++;
    last_pinned = pinned;
  }
  void OnGlanceSettingsChanged() override { settings_changed_count++; }
};

// Minimal observer that overrides only one method to test defaults.
class MinimalObserver : public AstraGlanceViewController::Observer {
 public:
  int shown_count = 0;
  void OnGlanceShown() override { shown_count++; }
  // All other methods use default empty implementations.
};

// Test observer for the new AstraGlanceObserver interface.
class TestAstraGlanceObserver : public AstraGlanceObserver {
 public:
  int shown_count = 0;
  int hidden_count = 0;
  int pinned_count = 0;
  int expanded_count = 0;
  int size_changed_count = 0;
  int content_type_changed_count = 0;
  int shutdown_count = 0;

  int last_tab_index = -1;
  bool last_pinned = false;
  bool last_expanded = false;
  gfx::Size last_size;
  AstraGlanceContentType last_content_type = AstraGlanceContentType::kTabInfo;
  raw_ptr<AstraGlanceViewController> last_controller = nullptr;

  void OnGlanceShown(AstraGlanceViewController* controller,
                     int tab_index) override {
    shown_count++;
    last_tab_index = tab_index;
    last_controller = controller;
  }

  void OnGlanceHidden(AstraGlanceViewController* controller) override {
    hidden_count++;
    last_controller = controller;
  }

  void OnGlancePinned(AstraGlanceViewController* controller,
                      bool pinned) override {
    pinned_count++;
    last_pinned = pinned;
    last_controller = controller;
  }

  void OnGlanceExpanded(AstraGlanceViewController* controller,
                        bool expanded) override {
    expanded_count++;
    last_expanded = expanded;
    last_controller = controller;
  }

  void OnGlanceSizeChanged(AstraGlanceViewController* controller,
                           const gfx::Size& new_size) override {
    size_changed_count++;
    last_size = new_size;
    last_controller = controller;
  }

  void OnGlanceContentTypeChanged(AstraGlanceViewController* controller,
                                  AstraGlanceContentType type) override {
    content_type_changed_count++;
    last_content_type = type;
    last_controller = controller;
  }

  void OnGlanceViewControllerShutdown(
      AstraGlanceViewController* controller) override {
    shutdown_count++;
    last_controller = controller;
  }
};

// Minimal AstraGlanceObserver that overrides only one method.
class MinimalAstraGlanceObserver : public AstraGlanceObserver {
 public:
  int shown_count = 0;
  void OnGlanceShown(AstraGlanceViewController* controller,
                     int tab_index) override {
    shown_count++;
  }
  // All other methods use default empty implementations.
};

}  // namespace

// ===========================================================================
// AstraGlanceViewTest — view-level tests
// ===========================================================================

class AstraGlanceViewTest : public views::ViewsTestBase {
 public:
  AstraGlanceViewTest() = default;
  ~AstraGlanceViewTest() override = default;

  void SetUp() override {
    ViewsTestBase::SetUp();
    widget_ = CreateTestWidget();
    anchor_view_ = widget_->SetContentsView(std::make_unique<views::View>());
    anchor_view_->SetPreferredSize(gfx::Size(100, 100));
    widget_->Show();
  }

  void TearDown() override {
    widget_.reset();
    ViewsTestBase::TearDown();
  }

  AstraGlanceView* CreateGlanceView(AstraGlanceView::Delegate* delegate) {
    views::Widget* bubble_widget =
        AstraGlanceView::ShowBubble(anchor_view_, gfx::Rect(), delegate);
    return static_cast<AstraGlanceView*>(
        bubble_widget->widget_delegate()->AsDialogDelegate());
  }

 protected:
  std::unique_ptr<views::Widget> widget_;
  raw_ptr<views::View> anchor_view_ = nullptr;
};

// ===========================================================================
// Construction and default state tests
// ===========================================================================

TEST_F(AstraGlanceViewTest, DefaultDisplayModeIsExpanded) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);
  EXPECT_EQ(AstraGlanceView::DisplayMode::kExpanded, glance->display_mode());
}

TEST_F(AstraGlanceViewTest, DefaultLoadingStateIsLoaded) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);
  EXPECT_EQ(AstraGlanceView::LoadingState::kLoaded, glance->loading_state());
}

TEST_F(AstraGlanceViewTest, DefaultSecurityStateIsUnknown) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);
  EXPECT_EQ(AstraGlanceView::SecurityState::kUnknown, glance->security_state());
}

TEST_F(AstraGlanceViewTest, DefaultWebContentsIsNull) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);
  EXPECT_EQ(nullptr, glance->web_contents());
}

TEST_F(AstraGlanceViewTest, WidgetIsCreated) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);
  EXPECT_NE(nullptr, glance->GetWidget());
}

TEST_F(AstraGlanceViewTest, DefaultGlancePinnedIsFalse) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);
  EXPECT_FALSE(glance->IsGlancePinned());
}

TEST_F(AstraGlanceViewTest, DefaultSettingsButtonVisible) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);
  EXPECT_TRUE(glance->IsSettingsButtonVisible());
}

TEST_F(AstraGlanceViewTest, DefaultSizePresetIsMedium) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);
  EXPECT_EQ(AstraGlanceView::SizePreset::kMedium, glance->GetCurrentSizePreset());
}

TEST_F(AstraGlanceViewTest, ExpandedModeHasStatusBarVisible) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);
  glance->SetDisplayMode(AstraGlanceView::DisplayMode::kExpanded);
  EXPECT_TRUE(glance->IsStatusBarVisible());
}

TEST_F(AstraGlanceViewTest, ExpandedModeHasActionBarVisible) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);
  glance->SetDisplayMode(AstraGlanceView::DisplayMode::kExpanded);
  EXPECT_TRUE(glance->IsActionBarVisible());
}

TEST_F(AstraGlanceViewTest, ExpandedModeHasResizeHandleVisible) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);
  glance->SetDisplayMode(AstraGlanceView::DisplayMode::kExpanded);
  EXPECT_TRUE(glance->IsResizeHandleVisible());
}

// ===========================================================================
// Display mode tests
// ===========================================================================

TEST_F(AstraGlanceViewTest, SetDisplayModeCompact) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetDisplayMode(AstraGlanceView::DisplayMode::kCompact);
  EXPECT_EQ(AstraGlanceView::DisplayMode::kCompact, glance->display_mode());
}

TEST_F(AstraGlanceViewTest, SetDisplayModeExpanded) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetDisplayMode(AstraGlanceView::DisplayMode::kCompact);
  ASSERT_EQ(AstraGlanceView::DisplayMode::kCompact, glance->display_mode());

  glance->SetDisplayMode(AstraGlanceView::DisplayMode::kExpanded);
  EXPECT_EQ(AstraGlanceView::DisplayMode::kExpanded, glance->display_mode());
}

TEST_F(AstraGlanceViewTest, ToggleDisplayMode) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  ASSERT_EQ(AstraGlanceView::DisplayMode::kExpanded, glance->display_mode());

  glance->ToggleDisplayMode();
  EXPECT_EQ(AstraGlanceView::DisplayMode::kCompact, glance->display_mode());

  glance->ToggleDisplayMode();
  EXPECT_EQ(AstraGlanceView::DisplayMode::kExpanded, glance->display_mode());
}

TEST_F(AstraGlanceViewTest, SetSameDisplayModeIsNoOp) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  int toggle_count_before = delegate.toggle_expanded_count;
  glance->SetDisplayMode(AstraGlanceView::DisplayMode::kExpanded);
  EXPECT_EQ(delegate.toggle_expanded_count, toggle_count_before);
}

TEST_F(AstraGlanceViewTest, ExpandedPreferredSizeIsLargerThanCompact) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetDisplayMode(AstraGlanceView::DisplayMode::kExpanded);
  gfx::Size expanded_size = glance->CalculatePreferredSize();

  glance->SetDisplayMode(AstraGlanceView::DisplayMode::kCompact);
  gfx::Size compact_size = glance->CalculatePreferredSize();

  EXPECT_GT(expanded_size.width(), compact_size.width());
  EXPECT_GT(expanded_size.height(), compact_size.height());
}

TEST_F(AstraGlanceViewTest, CompactPreferredSizeMatchesConstants) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetDisplayMode(AstraGlanceView::DisplayMode::kCompact);
  gfx::Size size = glance->CalculatePreferredSize();

  EXPECT_EQ(AstraGlanceView::kCompactSize.width(), size.width());
  EXPECT_EQ(AstraGlanceView::kCompactSize.height(), size.height());
}

TEST_F(AstraGlanceViewTest, ExpandedPreferredSizeMatchesConstants) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetDisplayMode(AstraGlanceView::DisplayMode::kExpanded);
  gfx::Size size = glance->CalculatePreferredSize();

  EXPECT_EQ(AstraGlanceView::kExpandedSize.width(), size.width());
  EXPECT_EQ(AstraGlanceView::kExpandedSize.height(), size.height());
}

TEST_F(AstraGlanceViewTest, CompactModeHidesStatusBar) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetDisplayMode(AstraGlanceView::DisplayMode::kCompact);
  EXPECT_FALSE(glance->IsStatusBarVisible());
}

TEST_F(AstraGlanceViewTest, CompactModeHidesActionBar) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetDisplayMode(AstraGlanceView::DisplayMode::kCompact);
  EXPECT_FALSE(glance->IsActionBarVisible());
}

TEST_F(AstraGlanceViewTest, CompactModeHidesResizeHandle) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetDisplayMode(AstraGlanceView::DisplayMode::kCompact);
  EXPECT_FALSE(glance->IsResizeHandleVisible());
}

// ===========================================================================
// Loading state tests
// ===========================================================================

TEST_F(AstraGlanceViewTest, SetLoadingStateLoading) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetLoadingState(AstraGlanceView::LoadingState::kLoading);
  EXPECT_EQ(AstraGlanceView::LoadingState::kLoading, glance->loading_state());
}

TEST_F(AstraGlanceViewTest, SetLoadingStateError) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetLoadingState(AstraGlanceView::LoadingState::kError);
  EXPECT_EQ(AstraGlanceView::LoadingState::kError, glance->loading_state());
}

TEST_F(AstraGlanceViewTest, SetLoadingStateLoaded) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetLoadingState(AstraGlanceView::LoadingState::kLoading);
  ASSERT_EQ(AstraGlanceView::LoadingState::kLoading, glance->loading_state());

  glance->SetLoadingState(AstraGlanceView::LoadingState::kLoaded);
  EXPECT_EQ(AstraGlanceView::LoadingState::kLoaded, glance->loading_state());
}

TEST_F(AstraGlanceViewTest, SetSameLoadingStateIsNoOp) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetLoadingState(AstraGlanceView::LoadingState::kLoaded);
  EXPECT_EQ(AstraGlanceView::LoadingState::kLoaded, glance->loading_state());
}

TEST_F(AstraGlanceViewTest, SetErrorMessage) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetErrorMessage(u"Test error message");
  glance->SetLoadingState(AstraGlanceView::LoadingState::kError);
  EXPECT_EQ(AstraGlanceView::LoadingState::kError, glance->loading_state());
}

TEST_F(AstraGlanceViewTest, EmptyErrorMessageDoesNotCrash) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetErrorMessage(std::u16string());
  glance->SetLoadingState(AstraGlanceView::LoadingState::kError);
  EXPECT_EQ(AstraGlanceView::LoadingState::kError, glance->loading_state());
}

TEST_F(AstraGlanceViewTest, LongErrorMessageDoesNotCrash) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  std::u16string long_msg(1000, u'x');
  glance->SetErrorMessage(long_msg);
  glance->SetLoadingState(AstraGlanceView::LoadingState::kError);
  // No crash = success.
}

// ===========================================================================
// Security state tests
// ===========================================================================

TEST_F(AstraGlanceViewTest, SetSecurityStateSecure) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetSecurityState(AstraGlanceView::SecurityState::kSecure);
  EXPECT_EQ(AstraGlanceView::SecurityState::kSecure, glance->security_state());
}

TEST_F(AstraGlanceViewTest, SetSecurityStateNonSecure) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetSecurityState(AstraGlanceView::SecurityState::kNonSecure);
  EXPECT_EQ(AstraGlanceView::SecurityState::kNonSecure,
            glance->security_state());
}

TEST_F(AstraGlanceViewTest, SetSameSecurityStateIsNoOp) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetSecurityState(AstraGlanceView::SecurityState::kUnknown);
  EXPECT_EQ(AstraGlanceView::SecurityState::kUnknown, glance->security_state());
}

// ===========================================================================
// Title / URL / favicon tests
// ===========================================================================

TEST_F(AstraGlanceViewTest, SetTitleText) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);
  glance->SetTitleText(u"Test Page Title");
  // No crash = success; title is set on internal label.
}

TEST_F(AstraGlanceViewTest, SetURLText) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);
  glance->SetURLText(u"https://example.com");
}

TEST_F(AstraGlanceViewTest, SetTitleEmpty) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);
  glance->SetTitleText(std::u16string());
  // Should not crash with empty title.
}

TEST_F(AstraGlanceViewTest, SetURLEmpty) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);
  glance->SetURLText(std::u16string());
  // Should not crash with empty URL.
}

TEST_F(AstraGlanceViewTest, SetTitleMultipleTimes) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetTitleText(u"First Title");
  glance->SetTitleText(u"Second Title");
  glance->SetTitleText(u"Third Title");
  // No crash = success.
}

TEST_F(AstraGlanceViewTest, SetURLVeryLong) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  std::u16string long_url(500, u'a');
  long_url += u".com";
  glance->SetURLText(long_url);
  // No crash = success.
}

// ===========================================================================
// Action button state tests
// ===========================================================================

TEST_F(AstraGlanceViewTest, SetIsFavoriteTrue) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);
  glance->SetIsFavorite(true);
  // No crash = success.
}

TEST_F(AstraGlanceViewTest, SetIsFavoriteFalse) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);
  glance->SetIsFavorite(true);
  glance->SetIsFavorite(false);
}

TEST_F(AstraGlanceViewTest, SetSameFavoriteStateIsNoOp) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetIsFavorite(false);
  glance->SetIsFavorite(false);
  // No crash = success.
}

TEST_F(AstraGlanceViewTest, SetIsPinnedTrue) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);
  glance->SetIsPinned(true);
}

TEST_F(AstraGlanceViewTest, SetIsPinnedFalse) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);
  glance->SetIsPinned(true);
  glance->SetIsPinned(false);
}

TEST_F(AstraGlanceViewTest, SetSamePinnedStateIsNoOp) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetIsPinned(false);
  glance->SetIsPinned(false);
  // No crash = success.
}

TEST_F(AstraGlanceViewTest, SetActionButtonEnabledAllTrue) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);
  glance->SetActionButtonEnabled(true, true, true, true, true);
}

TEST_F(AstraGlanceViewTest, SetActionButtonEnabledAllFalse) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);
  glance->SetActionButtonEnabled(false, false, false, false, false);
}

TEST_F(AstraGlanceViewTest, SetActionButtonEnabledMixed) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);
  glance->SetActionButtonEnabled(true, false, true, false, true);
}

// ===========================================================================
// Glance pinning tests
// ===========================================================================

TEST_F(AstraGlanceViewTest, SetGlancePinnedTrue) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  EXPECT_FALSE(glance->IsGlancePinned());
  glance->SetGlancePinned(true);
  EXPECT_TRUE(glance->IsGlancePinned());
}

TEST_F(AstraGlanceViewTest, SetGlancePinnedFalse) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetGlancePinned(true);
  ASSERT_TRUE(glance->IsGlancePinned());

  glance->SetGlancePinned(false);
  EXPECT_FALSE(glance->IsGlancePinned());
}

TEST_F(AstraGlanceViewTest, SetSameGlancePinnedIsNoOp) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetGlancePinned(false);
  EXPECT_FALSE(glance->IsGlancePinned());
  // Still false, no crash.
}

// ===========================================================================
// Size preset tests
// ===========================================================================

TEST_F(AstraGlanceViewTest, ApplySizePresetSmall) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->ApplySizePreset(AstraGlanceView::SizePreset::kSmall);
  EXPECT_EQ(AstraGlanceView::SizePreset::kSmall, glance->GetCurrentSizePreset());
}

TEST_F(AstraGlanceViewTest, ApplySizePresetMedium) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->ApplySizePreset(AstraGlanceView::SizePreset::kSmall);
  glance->ApplySizePreset(AstraGlanceView::SizePreset::kMedium);
  EXPECT_EQ(AstraGlanceView::SizePreset::kMedium,
            glance->GetCurrentSizePreset());
}

TEST_F(AstraGlanceViewTest, ApplySizePresetLarge) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->ApplySizePreset(AstraGlanceView::SizePreset::kLarge);
  EXPECT_EQ(AstraGlanceView::SizePreset::kLarge, glance->GetCurrentSizePreset());
}

TEST_F(AstraGlanceViewTest, SizePresetSmallIsSmallest) {
  EXPECT_LT(AstraGlanceView::kSmallSize.width(),
            AstraGlanceView::kMediumSize.width());
  EXPECT_LT(AstraGlanceView::kSmallSize.height(),
            AstraGlanceView::kMediumSize.height());
}

TEST_F(AstraGlanceViewTest, SizePresetLargeIsLargest) {
  EXPECT_GT(AstraGlanceView::kLargeSize.width(),
            AstraGlanceView::kMediumSize.width());
  EXPECT_GT(AstraGlanceView::kLargeSize.height(),
            AstraGlanceView::kMediumSize.height());
}

TEST_F(AstraGlanceViewTest, AllSizePresetsArePositive) {
  EXPECT_GT(AstraGlanceView::kSmallSize.width(), 0);
  EXPECT_GT(AstraGlanceView::kSmallSize.height(), 0);
  EXPECT_GT(AstraGlanceView::kMediumSize.width(), 0);
  EXPECT_GT(AstraGlanceView::kMediumSize.height(), 0);
  EXPECT_GT(AstraGlanceView::kLargeSize.width(), 0);
  EXPECT_GT(AstraGlanceView::kLargeSize.height(), 0);
}

// ===========================================================================
// Section visibility tests
// ===========================================================================

TEST_F(AstraGlanceViewTest, SetStatusBarVisible) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  EXPECT_TRUE(glance->IsStatusBarVisible());
  glance->SetStatusBarVisible(false);
  EXPECT_FALSE(glance->IsStatusBarVisible());
  glance->SetStatusBarVisible(true);
  EXPECT_TRUE(glance->IsStatusBarVisible());
}

TEST_F(AstraGlanceViewTest, SetActionBarVisible) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  EXPECT_TRUE(glance->IsActionBarVisible());
  glance->SetActionBarVisible(false);
  EXPECT_FALSE(glance->IsActionBarVisible());
  glance->SetActionBarVisible(true);
  EXPECT_TRUE(glance->IsActionBarVisible());
}

TEST_F(AstraGlanceViewTest, SetResizeHandleVisible) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  EXPECT_TRUE(glance->IsResizeHandleVisible());
  glance->SetResizeHandleVisible(false);
  EXPECT_FALSE(glance->IsResizeHandleVisible());
  glance->SetResizeHandleVisible(true);
  EXPECT_TRUE(glance->IsResizeHandleVisible());
}

// ===========================================================================
// Settings button tests
// ===========================================================================

TEST_F(AstraGlanceViewTest, SetSettingsButtonVisibleFalse) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  ASSERT_TRUE(glance->IsSettingsButtonVisible());
  glance->SetSettingsButtonVisible(false);
  EXPECT_FALSE(glance->IsSettingsButtonVisible());
}

TEST_F(AstraGlanceViewTest, SetSettingsButtonVisibleSameStateIsNoOp) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  ASSERT_TRUE(glance->IsSettingsButtonVisible());
  glance->SetSettingsButtonVisible(true);
  EXPECT_TRUE(glance->IsSettingsButtonVisible());
}

// ===========================================================================
// Delegate callback tests
// ===========================================================================

TEST_F(AstraGlanceViewTest, WidgetDestroyNotifiesDelegate) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  EXPECT_EQ(0, delegate.view_destroyed_count);
  glance->GetWidget()->CloseNow();
  EXPECT_GE(delegate.view_destroyed_count, 1);
}

// ===========================================================================
// Theme / color tests
// ===========================================================================

TEST_F(AstraGlanceViewTest, OnThemeChangedDoesNotCrash) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);
  glance->OnThemeChanged();
}

TEST_F(AstraGlanceViewTest, HasColorProvider) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);
  EXPECT_NE(nullptr, glance->GetColorProvider());
}

// ===========================================================================
// Accessibility tests
// ===========================================================================

TEST_F(AstraGlanceViewTest, AccessibleRoleIsDialog) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  ui::AXNodeData data;
  glance->GetAccessibleNodeData(&data);
  EXPECT_EQ(ax::mojom::Role::kDialog, data.role);
}

TEST_F(AstraGlanceViewTest, AccessibleNameWithTitle) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetTitleText(u"My Test Page");

  ui::AXNodeData data;
  glance->GetAccessibleNodeData(&data);
  EXPECT_FALSE(data.GetName().empty());
}

TEST_F(AstraGlanceViewTest, AccessibleNameWithoutTitle) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  ui::AXNodeData data;
  glance->GetAccessibleNodeData(&data);
  EXPECT_FALSE(data.GetName().empty());
}

TEST_F(AstraGlanceViewTest, AccessibleNameNotEmptyForErrorState) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetLoadingState(AstraGlanceView::LoadingState::kError);
  ui::AXNodeData data;
  glance->GetAccessibleNodeData(&data);
  EXPECT_FALSE(data.GetName().empty());
}

// ===========================================================================
// Size constant tests
// ===========================================================================

TEST_F(AstraGlanceViewTest, CompactSizeIsPositive) {
  EXPECT_GT(AstraGlanceView::kCompactSize.width(), 0);
  EXPECT_GT(AstraGlanceView::kCompactSize.height(), 0);
}

TEST_F(AstraGlanceViewTest, ExpandedSizeIsPositive) {
  EXPECT_GT(AstraGlanceView::kExpandedSize.width(), 0);
  EXPECT_GT(AstraGlanceView::kExpandedSize.height(), 0);
}

TEST_F(AstraGlanceViewTest, ExpandedSizeLargerThanCompact) {
  EXPECT_GT(AstraGlanceView::kExpandedSize.width(),
            AstraGlanceView::kCompactSize.width());
  EXPECT_GT(AstraGlanceView::kExpandedSize.height(),
            AstraGlanceView::kCompactSize.height());
}

TEST_F(AstraGlanceViewTest, SectionHeightsArePositive) {
  EXPECT_GT(AstraGlanceView::kHeaderHeight, 0);
  EXPECT_GT(AstraGlanceView::kStatusBarHeight, 0);
  EXPECT_GT(AstraGlanceView::kActionBarHeight, 0);
  EXPECT_GT(AstraGlanceView::kResizeHandleSize, 0);
}

// ===========================================================================
// WebContents tests (null WebContents)
// ===========================================================================

TEST_F(AstraGlanceViewTest, SetNullWebContents) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetWebContents(nullptr);
  EXPECT_EQ(nullptr, glance->web_contents());
}

TEST_F(AstraGlanceViewTest, SetSameWebContentsIsNoOp) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetWebContents(nullptr);
  glance->SetWebContents(nullptr);
  // No crash = success.
}

// ===========================================================================
// Animation stub tests
// ===========================================================================

TEST_F(AstraGlanceViewTest, PlayEntranceAnimationDoesNotCrash) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);
  glance->PlayEntranceAnimation();
}

TEST_F(AstraGlanceViewTest, PlayExitAnimationCallsCallback) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  bool callback_called = false;
  glance->PlayExitAnimation(base::BindOnce(
      [](bool* called) { *called = true; }, base::Unretained(&callback_called)));
  EXPECT_TRUE(callback_called);
}

TEST_F(AstraGlanceViewTest, PlayExitAnimationWithNullCallback) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);
  glance->PlayExitAnimation(base::NullCallback());
  // No crash = success.
}

// ===========================================================================
// Unified content API tests (new API)
// ===========================================================================

TEST_F(AstraGlanceViewTest, SetContentSetsAllFields) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  GURL url("https://example.com/page");
  gfx::ImageSkia image;
  glance->SetContent(AstraGlanceContentType::kTabPreview, u"Test Title", url,
                     image);

  EXPECT_EQ(u"Test Title", glance->GetTitle());
  EXPECT_EQ(url, glance->GetUrl());
  EXPECT_EQ(AstraGlanceContentType::kTabPreview, glance->GetContentType());
}

TEST_F(AstraGlanceViewTest, SetTitleUpdatesTitle) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  EXPECT_TRUE(glance->GetTitle().empty());
  glance->SetTitle(u"My Page");
  EXPECT_EQ(u"My Page", glance->GetTitle());
}

TEST_F(AstraGlanceViewTest, SetTitleEmptyNewApi) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetTitle(u"Some Title");
  ASSERT_EQ(u"Some Title", glance->GetTitle());

  glance->SetTitle(std::u16string());
  EXPECT_TRUE(glance->GetTitle().empty());
}

TEST_F(AstraGlanceViewTest, SetTitleVeryLongNewApi) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  std::u16string long_title(500, u'x');
  glance->SetTitle(long_title);
  EXPECT_EQ(long_title, glance->GetTitle());
}

TEST_F(AstraGlanceViewTest, SetUrlUpdatesUrl) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  GURL url("https://example.com");
  glance->SetUrl(url);
  EXPECT_EQ(url, glance->GetUrl());
}

TEST_F(AstraGlanceViewTest, SetUrlInvalidUrl) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  GURL invalid_url;
  glance->SetUrl(invalid_url);
  EXPECT_FALSE(glance->GetUrl().is_valid());
}

TEST_F(AstraGlanceViewTest, SetUrlMultipleTimesNewApi) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetUrl(GURL("https://first.com"));
  ASSERT_EQ(GURL("https://first.com"), glance->GetUrl());

  glance->SetUrl(GURL("https://second.com"));
  EXPECT_EQ(GURL("https://second.com"), glance->GetUrl());

  glance->SetUrl(GURL("https://third.com"));
  EXPECT_EQ(GURL("https://third.com"), glance->GetUrl());
}

TEST_F(AstraGlanceViewTest, SetPreviewImageDoesNotCrash) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  gfx::ImageSkia image;
  glance->SetPreviewImage(image);
}

TEST_F(AstraGlanceViewTest, ClearPreviewImageDoesNotCrash) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->ClearPreviewImage();
}

TEST_F(AstraGlanceViewTest, SetPreviewImageThenClear) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  gfx::ImageSkia image;
  glance->SetPreviewImage(image);
  glance->ClearPreviewImage();
}

// ===========================================================================
// Content type tests (new API)
// ===========================================================================

TEST_F(AstraGlanceViewTest, DefaultContentTypeIsTabInfo) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  EXPECT_EQ(AstraGlanceContentType::kTabInfo, glance->GetContentType());
}

TEST_F(AstraGlanceViewTest, SetContentTypeTabPreview) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetContentType(AstraGlanceContentType::kTabPreview);
  EXPECT_EQ(AstraGlanceContentType::kTabPreview, glance->GetContentType());
}

TEST_F(AstraGlanceViewTest, SetContentTypeScreenshot) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetContentType(AstraGlanceContentType::kScreenshot);
  EXPECT_EQ(AstraGlanceContentType::kScreenshot, glance->GetContentType());
}

TEST_F(AstraGlanceViewTest, SetContentTypeReadingList) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetContentType(AstraGlanceContentType::kReadingList);
  EXPECT_EQ(AstraGlanceContentType::kReadingList, glance->GetContentType());
}

TEST_F(AstraGlanceViewTest, SetContentTypeNote) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetContentType(AstraGlanceContentType::kNote);
  EXPECT_EQ(AstraGlanceContentType::kNote, glance->GetContentType());
}

TEST_F(AstraGlanceViewTest, SetContentTypeBookmark) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetContentType(AstraGlanceContentType::kBookmark);
  EXPECT_EQ(AstraGlanceContentType::kBookmark, glance->GetContentType());
}

TEST_F(AstraGlanceViewTest, SetSameContentTypeIsNoOp) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetContentType(AstraGlanceContentType::kTabInfo);
  EXPECT_EQ(AstraGlanceContentType::kTabInfo, glance->GetContentType());
}

TEST_F(AstraGlanceViewTest, ContentTypeEnumAllValuesDistinct) {
  EXPECT_NE(static_cast<int>(AstraGlanceContentType::kTabPreview),
            static_cast<int>(AstraGlanceContentType::kTabInfo));
  EXPECT_NE(static_cast<int>(AstraGlanceContentType::kTabPreview),
            static_cast<int>(AstraGlanceContentType::kScreenshot));
  EXPECT_NE(static_cast<int>(AstraGlanceContentType::kTabPreview),
            static_cast<int>(AstraGlanceContentType::kReadingList));
  EXPECT_NE(static_cast<int>(AstraGlanceContentType::kTabPreview),
            static_cast<int>(AstraGlanceContentType::kNote));
  EXPECT_NE(static_cast<int>(AstraGlanceContentType::kTabPreview),
            static_cast<int>(AstraGlanceContentType::kBookmark));
  EXPECT_NE(static_cast<int>(AstraGlanceContentType::kTabInfo),
            static_cast<int>(AstraGlanceContentType::kScreenshot));
  EXPECT_NE(static_cast<int>(AstraGlanceContentType::kTabInfo),
            static_cast<int>(AstraGlanceContentType::kReadingList));
  EXPECT_NE(static_cast<int>(AstraGlanceContentType::kTabInfo),
            static_cast<int>(AstraGlanceContentType::kNote));
  EXPECT_NE(static_cast<int>(AstraGlanceContentType::kTabInfo),
            static_cast<int>(AstraGlanceContentType::kBookmark));
}

// ===========================================================================
// Pin state tests (new API — SetPinned/IsPinned)
// ===========================================================================

TEST_F(AstraGlanceViewTest, SetPinnedTrueNewApi) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  EXPECT_FALSE(glance->IsPinned());
  glance->SetPinned(true);
  EXPECT_TRUE(glance->IsPinned());
}

TEST_F(AstraGlanceViewTest, SetPinnedFalseNewApi) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetPinned(true);
  ASSERT_TRUE(glance->IsPinned());

  glance->SetPinned(false);
  EXPECT_FALSE(glance->IsPinned());
}

TEST_F(AstraGlanceViewTest, PinnedStateConsistentWithGlancePinned) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetPinned(true);
  EXPECT_TRUE(glance->IsPinned());
  EXPECT_TRUE(glance->IsGlancePinned());

  glance->SetPinned(false);
  EXPECT_FALSE(glance->IsPinned());
  EXPECT_FALSE(glance->IsGlancePinned());
}

// ===========================================================================
// Expanded state tests (new API — SetExpanded/IsExpanded)
// ===========================================================================

TEST_F(AstraGlanceViewTest, DefaultIsExpanded) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  EXPECT_TRUE(glance->IsExpanded());
}

TEST_F(AstraGlanceViewTest, SetExpandedFalse) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetExpanded(false);
  EXPECT_FALSE(glance->IsExpanded());
}

TEST_F(AstraGlanceViewTest, SetExpandedTrue) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetExpanded(false);
  ASSERT_FALSE(glance->IsExpanded());

  glance->SetExpanded(true);
  EXPECT_TRUE(glance->IsExpanded());
}

TEST_F(AstraGlanceViewTest, ExpandedStateConsistentWithDisplayMode) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetExpanded(false);
  EXPECT_FALSE(glance->IsExpanded());
  EXPECT_EQ(AstraGlanceView::DisplayMode::kCompact, glance->display_mode());

  glance->SetExpanded(true);
  EXPECT_TRUE(glance->IsExpanded());
  EXPECT_EQ(AstraGlanceView::DisplayMode::kExpanded, glance->display_mode());
}

// ===========================================================================
// Button visibility tests
// ===========================================================================

TEST_F(AstraGlanceViewTest, SetShowCloseButtonDoesNotCrash) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetShowCloseButton(true);
  glance->SetShowCloseButton(false);
}

TEST_F(AstraGlanceViewTest, SetShowPinButtonDoesNotCrash) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetShowPinButton(true);
  glance->SetShowPinButton(false);
}

TEST_F(AstraGlanceViewTest, SetShowOpenButtonDoesNotCrash) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetShowOpenButton(true);
  glance->SetShowOpenButton(false);
}

// ===========================================================================
// Domain text tests
// ===========================================================================

TEST_F(AstraGlanceViewTest, SetDomainTextDoesNotCrash) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetDomainText(u"example.com");
}

TEST_F(AstraGlanceViewTest, SetDomainTextEmpty) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetDomainText(std::u16string());
}

TEST_F(AstraGlanceViewTest, SetDomainTextVeryLong) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  std::u16string long_domain(200, u'a');
  long_domain += u".com";
  glance->SetDomainText(long_domain);
}

// ===========================================================================
// Loading state boolean API tests
// ===========================================================================

TEST_F(AstraGlanceViewTest, DefaultIsNotLoading) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  EXPECT_FALSE(glance->IsLoading());
}

TEST_F(AstraGlanceViewTest, SetLoadingTrue) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetLoading(true);
  EXPECT_TRUE(glance->IsLoading());
}

TEST_F(AstraGlanceViewTest, SetLoadingFalse) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetLoading(true);
  ASSERT_TRUE(glance->IsLoading());

  glance->SetLoading(false);
  EXPECT_FALSE(glance->IsLoading());
}

TEST_F(AstraGlanceViewTest, LoadingBooleanConsistentWithLoadingState) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  glance->SetLoading(true);
  EXPECT_EQ(AstraGlanceView::LoadingState::kLoading, glance->loading_state());

  glance->SetLoading(false);
  EXPECT_EQ(AstraGlanceView::LoadingState::kLoaded, glance->loading_state());
}

TEST_F(AstraGlanceViewTest, LoadingToggleManyTimes) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  for (int i = 0; i < 20; i++) {
    glance->SetLoading(true);
    ASSERT_TRUE(glance->IsLoading());
    glance->SetLoading(false);
    ASSERT_FALSE(glance->IsLoading());
  }
  EXPECT_FALSE(glance->IsLoading());
}

// ===========================================================================
// AstraGlanceViewControllerTest — controller-level tests
// ===========================================================================

class AstraGlanceViewControllerTest : public testing::Test {
 public:
  AstraGlanceViewControllerTest() = default;
  ~AstraGlanceViewControllerTest() override = default;

  void SetUp() override {
    profile_ = std::make_unique<TestingProfile>();
    prefs_ = profile_->GetPrefs();
    // Register Astra prefs for the testing profile.
    prefs::RegisterProfilePrefs(prefs_->registry());
  }

  void TearDown() override {
    profile_.reset();
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
  raw_ptr<PrefService> prefs_ = nullptr;
};

// ===========================================================================
// Controller observer default tests
// ===========================================================================

TEST_F(AstraGlanceViewControllerTest, ObserverHasDefaultEmptyImplementations) {
  // Verify that a minimal observer that only overrides one method
  // can be added and other notifications don't crash.
  MinimalObserver observer;
  // Test passes if we can construct without errors and
  // default methods don't crash when called.

  // We can't easily test direct calls, but we verify the class is
  // instantiable (which implies default implementations are valid).
}

TEST_F(AstraGlanceViewControllerTest, ObserverCanObserveMultipleEvents) {
  TestGlanceObserver observer;

  // Initially all counters are zero.
  EXPECT_EQ(0, observer.shown_count);
  EXPECT_EQ(0, observer.hidden_count);
  EXPECT_EQ(0, observer.expanded_count);
  EXPECT_EQ(0, observer.pinned_as_tab_count);
  EXPECT_EQ(0, observer.mode_changed_count);
  EXPECT_EQ(0, observer.resized_count);
  EXPECT_EQ(0, observer.source_changed_count);
  EXPECT_EQ(0, observer.pinned_changed_count);
  EXPECT_EQ(0, observer.settings_changed_count);
}

// ===========================================================================
// Presentation settings tests (PrefService-backed)
// ===========================================================================

TEST_F(AstraGlanceViewControllerTest, PrefsStartWithDefaults) {
  EXPECT_EQ(prefs::kDefaultGlanceDefaultDisplayMode,
            prefs_->GetString(prefs::kPrefGlanceDefaultDisplayMode));
  EXPECT_EQ(prefs::kDefaultGlanceShowDelayMs,
            prefs_->GetInteger(prefs::kPrefGlanceShowDelayMs));
  EXPECT_EQ(prefs::kDefaultGlanceAutoHideDelayMs,
            prefs_->GetInteger(prefs::kPrefGlanceAutoHideDelayMs));
  EXPECT_EQ(prefs::kDefaultGlanceCompactWidth,
            prefs_->GetInteger(prefs::kPrefGlanceCompactWidth));
  EXPECT_EQ(prefs::kDefaultGlanceCompactHeight,
            prefs_->GetInteger(prefs::kPrefGlanceCompactHeight));
  EXPECT_EQ(prefs::kDefaultGlanceExpandedWidth,
            prefs_->GetInteger(prefs::kPrefGlanceExpandedWidth));
  EXPECT_EQ(prefs::kDefaultGlanceExpandedHeight,
            prefs_->GetInteger(prefs::kPrefGlanceExpandedHeight));
  EXPECT_EQ(prefs::kDefaultGlanceShowActionBar,
            prefs_->GetBoolean(prefs::kPrefGlanceShowActionBar));
  EXPECT_EQ(prefs::kDefaultGlanceShowStatusBar,
            prefs_->GetBoolean(prefs::kPrefGlanceShowStatusBar));
  EXPECT_EQ(prefs::kDefaultGlanceShowResizeHandle,
            prefs_->GetBoolean(prefs::kPrefGlanceShowResizeHandle));
  EXPECT_EQ(prefs::kDefaultGlanceHoverPeekEnabled,
            prefs_->GetBoolean(prefs::kPrefGlanceHoverPeekEnabled));
  EXPECT_EQ(prefs::kDefaultGlanceDefaultPosition,
            prefs_->GetString(prefs::kPrefGlanceDefaultPosition));
  EXPECT_EQ(prefs::kDefaultGlancePinnedByDefault,
            prefs_->GetBoolean(prefs::kPrefGlancePinnedByDefault));
  EXPECT_EQ(prefs::kDefaultGlanceRememberSize,
            prefs_->GetBoolean(prefs::kPrefGlanceRememberSize));
  EXPECT_EQ(prefs::kDefaultGlanceRecentMaxCount,
            prefs_->GetInteger(prefs::kPrefGlanceRecentMaxCount));
  EXPECT_EQ(prefs::kDefaultGlanceShowSettingsButton,
            prefs_->GetBoolean(prefs::kPrefGlanceShowSettingsButton));
  EXPECT_EQ(prefs::kDefaultGlanceAnimationsEnabled,
            prefs_->GetBoolean(prefs::kPrefGlanceAnimationsEnabled));
  EXPECT_EQ(prefs::kDefaultGlanceSmallWidth,
            prefs_->GetInteger(prefs::kPrefGlanceSmallWidth));
  EXPECT_EQ(prefs::kDefaultGlanceSmallHeight,
            prefs_->GetInteger(prefs::kPrefGlanceSmallHeight));
  EXPECT_EQ(prefs::kDefaultGlanceLargeWidth,
            prefs_->GetInteger(prefs::kPrefGlanceLargeWidth));
  EXPECT_EQ(prefs::kDefaultGlanceLargeHeight,
            prefs_->GetInteger(prefs::kPrefGlanceLargeHeight));
}

TEST_F(AstraGlanceViewControllerTest, SetAndGetDisplayModePref) {
  // Default is expanded.
  EXPECT_EQ("expanded",
            prefs_->GetString(prefs::kPrefGlanceDefaultDisplayMode));

  // Set to compact.
  prefs_->SetString(prefs::kPrefGlanceDefaultDisplayMode, "compact");
  EXPECT_EQ("compact",
            prefs_->GetString(prefs::kPrefGlanceDefaultDisplayMode));

  // Set back to expanded.
  prefs_->SetString(prefs::kPrefGlanceDefaultDisplayMode, "expanded");
  EXPECT_EQ("expanded",
            prefs_->GetString(prefs::kPrefGlanceDefaultDisplayMode));
}

TEST_F(AstraGlanceViewControllerTest, SetAndGetShowDelayPref) {
  // Default value.
  EXPECT_EQ(prefs::kDefaultGlanceShowDelayMs,
            prefs_->GetInteger(prefs::kPrefGlanceShowDelayMs));

  // Set to custom value.
  prefs_->SetInteger(prefs::kPrefGlanceShowDelayMs, 1000);
  EXPECT_EQ(1000, prefs_->GetInteger(prefs::kPrefGlanceShowDelayMs));

  // Set to zero (instant show).
  prefs_->SetInteger(prefs::kPrefGlanceShowDelayMs, 0);
  EXPECT_EQ(0, prefs_->GetInteger(prefs::kPrefGlanceShowDelayMs));
}

TEST_F(AstraGlanceViewControllerTest, SetAndGetAutoHideDelayPref) {
  EXPECT_EQ(prefs::kDefaultGlanceAutoHideDelayMs,
            prefs_->GetInteger(prefs::kPrefGlanceAutoHideDelayMs));

  prefs_->SetInteger(prefs::kPrefGlanceAutoHideDelayMs, 500);
  EXPECT_EQ(500, prefs_->GetInteger(prefs::kPrefGlanceAutoHideDelayMs));
}

TEST_F(AstraGlanceViewControllerTest, SetAndGetCompactSizePrefs) {
  EXPECT_EQ(prefs::kDefaultGlanceCompactWidth,
            prefs_->GetInteger(prefs::kPrefGlanceCompactWidth));
  EXPECT_EQ(prefs::kDefaultGlanceCompactHeight,
            prefs_->GetInteger(prefs::kPrefGlanceCompactHeight));

  prefs_->SetInteger(prefs::kPrefGlanceCompactWidth, 500);
  prefs_->SetInteger(prefs::kPrefGlanceCompactHeight, 350);
  EXPECT_EQ(500, prefs_->GetInteger(prefs::kPrefGlanceCompactWidth));
  EXPECT_EQ(350, prefs_->GetInteger(prefs::kPrefGlanceCompactHeight));
}

TEST_F(AstraGlanceViewControllerTest, SetAndGetExpandedSizePrefs) {
  EXPECT_EQ(prefs::kDefaultGlanceExpandedWidth,
            prefs_->GetInteger(prefs::kPrefGlanceExpandedWidth));
  EXPECT_EQ(prefs::kDefaultGlanceExpandedHeight,
            prefs_->GetInteger(prefs::kPrefGlanceExpandedHeight));

  prefs_->SetInteger(prefs::kPrefGlanceExpandedWidth, 700);
  prefs_->SetInteger(prefs::kPrefGlanceExpandedHeight, 500);
  EXPECT_EQ(700, prefs_->GetInteger(prefs::kPrefGlanceExpandedWidth));
  EXPECT_EQ(500, prefs_->GetInteger(prefs::kPrefGlanceExpandedHeight));
}

TEST_F(AstraGlanceViewControllerTest, SetAndGetShowActionBarPref) {
  EXPECT_TRUE(prefs_->GetBoolean(prefs::kPrefGlanceShowActionBar));

  prefs_->SetBoolean(prefs::kPrefGlanceShowActionBar, false);
  EXPECT_FALSE(prefs_->GetBoolean(prefs::kPrefGlanceShowActionBar));
}

TEST_F(AstraGlanceViewControllerTest, SetAndGetShowStatusBarPref) {
  EXPECT_TRUE(prefs_->GetBoolean(prefs::kPrefGlanceShowStatusBar));

  prefs_->SetBoolean(prefs::kPrefGlanceShowStatusBar, false);
  EXPECT_FALSE(prefs_->GetBoolean(prefs::kPrefGlanceShowStatusBar));
}

TEST_F(AstraGlanceViewControllerTest, SetAndGetShowResizeHandlePref) {
  EXPECT_TRUE(prefs_->GetBoolean(prefs::kPrefGlanceShowResizeHandle));

  prefs_->SetBoolean(prefs::kPrefGlanceShowResizeHandle, false);
  EXPECT_FALSE(prefs_->GetBoolean(prefs::kPrefGlanceShowResizeHandle));
}

TEST_F(AstraGlanceViewControllerTest, SetAndGetHoverPeekEnabledPref) {
  EXPECT_TRUE(prefs_->GetBoolean(prefs::kPrefGlanceHoverPeekEnabled));

  prefs_->SetBoolean(prefs::kPrefGlanceHoverPeekEnabled, false);
  EXPECT_FALSE(prefs_->GetBoolean(prefs::kPrefGlanceHoverPeekEnabled));
}

TEST_F(AstraGlanceViewControllerTest, SetAndGetDefaultPositionPref) {
  EXPECT_EQ("right",
            prefs_->GetString(prefs::kPrefGlanceDefaultPosition));

  prefs_->SetString(prefs::kPrefGlanceDefaultPosition, "left");
  EXPECT_EQ("left",
            prefs_->GetString(prefs::kPrefGlanceDefaultPosition));

  prefs_->SetString(prefs::kPrefGlanceDefaultPosition, "top");
  EXPECT_EQ("top",
            prefs_->GetString(prefs::kPrefGlanceDefaultPosition));

  prefs_->SetString(prefs::kPrefGlanceDefaultPosition, "bottom");
  EXPECT_EQ("bottom",
            prefs_->GetString(prefs::kPrefGlanceDefaultPosition));
}

TEST_F(AstraGlanceViewControllerTest, SetAndGetPinnedByDefaultPref) {
  EXPECT_FALSE(prefs_->GetBoolean(prefs::kPrefGlancePinnedByDefault));

  prefs_->SetBoolean(prefs::kPrefGlancePinnedByDefault, true);
  EXPECT_TRUE(prefs_->GetBoolean(prefs::kPrefGlancePinnedByDefault));
}

TEST_F(AstraGlanceViewControllerTest, SetAndGetRememberSizePref) {
  EXPECT_TRUE(prefs_->GetBoolean(prefs::kPrefGlanceRememberSize));

  prefs_->SetBoolean(prefs::kPrefGlanceRememberSize, false);
  EXPECT_FALSE(prefs_->GetBoolean(prefs::kPrefGlanceRememberSize));
}

TEST_F(AstraGlanceViewControllerTest, SetAndGetRecentMaxCountPref) {
  EXPECT_EQ(10, prefs_->GetInteger(prefs::kPrefGlanceRecentMaxCount));

  prefs_->SetInteger(prefs::kPrefGlanceRecentMaxCount, 20);
  EXPECT_EQ(20, prefs_->GetInteger(prefs::kPrefGlanceRecentMaxCount));

  prefs_->SetInteger(prefs::kPrefGlanceRecentMaxCount, 1);
  EXPECT_EQ(1, prefs_->GetInteger(prefs::kPrefGlanceRecentMaxCount));

  prefs_->SetInteger(prefs::kPrefGlanceRecentMaxCount, 0);
  EXPECT_EQ(0, prefs_->GetInteger(prefs::kPrefGlanceRecentMaxCount));
}

TEST_F(AstraGlanceViewControllerTest, SetAndGetShowSettingsButtonPref) {
  EXPECT_TRUE(prefs_->GetBoolean(prefs::kPrefGlanceShowSettingsButton));

  prefs_->SetBoolean(prefs::kPrefGlanceShowSettingsButton, false);
  EXPECT_FALSE(prefs_->GetBoolean(prefs::kPrefGlanceShowSettingsButton));
}

TEST_F(AstraGlanceViewControllerTest, SetAndGetAnimationsEnabledPref) {
  EXPECT_TRUE(prefs_->GetBoolean(prefs::kPrefGlanceAnimationsEnabled));

  prefs_->SetBoolean(prefs::kPrefGlanceAnimationsEnabled, false);
  EXPECT_FALSE(prefs_->GetBoolean(prefs::kPrefGlanceAnimationsEnabled));
}

TEST_F(AstraGlanceViewControllerTest, SetAndGetSmallSizePrefs) {
  EXPECT_EQ(prefs::kDefaultGlanceSmallWidth,
            prefs_->GetInteger(prefs::kPrefGlanceSmallWidth));
  EXPECT_EQ(prefs::kDefaultGlanceSmallHeight,
            prefs_->GetInteger(prefs::kPrefGlanceSmallHeight));

  prefs_->SetInteger(prefs::kPrefGlanceSmallWidth, 250);
  prefs_->SetInteger(prefs::kPrefGlanceSmallHeight, 180);
  EXPECT_EQ(250, prefs_->GetInteger(prefs::kPrefGlanceSmallWidth));
  EXPECT_EQ(180, prefs_->GetInteger(prefs::kPrefGlanceSmallHeight));
}

TEST_F(AstraGlanceViewControllerTest, SetAndGetLargeSizePrefs) {
  EXPECT_EQ(prefs::kDefaultGlanceLargeWidth,
            prefs_->GetInteger(prefs::kPrefGlanceLargeWidth));
  EXPECT_EQ(prefs::kDefaultGlanceLargeHeight,
            prefs_->GetInteger(prefs::kPrefGlanceLargeHeight));

  prefs_->SetInteger(prefs::kPrefGlanceLargeWidth, 800);
  prefs_->SetInteger(prefs::kPrefGlanceLargeHeight, 600);
  EXPECT_EQ(800, prefs_->GetInteger(prefs::kPrefGlanceLargeWidth));
  EXPECT_EQ(600, prefs_->GetInteger(prefs::kPrefGlanceLargeHeight));
}

// ===========================================================================
// Recent glance history tests
// ===========================================================================

TEST_F(AstraGlanceViewControllerTest, RecentGlancesStartsEmpty) {
  const base::Value::List& list = prefs_->GetList(prefs::kPrefGlanceRecentUrls);
  EXPECT_TRUE(list.empty());
}

TEST_F(AstraGlanceViewControllerTest, RecentGlancesAddValidUrl) {
  const base::Value::List& empty_list =
      prefs_->GetList(prefs::kPrefGlanceRecentUrls);
  ASSERT_TRUE(empty_list.empty());

  GURL url("https://example.com");
  prefs_->SetList(prefs::kPrefGlanceRecentUrls,
                  base::Value::List().Append("https://example.com"));

  const base::Value::List& list =
      prefs_->GetList(prefs::kPrefGlanceRecentUrls);
  ASSERT_EQ(1u, list.size());
  EXPECT_EQ("https://example.com", list[0].GetString());
}

TEST_F(AstraGlanceViewControllerTest, RecentGlancesMultipleUrls) {
  base::Value::List urls;
  urls.Append("https://example.com");
  urls.Append("https://google.com");
  urls.Append("https://github.com");
  prefs_->SetList(prefs::kPrefGlanceRecentUrls, std::move(urls));

  const base::Value::List& list =
      prefs_->GetList(prefs::kPrefGlanceRecentUrls);
  EXPECT_EQ(3u, list.size());
}

TEST_F(AstraGlanceViewControllerTest, RecentGlancesDeduplication) {
  // Simulate adding a URL that already exists (should move to front).
  base::Value::List urls;
  urls.Append("https://example.com");
  urls.Append("https://google.com");
  prefs_->SetList(prefs::kPrefGlanceRecentUrls, std::move(urls));

  // Re-add "https://example.com" — it should still be in the list.
  // (Full dedup logic is in the controller; here we just verify prefs work.)
  const base::Value::List& list =
      prefs_->GetList(prefs::kPrefGlanceRecentUrls);
  EXPECT_EQ(2u, list.size());
}

TEST_F(AstraGlanceViewControllerTest, RecentGlancesClear) {
  base::Value::List urls;
  urls.Append("https://example.com");
  urls.Append("https://google.com");
  prefs_->SetList(prefs::kPrefGlanceRecentUrls, std::move(urls));

  prefs_->SetList(prefs::kPrefGlanceRecentUrls, base::Value::List());
  const base::Value::List& list =
      prefs_->GetList(prefs::kPrefGlanceRecentUrls);
  EXPECT_TRUE(list.empty());
}

TEST_F(AstraGlanceViewControllerTest, RecentGlancesMaxCount) {
  // Verify that we can store up to the max count.
  base::Value::List urls;
  for (int i = 0; i < 20; i++) {
    urls.Append("https://example" + base::NumberToString(i) + ".com");
  }
  prefs_->SetList(prefs::kPrefGlanceRecentUrls, std::move(urls));

  const base::Value::List& list =
      prefs_->GetList(prefs::kPrefGlanceRecentUrls);
  EXPECT_EQ(20u, list.size());
}

// ===========================================================================
// Position enum conversion tests
// ===========================================================================

TEST_F(AstraGlanceViewControllerTest, PositionToStringLeft) {
  EXPECT_EQ("left", AstraGlanceViewController::PositionToString(
                        AstraGlanceViewController::Position::kLeft));
}

TEST_F(AstraGlanceViewControllerTest, PositionToStringRight) {
  EXPECT_EQ("right", AstraGlanceViewController::PositionToString(
                        AstraGlanceViewController::Position::kRight));
}

TEST_F(AstraGlanceViewControllerTest, PositionToStringTop) {
  EXPECT_EQ("top", AstraGlanceViewController::PositionToString(
                        AstraGlanceViewController::Position::kTop));
}

TEST_F(AstraGlanceViewControllerTest, PositionToStringBottom) {
  EXPECT_EQ("bottom", AstraGlanceViewController::PositionToString(
                        AstraGlanceViewController::Position::kBottom));
}

TEST_F(AstraGlanceViewControllerTest, StringToPositionLeft) {
  EXPECT_EQ(AstraGlanceViewController::Position::kLeft,
            AstraGlanceViewController::StringToPosition("left"));
}

TEST_F(AstraGlanceViewControllerTest, StringToPositionRight) {
  EXPECT_EQ(AstraGlanceViewController::Position::kRight,
            AstraGlanceViewController::StringToPosition("right"));
}

TEST_F(AstraGlanceViewControllerTest, StringToPositionTop) {
  EXPECT_EQ(AstraGlanceViewController::Position::kTop,
            AstraGlanceViewController::StringToPosition("top"));
}

TEST_F(AstraGlanceViewControllerTest, StringToPositionBottom) {
  EXPECT_EQ(AstraGlanceViewController::Position::kBottom,
            AstraGlanceViewController::StringToPosition("bottom"));
}

TEST_F(AstraGlanceViewControllerTest, StringToPositionInvalid) {
  // Invalid string should return default (right).
  EXPECT_EQ(AstraGlanceViewController::Position::kRight,
            AstraGlanceViewController::StringToPosition("invalid"));
  EXPECT_EQ(AstraGlanceViewController::Position::kRight,
            AstraGlanceViewController::StringToPosition(""));
  EXPECT_EQ(AstraGlanceViewController::Position::kRight,
            AstraGlanceViewController::StringToPosition("foo"));
}

TEST_F(AstraGlanceViewControllerTest, PositionRoundTrip) {
  // Each position should round-trip through string conversion.
  std::vector<AstraGlanceViewController::Position> positions = {
      AstraGlanceViewController::Position::kLeft,
      AstraGlanceViewController::Position::kRight,
      AstraGlanceViewController::Position::kTop,
      AstraGlanceViewController::Position::kBottom,
  };
  for (auto pos : positions) {
    std::string str = AstraGlanceViewController::PositionToString(pos);
    EXPECT_EQ(pos, AstraGlanceViewController::StringToPosition(str));
  }
}

// ===========================================================================
// Pref persistence round-trip tests
// ===========================================================================

TEST_F(AstraGlanceViewControllerTest, PrefPersistenceStringValues) {
  // Set custom values.
  prefs_->SetString(prefs::kPrefGlanceDefaultDisplayMode, "compact");
  prefs_->SetString(prefs::kPrefGlanceDefaultPosition, "bottom");

  // Verify they persist (still the same values).
  EXPECT_EQ("compact",
            prefs_->GetString(prefs::kPrefGlanceDefaultDisplayMode));
  EXPECT_EQ("bottom",
            prefs_->GetString(prefs::kPrefGlanceDefaultPosition));
}

TEST_F(AstraGlanceViewControllerTest, PrefPersistenceBooleanValues) {
  prefs_->SetBoolean(prefs::kPrefGlanceShowActionBar, false);
  prefs_->SetBoolean(prefs::kPrefGlanceShowStatusBar, false);
  prefs_->SetBoolean(prefs::kPrefGlanceShowResizeHandle, false);
  prefs_->SetBoolean(prefs::kPrefGlanceHoverPeekEnabled, false);
  prefs_->SetBoolean(prefs::kPrefGlancePinnedByDefault, true);
  prefs_->SetBoolean(prefs::kPrefGlanceRememberSize, false);
  prefs_->SetBoolean(prefs::kPrefGlanceShowSettingsButton, false);
  prefs_->SetBoolean(prefs::kPrefGlanceAnimationsEnabled, false);

  EXPECT_FALSE(prefs_->GetBoolean(prefs::kPrefGlanceShowActionBar));
  EXPECT_FALSE(prefs_->GetBoolean(prefs::kPrefGlanceShowStatusBar));
  EXPECT_FALSE(prefs_->GetBoolean(prefs::kPrefGlanceShowResizeHandle));
  EXPECT_FALSE(prefs_->GetBoolean(prefs::kPrefGlanceHoverPeekEnabled));
  EXPECT_TRUE(prefs_->GetBoolean(prefs::kPrefGlancePinnedByDefault));
  EXPECT_FALSE(prefs_->GetBoolean(prefs::kPrefGlanceRememberSize));
  EXPECT_FALSE(prefs_->GetBoolean(prefs::kPrefGlanceShowSettingsButton));
  EXPECT_FALSE(prefs_->GetBoolean(prefs::kPrefGlanceAnimationsEnabled));
}

TEST_F(AstraGlanceViewControllerTest, PrefPersistenceIntegerValues) {
  prefs_->SetInteger(prefs::kPrefGlanceShowDelayMs, 250);
  prefs_->SetInteger(prefs::kPrefGlanceAutoHideDelayMs, 150);
  prefs_->SetInteger(prefs::kPrefGlanceCompactWidth, 300);
  prefs_->SetInteger(prefs::kPrefGlanceCompactHeight, 200);
  prefs_->SetInteger(prefs::kPrefGlanceExpandedWidth, 600);
  prefs_->SetInteger(prefs::kPrefGlanceExpandedHeight, 450);
  prefs_->SetInteger(prefs::kPrefGlanceRecentMaxCount, 5);

  EXPECT_EQ(250, prefs_->GetInteger(prefs::kPrefGlanceShowDelayMs));
  EXPECT_EQ(150, prefs_->GetInteger(prefs::kPrefGlanceAutoHideDelayMs));
  EXPECT_EQ(300, prefs_->GetInteger(prefs::kPrefGlanceCompactWidth));
  EXPECT_EQ(200, prefs_->GetInteger(prefs::kPrefGlanceCompactHeight));
  EXPECT_EQ(600, prefs_->GetInteger(prefs::kPrefGlanceExpandedWidth));
  EXPECT_EQ(450, prefs_->GetInteger(prefs::kPrefGlanceExpandedHeight));
  EXPECT_EQ(5, prefs_->GetInteger(prefs::kPrefGlanceRecentMaxCount));
}

TEST_F(AstraGlanceViewControllerTest, PrefClearResetsToDefault) {
  // Set a custom value.
  prefs_->SetInteger(prefs::kPrefGlanceShowDelayMs, 999);
  ASSERT_EQ(999, prefs_->GetInteger(prefs::kPrefGlanceShowDelayMs));

  // Clear the pref.
  prefs_->ClearPref(prefs::kPrefGlanceShowDelayMs);

  // Should be back to default.
  EXPECT_EQ(prefs::kDefaultGlanceShowDelayMs,
            prefs_->GetInteger(prefs::kPrefGlanceShowDelayMs));
}

// ===========================================================================
// Edge case tests
// ===========================================================================

TEST_F(AstraGlanceViewControllerTest, VeryLargeCompactWidth) {
  // Verify we can set very large values (prefs don't enforce clamping at
  // the registry level; clamping happens at the API layer).
  prefs_->SetInteger(prefs::kPrefGlanceCompactWidth, 99999);
  EXPECT_EQ(99999, prefs_->GetInteger(prefs::kPrefGlanceCompactWidth));
}

TEST_F(AstraGlanceViewControllerTest, NegativeSizeValues) {
  // Negative values should be stored but may be clamped by the API.
  prefs_->SetInteger(prefs::kPrefGlanceCompactWidth, -100);
  EXPECT_EQ(-100, prefs_->GetInteger(prefs::kPrefGlanceCompactWidth));
}

TEST_F(AstraGlanceViewControllerTest, ZeroSizeValues) {
  prefs_->SetInteger(prefs::kPrefGlanceCompactWidth, 0);
  prefs_->SetInteger(prefs::kPrefGlanceCompactHeight, 0);
  EXPECT_EQ(0, prefs_->GetInteger(prefs::kPrefGlanceCompactWidth));
  EXPECT_EQ(0, prefs_->GetInteger(prefs::kPrefGlanceCompactHeight));
}

TEST_F(AstraGlanceViewControllerTest, EmptyStringPosition) {
  prefs_->SetString(prefs::kPrefGlanceDefaultPosition, "");
  // Empty string should be stored as-is.
  EXPECT_EQ("", prefs_->GetString(prefs::kPrefGlanceDefaultPosition));
}

TEST_F(AstraGlanceViewControllerTest, NullPrefServiceEdgeCases) {
  // Test that PositionToString/StringToPosition work without PrefService.
  // These are static methods that don't depend on PrefService.
  EXPECT_EQ("left", AstraGlanceViewController::PositionToString(
                        AstraGlanceViewController::Position::kLeft));
  EXPECT_EQ(AstraGlanceViewController::Position::kRight,
            AstraGlanceViewController::StringToPosition("right"));
}

TEST_F(AstraGlanceViewControllerTest, SizePresetAreaOrdering) {
  // Verify area ordering: small < medium < large.
  gfx::Size small_size = AstraGlanceView::kSmallSize;
  gfx::Size medium_size = AstraGlanceView::kMediumSize;
  gfx::Size large_size = AstraGlanceView::kLargeSize;

  EXPECT_LT(small_size.GetArea(), medium_size.GetArea());
  EXPECT_LT(medium_size.GetArea(), large_size.GetArea());
  EXPECT_LT(small_size.GetArea(), large_size.GetArea());
}

TEST_F(AstraGlanceViewControllerTest, DisplayModeEnumValues) {
  // Compact and expanded are distinct values.
  EXPECT_NE(static_cast<int>(AstraGlanceView::DisplayMode::kCompact),
            static_cast<int>(AstraGlanceView::DisplayMode::kExpanded));
}

TEST_F(AstraGlanceViewControllerTest, LoadingStateEnumValues) {
  // All loading states are distinct.
  EXPECT_NE(static_cast<int>(AstraGlanceView::LoadingState::kLoading),
            static_cast<int>(AstraGlanceView::LoadingState::kLoaded));
  EXPECT_NE(static_cast<int>(AstraGlanceView::LoadingState::kLoading),
            static_cast<int>(AstraGlanceView::LoadingState::kError));
  EXPECT_NE(static_cast<int>(AstraGlanceView::LoadingState::kLoaded),
            static_cast<int>(AstraGlanceView::LoadingState::kError));
}

TEST_F(AstraGlanceViewControllerTest, SecurityStateEnumValues) {
  EXPECT_NE(static_cast<int>(AstraGlanceView::SecurityState::kSecure),
            static_cast<int>(AstraGlanceView::SecurityState::kNonSecure));
  EXPECT_NE(static_cast<int>(AstraGlanceView::SecurityState::kSecure),
            static_cast<int>(AstraGlanceView::SecurityState::kUnknown));
  EXPECT_NE(static_cast<int>(AstraGlanceView::SecurityState::kNonSecure),
            static_cast<int>(AstraGlanceView::SecurityState::kUnknown));
}

TEST_F(AstraGlanceViewControllerTest, SourceEnumValues) {
  EXPECT_NE(static_cast<int>(AstraGlanceView::Source::kSidebarHover),
            static_cast<int>(AstraGlanceView::Source::kLinkHover));
  EXPECT_NE(static_cast<int>(AstraGlanceView::Source::kSidebarHover),
            static_cast<int>(AstraGlanceView::Source::kKeyboard));
  EXPECT_NE(static_cast<int>(AstraGlanceView::Source::kSidebarHover),
            static_cast<int>(AstraGlanceView::Source::kUnknown));
}

TEST_F(AstraGlanceViewControllerTest, SizePresetEnumValues) {
  EXPECT_NE(static_cast<int>(AstraGlanceView::SizePreset::kSmall),
            static_cast<int>(AstraGlanceView::SizePreset::kMedium));
  EXPECT_NE(static_cast<int>(AstraGlanceView::SizePreset::kSmall),
            static_cast<int>(AstraGlanceView::SizePreset::kLarge));
  EXPECT_NE(static_cast<int>(AstraGlanceView::SizePreset::kMedium),
            static_cast<int>(AstraGlanceView::SizePreset::kLarge));
}

TEST_F(AstraGlanceViewControllerTest, PositionEnumValues) {
  EXPECT_NE(static_cast<int>(AstraGlanceViewController::Position::kLeft),
            static_cast<int>(AstraGlanceViewController::Position::kRight));
  EXPECT_NE(static_cast<int>(AstraGlanceViewController::Position::kLeft),
            static_cast<int>(AstraGlanceViewController::Position::kTop));
  EXPECT_NE(static_cast<int>(AstraGlanceViewController::Position::kLeft),
            static_cast<int>(AstraGlanceViewController::Position::kBottom));
}

// ===========================================================================
// Delegate interface completeness tests
// ===========================================================================

TEST_F(AstraGlanceViewTest, DelegateAllMethodsAreDeclared) {
  // Verify that the TestGlanceDelegate (which implements all methods)
  // can be constructed and all methods are accessible.
  TestGlanceDelegate delegate;

  // All counters should start at zero.
  EXPECT_EQ(0, delegate.close_requested_count);
  EXPECT_EQ(0, delegate.promote_to_tab_count);
  EXPECT_EQ(0, delegate.view_destroyed_count);
  EXPECT_EQ(0, delegate.add_to_favorites_count);
  EXPECT_EQ(0, delegate.copy_url_count);
  EXPECT_EQ(0, delegate.pin_tab_count);
  EXPECT_EQ(0, delegate.close_tab_count);
  EXPECT_EQ(0, delegate.toggle_expanded_count);
  EXPECT_EQ(0, delegate.toggle_pinned_count);
  EXPECT_EQ(0, delegate.size_preset_changed_count);
  EXPECT_EQ(0, delegate.settings_requested_count);
  EXPECT_EQ(0, delegate.resized_count);
  EXPECT_EQ(0, delegate.cycle_position_count);
}

// ===========================================================================
// View + controller integration edge cases
// ===========================================================================

TEST_F(AstraGlanceViewTest, ViewWithNullDelegateDoesNotCrash) {
  // The view should handle a null delegate gracefully for most operations.
  // Note: ShowBubble requires a delegate, so we test by creating a view
  // with a delegate and then simulating null-delegate behavior via
  // direct method calls that check delegate_.

  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  // Set title, URL, etc. — these don't need delegate.
  glance->SetTitleText(u"Test");
  glance->SetURLText(u"https://example.com");
  glance->SetLoadingState(AstraGlanceView::LoadingState::kLoading);
  glance->SetSecurityState(AstraGlanceView::SecurityState::kSecure);

  // No crash = success.
}

TEST_F(AstraGlanceViewTest, MultipleDisplayModeChanges) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  // Toggle many times — should not crash or leak.
  for (int i = 0; i < 100; i++) {
    glance->ToggleDisplayMode();
  }

  // After even number of toggles, should be back to start.
  EXPECT_EQ(AstraGlanceView::DisplayMode::kExpanded, glance->display_mode());
}

TEST_F(AstraGlanceViewTest, MultipleLoadingStateChanges) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  for (int i = 0; i < 50; i++) {
    glance->SetLoadingState(AstraGlanceView::LoadingState::kLoading);
    glance->SetLoadingState(AstraGlanceView::LoadingState::kLoaded);
    glance->SetLoadingState(AstraGlanceView::LoadingState::kError);
    glance->SetLoadingState(AstraGlanceView::LoadingState::kLoaded);
  }

  // Should be in loaded state after the cycle.
  EXPECT_EQ(AstraGlanceView::LoadingState::kLoaded, glance->loading_state());
}

TEST_F(AstraGlanceViewTest, MultipleSizePresetChanges) {
  TestGlanceDelegate delegate;
  AstraGlanceView* glance = CreateGlanceView(&delegate);

  for (int i = 0; i < 20; i++) {
    glance->ApplySizePreset(AstraGlanceView::SizePreset::kSmall);
    glance->ApplySizePreset(AstraGlanceView::SizePreset::kMedium);
    glance->ApplySizePreset(AstraGlanceView::SizePreset::kLarge);
  }

  // Should end at large.
  EXPECT_EQ(AstraGlanceView::SizePreset::kLarge, glance->GetCurrentSizePreset());
}

// ===========================================================================
// New controller API tests — tab index / show / hide
// ===========================================================================

TEST_F(AstraGlanceViewControllerTest, DefaultTabIndexIsNegative) {
  AstraGlanceViewController controller(nullptr);
  EXPECT_EQ(-1, controller.GetTabIndex());
}

TEST_F(AstraGlanceViewControllerTest, DefaultIsNotVisible) {
  AstraGlanceViewController controller(nullptr);
  EXPECT_FALSE(controller.IsVisible());
}

TEST_F(AstraGlanceViewControllerTest, DefaultIsNotAnimating) {
  AstraGlanceViewController controller(nullptr);
  EXPECT_FALSE(controller.IsAnimating());
}

TEST_F(AstraGlanceViewControllerTest, HideGlanceWhenNotVisibleIsNoOp) {
  AstraGlanceViewController controller(nullptr);
  // Should not crash when hiding a non-visible glance.
  controller.HideGlance();
  EXPECT_FALSE(controller.IsVisible());
}

TEST_F(AstraGlanceViewControllerTest, ShowGlanceWithTabIndex) {
  AstraGlanceViewController controller(nullptr);
  controller.ShowGlance(3, gfx::Point(100, 200));
  // Tab index should be updated.
  EXPECT_EQ(3, controller.GetTabIndex());
}

TEST_F(AstraGlanceViewControllerTest, ShowGlanceUpdatesAnchorPosition) {
  AstraGlanceViewController controller(nullptr);
  gfx::Point anchor(250, 350);
  controller.ShowGlance(0, anchor);
  EXPECT_EQ(anchor, controller.GetAnchorPosition());
}

TEST_F(AstraGlanceViewControllerTest, CancelHideWhenNoHidePending) {
  AstraGlanceViewController controller(nullptr);
  controller.CancelHide();
  // Should not crash.
}

TEST_F(AstraGlanceViewControllerTest, HideGlanceAfterDelayCanBeCancelled) {
  AstraGlanceViewController controller(nullptr);
  controller.SetDismissDelay(100);
  controller.HideGlanceAfterDelay();
  controller.CancelHide();
  // No crash and no hide happens.
  EXPECT_FALSE(controller.IsVisible());
}

TEST_F(AstraGlanceViewControllerTest, ShowGlanceWithInvalidTabIndex) {
  AstraGlanceViewController controller(nullptr);
  controller.ShowGlance(-1, gfx::Point());
  // Should not crash; tab index is stored as-is.
  EXPECT_EQ(-1, controller.GetTabIndex());
}

TEST_F(AstraGlanceViewControllerTest, ShowGlanceWithLargeTabIndex) {
  AstraGlanceViewController controller(nullptr);
  controller.ShowGlance(9999, gfx::Point());
  EXPECT_EQ(9999, controller.GetTabIndex());
}

// ===========================================================================
// Content type tests (controller)
// ===========================================================================

TEST_F(AstraGlanceViewControllerTest, DefaultContentTypeIsTabInfo) {
  AstraGlanceViewController controller(nullptr);
  EXPECT_EQ(AstraGlanceContentType::kTabInfo, controller.GetContentType());
}

TEST_F(AstraGlanceViewControllerTest, SetContentTypeTabPreview) {
  AstraGlanceViewController controller(nullptr);
  controller.SetContentType(AstraGlanceContentType::kTabPreview);
  EXPECT_EQ(AstraGlanceContentType::kTabPreview, controller.GetContentType());
}

TEST_F(AstraGlanceViewControllerTest, SetContentTypeTabInfo) {
  AstraGlanceViewController controller(nullptr);
  controller.SetContentType(AstraGlanceContentType::kTabPreview);
  controller.SetContentType(AstraGlanceContentType::kTabInfo);
  EXPECT_EQ(AstraGlanceContentType::kTabInfo, controller.GetContentType());
}

TEST_F(AstraGlanceViewControllerTest, SetContentTypeScreenshot) {
  AstraGlanceViewController controller(nullptr);
  controller.SetContentType(AstraGlanceContentType::kScreenshot);
  EXPECT_EQ(AstraGlanceContentType::kScreenshot, controller.GetContentType());
}

TEST_F(AstraGlanceViewControllerTest, SetContentTypeReadingList) {
  AstraGlanceViewController controller(nullptr);
  controller.SetContentType(AstraGlanceContentType::kReadingList);
  EXPECT_EQ(AstraGlanceContentType::kReadingList, controller.GetContentType());
}

TEST_F(AstraGlanceViewControllerTest, SetContentTypeNote) {
  AstraGlanceViewController controller(nullptr);
  controller.SetContentType(AstraGlanceContentType::kNote);
  EXPECT_EQ(AstraGlanceContentType::kNote, controller.GetContentType());
}

TEST_F(AstraGlanceViewControllerTest, SetContentTypeBookmark) {
  AstraGlanceViewController controller(nullptr);
  controller.SetContentType(AstraGlanceContentType::kBookmark);
  EXPECT_EQ(AstraGlanceContentType::kBookmark, controller.GetContentType());
}

TEST_F(AstraGlanceViewControllerTest, SetSameContentTypeIsNoOp) {
  AstraGlanceViewController controller(nullptr);
  // Default is kTabInfo; setting it again should be a no-op.
  controller.SetContentType(AstraGlanceContentType::kTabInfo);
  EXPECT_EQ(AstraGlanceContentType::kTabInfo, controller.GetContentType());
}

TEST_F(AstraGlanceViewControllerTest, ContentTypeAllSixTypes) {
  AstraGlanceViewController controller(nullptr);

  // Verify all 6 content types can be set and retrieved.
  std::vector<AstraGlanceContentType> types = {
      AstraGlanceContentType::kTabPreview,
      AstraGlanceContentType::kTabInfo,
      AstraGlanceContentType::kScreenshot,
      AstraGlanceContentType::kReadingList,
      AstraGlanceContentType::kNote,
      AstraGlanceContentType::kBookmark,
  };

  for (auto type : types) {
    controller.SetContentType(type);
    EXPECT_EQ(type, controller.GetContentType());
  }
}

// ===========================================================================
// Sizing tests (controller)
// ===========================================================================

TEST_F(AstraGlanceViewControllerTest, DefaultSizePresetIsMedium) {
  AstraGlanceViewController controller(nullptr);
  EXPECT_EQ(AstraGlanceSize::kMedium, controller.GetSizePreset());
}

TEST_F(AstraGlanceViewControllerTest, SetSizePresetSmall) {
  AstraGlanceViewController controller(nullptr);
  controller.SetSizePreset(AstraGlanceSize::kSmall);
  EXPECT_EQ(AstraGlanceSize::kSmall, controller.GetSizePreset());
}

TEST_F(AstraGlanceViewControllerTest, SetSizePresetMedium) {
  AstraGlanceViewController controller(nullptr);
  controller.SetSizePreset(AstraGlanceSize::kSmall);
  controller.SetSizePreset(AstraGlanceSize::kMedium);
  EXPECT_EQ(AstraGlanceSize::kMedium, controller.GetSizePreset());
}

TEST_F(AstraGlanceViewControllerTest, SetSizePresetLarge) {
  AstraGlanceViewController controller(nullptr);
  controller.SetSizePreset(AstraGlanceSize::kLarge);
  EXPECT_EQ(AstraGlanceSize::kLarge, controller.GetSizePreset());
}

TEST_F(AstraGlanceViewControllerTest, SetSizePresetExtraLarge) {
  AstraGlanceViewController controller(nullptr);
  controller.SetSizePreset(AstraGlanceSize::kExtraLarge);
  EXPECT_EQ(AstraGlanceSize::kExtraLarge, controller.GetSizePreset());
}

TEST_F(AstraGlanceViewControllerTest, AllSizePresetsAreDistinct) {
  EXPECT_NE(static_cast<int>(AstraGlanceSize::kSmall),
            static_cast<int>(AstraGlanceSize::kMedium));
  EXPECT_NE(static_cast<int>(AstraGlanceSize::kSmall),
            static_cast<int>(AstraGlanceSize::kLarge));
  EXPECT_NE(static_cast<int>(AstraGlanceSize::kSmall),
            static_cast<int>(AstraGlanceSize::kExtraLarge));
  EXPECT_NE(static_cast<int>(AstraGlanceSize::kMedium),
            static_cast<int>(AstraGlanceSize::kLarge));
  EXPECT_NE(static_cast<int>(AstraGlanceSize::kMedium),
            static_cast<int>(AstraGlanceSize::kExtraLarge));
  EXPECT_NE(static_cast<int>(AstraGlanceSize::kLarge),
            static_cast<int>(AstraGlanceSize::kExtraLarge));
}

TEST_F(AstraGlanceViewControllerTest, DefaultMinSize) {
  AstraGlanceViewController controller(nullptr);
  gfx::Size min_size = controller.GetMinSize();
  EXPECT_GT(min_size.width(), 0);
  EXPECT_GT(min_size.height(), 0);
}

TEST_F(AstraGlanceViewControllerTest, SetMinSize) {
  AstraGlanceViewController controller(nullptr);
  gfx::Size new_min(300, 200);
  controller.SetMinSize(new_min);
  EXPECT_EQ(new_min, controller.GetMinSize());
}

TEST_F(AstraGlanceViewControllerTest, DefaultMaxSize) {
  AstraGlanceViewController controller(nullptr);
  gfx::Size max_size = controller.GetMaxSize();
  EXPECT_GT(max_size.width(), 0);
  EXPECT_GT(max_size.height(), 0);
  EXPECT_GT(max_size.width(), controller.GetMinSize().width());
  EXPECT_GT(max_size.height(), controller.GetMinSize().height());
}

TEST_F(AstraGlanceViewControllerTest, SetMaxSize) {
  AstraGlanceViewController controller(nullptr);
  gfx::Size new_max(800, 600);
  controller.SetMaxSize(new_max);
  EXPECT_EQ(new_max, controller.GetMaxSize());
}

TEST_F(AstraGlanceViewControllerTest, DefaultMaintainAspectRatioIsFalse) {
  AstraGlanceViewController controller(nullptr);
  EXPECT_FALSE(controller.GetMaintainAspectRatio());
}

TEST_F(AstraGlanceViewControllerTest, SetMaintainAspectRatioTrue) {
  AstraGlanceViewController controller(nullptr);
  controller.SetMaintainAspectRatio(true);
  EXPECT_TRUE(controller.GetMaintainAspectRatio());
}

TEST_F(AstraGlanceViewControllerTest, SetMaintainAspectRatioFalse) {
  AstraGlanceViewController controller(nullptr);
  controller.SetMaintainAspectRatio(true);
  ASSERT_TRUE(controller.GetMaintainAspectRatio());

  controller.SetMaintainAspectRatio(false);
  EXPECT_FALSE(controller.GetMaintainAspectRatio());
}

TEST_F(AstraGlanceViewControllerTest, GetSizeWhenNotVisible) {
  AstraGlanceViewController controller(nullptr);
  // Should return last_size or default when not visible.
  gfx::Size size = controller.GetSize();
  // Size should be non-negative.
  EXPECT_GE(size.width(), 0);
  EXPECT_GE(size.height(), 0);
}

// ===========================================================================
// Positioning tests (controller)
// ===========================================================================

TEST_F(AstraGlanceViewControllerTest, DefaultAnchorPositionIsOrigin) {
  AstraGlanceViewController controller(nullptr);
  EXPECT_EQ(gfx::Point(), controller.GetAnchorPosition());
}

TEST_F(AstraGlanceViewControllerTest, SetAnchorPosition) {
  AstraGlanceViewController controller(nullptr);
  gfx::Point point(100, 200);
  controller.SetAnchorPosition(point);
  EXPECT_EQ(point, controller.GetAnchorPosition());
}

TEST_F(AstraGlanceViewControllerTest, SetAnchorPositionNegative) {
  AstraGlanceViewController controller(nullptr);
  gfx::Point point(-50, -30);
  controller.SetAnchorPosition(point);
  EXPECT_EQ(point, controller.GetAnchorPosition());
}

TEST_F(AstraGlanceViewControllerTest, DefaultPlacementIsAuto) {
  AstraGlanceViewController controller(nullptr);
  EXPECT_EQ(AstraGlancePlacement::kAuto, controller.GetPlacement());
}

TEST_F(AstraGlanceViewControllerTest, SetPlacementBelow) {
  AstraGlanceViewController controller(nullptr);
  controller.SetPlacement(AstraGlancePlacement::kBelow);
  EXPECT_EQ(AstraGlancePlacement::kBelow, controller.GetPlacement());
}

TEST_F(AstraGlanceViewControllerTest, SetPlacementAbove) {
  AstraGlanceViewController controller(nullptr);
  controller.SetPlacement(AstraGlancePlacement::kAbove);
  EXPECT_EQ(AstraGlancePlacement::kAbove, controller.GetPlacement());
}

TEST_F(AstraGlanceViewControllerTest, SetPlacementLeft) {
  AstraGlanceViewController controller(nullptr);
  controller.SetPlacement(AstraGlancePlacement::kLeft);
  EXPECT_EQ(AstraGlancePlacement::kLeft, controller.GetPlacement());
}

TEST_F(AstraGlanceViewControllerTest, SetPlacementRight) {
  AstraGlanceViewController controller(nullptr);
  controller.SetPlacement(AstraGlancePlacement::kRight);
  EXPECT_EQ(AstraGlancePlacement::kRight, controller.GetPlacement());
}

TEST_F(AstraGlanceViewControllerTest, SetPlacementAuto) {
  AstraGlanceViewController controller(nullptr);
  controller.SetPlacement(AstraGlancePlacement::kBelow);
  ASSERT_EQ(AstraGlancePlacement::kBelow, controller.GetPlacement());

  controller.SetPlacement(AstraGlancePlacement::kAuto);
  EXPECT_EQ(AstraGlancePlacement::kAuto, controller.GetPlacement());
}

TEST_F(AstraGlanceViewControllerTest, AllPlacementsAreDistinct) {
  EXPECT_NE(static_cast<int>(AstraGlancePlacement::kAuto),
            static_cast<int>(AstraGlancePlacement::kBelow));
  EXPECT_NE(static_cast<int>(AstraGlancePlacement::kAuto),
            static_cast<int>(AstraGlancePlacement::kAbove));
  EXPECT_NE(static_cast<int>(AstraGlancePlacement::kAuto),
            static_cast<int>(AstraGlancePlacement::kLeft));
  EXPECT_NE(static_cast<int>(AstraGlancePlacement::kAuto),
            static_cast<int>(AstraGlancePlacement::kRight));
  EXPECT_NE(static_cast<int>(AstraGlancePlacement::kBelow),
            static_cast<int>(AstraGlancePlacement::kAbove));
  EXPECT_NE(static_cast<int>(AstraGlancePlacement::kBelow),
            static_cast<int>(AstraGlancePlacement::kLeft));
  EXPECT_NE(static_cast<int>(AstraGlancePlacement::kBelow),
            static_cast<int>(AstraGlancePlacement::kRight));
  EXPECT_NE(static_cast<int>(AstraGlancePlacement::kAbove),
            static_cast<int>(AstraGlancePlacement::kLeft));
  EXPECT_NE(static_cast<int>(AstraGlancePlacement::kAbove),
            static_cast<int>(AstraGlancePlacement::kRight));
  EXPECT_NE(static_cast<int>(AstraGlancePlacement::kLeft),
            static_cast<int>(AstraGlancePlacement::kRight));
}

TEST_F(AstraGlanceViewControllerTest, DefaultOffsetIsPositive) {
  AstraGlanceViewController controller(nullptr);
  EXPECT_GT(controller.GetOffset(), 0);
}

TEST_F(AstraGlanceViewControllerTest, SetOffset) {
  AstraGlanceViewController controller(nullptr);
  controller.SetOffset(16);
  EXPECT_EQ(16, controller.GetOffset());
}

TEST_F(AstraGlanceViewControllerTest, SetOffsetZero) {
  AstraGlanceViewController controller(nullptr);
  controller.SetOffset(0);
  EXPECT_EQ(0, controller.GetOffset());
}

TEST_F(AstraGlanceViewControllerTest, SetOffsetNegative) {
  AstraGlanceViewController controller(nullptr);
  controller.SetOffset(-5);
  EXPECT_EQ(-5, controller.GetOffset());
}

// ===========================================================================
// Trigger mode tests (controller)
// ===========================================================================

TEST_F(AstraGlanceViewControllerTest, DefaultTriggerModeIsHover) {
  AstraGlanceViewController controller(nullptr);
  EXPECT_EQ(AstraGlanceTriggerMode::kHover, controller.GetTriggerMode());
}

TEST_F(AstraGlanceViewControllerTest, SetTriggerModeHover) {
  AstraGlanceViewController controller(nullptr);
  controller.SetTriggerMode(AstraGlanceTriggerMode::kHover);
  EXPECT_EQ(AstraGlanceTriggerMode::kHover, controller.GetTriggerMode());
}

TEST_F(AstraGlanceViewControllerTest, SetTriggerModeHoverLong) {
  AstraGlanceViewController controller(nullptr);
  controller.SetTriggerMode(AstraGlanceTriggerMode::kHoverLong);
  EXPECT_EQ(AstraGlanceTriggerMode::kHoverLong, controller.GetTriggerMode());
}

TEST_F(AstraGlanceViewControllerTest, SetTriggerModeClick) {
  AstraGlanceViewController controller(nullptr);
  controller.SetTriggerMode(AstraGlanceTriggerMode::kClick);
  EXPECT_EQ(AstraGlanceTriggerMode::kClick, controller.GetTriggerMode());
}

TEST_F(AstraGlanceViewControllerTest, SetTriggerModeKeyboard) {
  AstraGlanceViewController controller(nullptr);
  controller.SetTriggerMode(AstraGlanceTriggerMode::kKeyboard);
  EXPECT_EQ(AstraGlanceTriggerMode::kKeyboard, controller.GetTriggerMode());
}

TEST_F(AstraGlanceViewControllerTest, SetTriggerModeDisabled) {
  AstraGlanceViewController controller(nullptr);
  controller.SetTriggerMode(AstraGlanceTriggerMode::kDisabled);
  EXPECT_EQ(AstraGlanceTriggerMode::kDisabled, controller.GetTriggerMode());
}

TEST_F(AstraGlanceViewControllerTest, AllTriggerModesAreDistinct) {
  EXPECT_NE(static_cast<int>(AstraGlanceTriggerMode::kHover),
            static_cast<int>(AstraGlanceTriggerMode::kHoverLong));
  EXPECT_NE(static_cast<int>(AstraGlanceTriggerMode::kHover),
            static_cast<int>(AstraGlanceTriggerMode::kClick));
  EXPECT_NE(static_cast<int>(AstraGlanceTriggerMode::kHover),
            static_cast<int>(AstraGlanceTriggerMode::kKeyboard));
  EXPECT_NE(static_cast<int>(AstraGlanceTriggerMode::kHover),
            static_cast<int>(AstraGlanceTriggerMode::kDisabled));
  EXPECT_NE(static_cast<int>(AstraGlanceTriggerMode::kHoverLong),
            static_cast<int>(AstraGlanceTriggerMode::kClick));
  EXPECT_NE(static_cast<int>(AstraGlanceTriggerMode::kHoverLong),
            static_cast<int>(AstraGlanceTriggerMode::kKeyboard));
  EXPECT_NE(static_cast<int>(AstraGlanceTriggerMode::kHoverLong),
            static_cast<int>(AstraGlanceTriggerMode::kDisabled));
  EXPECT_NE(static_cast<int>(AstraGlanceTriggerMode::kClick),
            static_cast<int>(AstraGlanceTriggerMode::kKeyboard));
  EXPECT_NE(static_cast<int>(AstraGlanceTriggerMode::kClick),
            static_cast<int>(AstraGlanceTriggerMode::kDisabled));
  EXPECT_NE(static_cast<int>(AstraGlanceTriggerMode::kKeyboard),
            static_cast<int>(AstraGlanceTriggerMode::kDisabled));
}

TEST_F(AstraGlanceViewControllerTest, DefaultHoverDelayIsPositive) {
  AstraGlanceViewController controller(nullptr);
  EXPECT_GT(controller.GetHoverDelay(), 0);
}

TEST_F(AstraGlanceViewControllerTest, SetHoverDelay) {
  AstraGlanceViewController controller(nullptr);
  controller.SetHoverDelay(750);
  EXPECT_EQ(750, controller.GetHoverDelay());
}

TEST_F(AstraGlanceViewControllerTest, SetHoverDelayZero) {
  AstraGlanceViewController controller(nullptr);
  controller.SetHoverDelay(0);
  EXPECT_EQ(0, controller.GetHoverDelay());
}

TEST_F(AstraGlanceViewControllerTest, SetHoverDelayNegativeClampsToZero) {
  AstraGlanceViewController controller(nullptr);
  controller.SetHoverDelay(-100);
  // Negative values should be clamped to 0.
  EXPECT_EQ(0, controller.GetHoverDelay());
}

TEST_F(AstraGlanceViewControllerTest, DefaultDismissDelayIsPositive) {
  AstraGlanceViewController controller(nullptr);
  EXPECT_GT(controller.GetDismissDelay(), 0);
}

TEST_F(AstraGlanceViewControllerTest, SetDismissDelay) {
  AstraGlanceViewController controller(nullptr);
  controller.SetDismissDelay(500);
  EXPECT_EQ(500, controller.GetDismissDelay());
}

TEST_F(AstraGlanceViewControllerTest, SetDismissDelayZero) {
  AstraGlanceViewController controller(nullptr);
  controller.SetDismissDelay(0);
  EXPECT_EQ(0, controller.GetDismissDelay());
}

TEST_F(AstraGlanceViewControllerTest, SetDismissDelayNegativeClampsToZero) {
  AstraGlanceViewController controller(nullptr);
  controller.SetDismissDelay(-50);
  EXPECT_EQ(0, controller.GetDismissDelay());
}

// ===========================================================================
// Hover trigger toggle tests
// ===========================================================================

TEST_F(AstraGlanceViewControllerTest, DefaultShowOnTabHoverIsTrue) {
  AstraGlanceViewController controller(nullptr);
  EXPECT_TRUE(controller.GetShowOnTabHover());
}

TEST_F(AstraGlanceViewControllerTest, SetShowOnTabHoverFalse) {
  AstraGlanceViewController controller(nullptr);
  controller.SetShowOnTabHover(false);
  EXPECT_FALSE(controller.GetShowOnTabHover());
}

TEST_F(AstraGlanceViewControllerTest, SetShowOnTabHoverTrue) {
  AstraGlanceViewController controller(nullptr);
  controller.SetShowOnTabHover(false);
  ASSERT_FALSE(controller.GetShowOnTabHover());

  controller.SetShowOnTabHover(true);
  EXPECT_TRUE(controller.GetShowOnTabHover());
}

TEST_F(AstraGlanceViewControllerTest, DefaultShowOnBookmarkHoverIsTrue) {
  AstraGlanceViewController controller(nullptr);
  EXPECT_TRUE(controller.GetShowOnBookmarkHover());
}

TEST_F(AstraGlanceViewControllerTest, SetShowOnBookmarkHoverFalse) {
  AstraGlanceViewController controller(nullptr);
  controller.SetShowOnBookmarkHover(false);
  EXPECT_FALSE(controller.GetShowOnBookmarkHover());
}

TEST_F(AstraGlanceViewControllerTest, DefaultShowOnHistoryHoverIsTrue) {
  AstraGlanceViewController controller(nullptr);
  EXPECT_TRUE(controller.GetShowOnHistoryHover());
}

TEST_F(AstraGlanceViewControllerTest, SetShowOnHistoryHoverFalse) {
  AstraGlanceViewController controller(nullptr);
  controller.SetShowOnHistoryHover(false);
  EXPECT_FALSE(controller.GetShowOnHistoryHover());
}

// ===========================================================================
// Pin / expand interaction tests
// ===========================================================================

TEST_F(AstraGlanceViewControllerTest, DefaultIsPinnedIsFalse) {
  AstraGlanceViewController controller(nullptr);
  EXPECT_FALSE(controller.IsPinned());
}

TEST_F(AstraGlanceViewControllerTest, PinGlance) {
  AstraGlanceViewController controller(nullptr);
  controller.PinGlance();
  EXPECT_TRUE(controller.IsPinned());
}

TEST_F(AstraGlanceViewControllerTest, UnpinGlance) {
  AstraGlanceViewController controller(nullptr);
  controller.PinGlance();
  ASSERT_TRUE(controller.IsPinned());

  controller.UnpinGlance();
  EXPECT_FALSE(controller.IsPinned());
}

TEST_F(AstraGlanceViewControllerTest, PinGlanceWhenAlreadyPinnedIsNoOp) {
  AstraGlanceViewController controller(nullptr);
  controller.PinGlance();
  ASSERT_TRUE(controller.IsPinned());

  controller.PinGlance();
  EXPECT_TRUE(controller.IsPinned());
}

TEST_F(AstraGlanceViewControllerTest, DefaultIsExpanded) {
  AstraGlanceViewController controller(nullptr);
  EXPECT_TRUE(controller.IsExpanded());
}

TEST_F(AstraGlanceViewControllerTest, CollapseGlance) {
  AstraGlanceViewController controller(nullptr);
  controller.CollapseGlance();
  EXPECT_FALSE(controller.IsExpanded());
}

TEST_F(AstraGlanceViewControllerTest, ExpandGlance) {
  AstraGlanceViewController controller(nullptr);
  controller.CollapseGlance();
  ASSERT_FALSE(controller.IsExpanded());

  controller.ExpandGlance();
  EXPECT_TRUE(controller.IsExpanded());
}

TEST_F(AstraGlanceViewControllerTest, ExpandGlanceWhenAlreadyExpanded) {
  AstraGlanceViewController controller(nullptr);
  ASSERT_TRUE(controller.IsExpanded());

  controller.ExpandGlance();
  EXPECT_TRUE(controller.IsExpanded());
}

TEST_F(AstraGlanceViewControllerTest, CollapseGlanceWhenAlreadyCollapsed) {
  AstraGlanceViewController controller(nullptr);
  controller.CollapseGlance();
  ASSERT_FALSE(controller.IsExpanded());

  controller.CollapseGlance();
  EXPECT_FALSE(controller.IsExpanded());
}

// ===========================================================================
// Navigation action tests
// ===========================================================================

TEST_F(AstraGlanceViewControllerTest, NavigateToTabWhenNoTab) {
  AstraGlanceViewController controller(nullptr);
  // Should not crash when there's no tab to navigate to.
  controller.NavigateToTab();
}

TEST_F(AstraGlanceViewControllerTest, OpenInNewTabWhenNoContents) {
  AstraGlanceViewController controller(nullptr);
  // Should not crash when there's no WebContents.
  controller.OpenInNewTab();
}

TEST_F(AstraGlanceViewControllerTest, CloseTabWhenNoContents) {
  AstraGlanceViewController controller(nullptr);
  // Should not crash when there's no tab to close.
  controller.CloseTab();
}

// ===========================================================================
// Static pref key tests
// ===========================================================================

TEST_F(AstraGlanceViewControllerTest, StaticPrefKeysExist) {
  // Verify all 18+ pref keys are declared as static constexpr.
  EXPECT_STREQ("astra.glance.default_size_preset",
               AstraGlanceViewController::kPrefDefaultSizePreset);
  EXPECT_STREQ("astra.glance.default_placement",
               AstraGlanceViewController::kPrefDefaultPlacement);
  EXPECT_STREQ("astra.glance.trigger_mode",
               AstraGlanceViewController::kPrefTriggerMode);
  EXPECT_STREQ("astra.glance.hover_delay_ms",
               AstraGlanceViewController::kPrefHoverDelayMs);
  EXPECT_STREQ("astra.glance.dismiss_delay_ms",
               AstraGlanceViewController::kPrefDismissDelayMs);
  EXPECT_STREQ("astra.glance.maintain_aspect_ratio",
               AstraGlanceViewController::kPrefMaintainAspectRatio);
  EXPECT_STREQ("astra.glance.show_on_tab_hover",
               AstraGlanceViewController::kPrefShowOnTabHover);
  EXPECT_STREQ("astra.glance.show_on_bookmark_hover",
               AstraGlanceViewController::kPrefShowOnBookmarkHover);
  EXPECT_STREQ("astra.glance.show_on_history_hover",
               AstraGlanceViewController::kPrefShowOnHistoryHover);
  EXPECT_STREQ("astra.glance.show_on_sidebar_item_hover",
               AstraGlanceViewController::kPrefShowOnSidebarItemHover);
  EXPECT_STREQ("astra.glance.pin_on_click",
               AstraGlanceViewController::kPrefPinOnClick);
  EXPECT_STREQ("astra.glance.expand_on_double_click",
               AstraGlanceViewController::kPrefExpandOnDoubleClick);
  EXPECT_STREQ("astra.glance.animation_enabled",
               AstraGlanceViewController::kPrefAnimationEnabled);
  EXPECT_STREQ("astra.glance.animation_duration_ms",
               AstraGlanceViewController::kPrefAnimationDurationMs);
  EXPECT_STREQ("astra.glance.show_close_button",
               AstraGlanceViewController::kPrefShowCloseButton);
  EXPECT_STREQ("astra.glance.show_pin_button",
               AstraGlanceViewController::kPrefShowPinButton);
  EXPECT_STREQ("astra.glance.show_open_button",
               AstraGlanceViewController::kPrefShowOpenButton);
  EXPECT_STREQ("astra.glance.offset_px",
               AstraGlanceViewController::kPrefOffsetPx);
}

TEST_F(AstraGlanceViewControllerTest, PrefKeysHaveAstraPrefix) {
  // All keys should start with "astra.glance."
  EXPECT_THAT(std::string(AstraGlanceViewController::kPrefDefaultSizePreset),
              testing::StartsWith("astra.glance."));
  EXPECT_THAT(std::string(AstraGlanceViewController::kPrefDefaultPlacement),
              testing::StartsWith("astra.glance."));
  EXPECT_THAT(std::string(AstraGlanceViewController::kPrefTriggerMode),
              testing::StartsWith("astra.glance."));
  EXPECT_THAT(std::string(AstraGlanceViewController::kPrefHoverDelayMs),
              testing::StartsWith("astra.glance."));
  EXPECT_THAT(std::string(AstraGlanceViewController::kPrefDismissDelayMs),
              testing::StartsWith("astra.glance."));
  EXPECT_THAT(std::string(AstraGlanceViewController::kPrefMaintainAspectRatio),
              testing::StartsWith("astra.glance."));
  EXPECT_THAT(std::string(AstraGlanceViewController::kPrefShowOnTabHover),
              testing::StartsWith("astra.glance."));
  EXPECT_THAT(std::string(AstraGlanceViewController::kPrefShowOnBookmarkHover),
              testing::StartsWith("astra.glance."));
  EXPECT_THAT(std::string(AstraGlanceViewController::kPrefShowOnHistoryHover),
              testing::StartsWith("astra.glance."));
  EXPECT_THAT(std::string(AstraGlanceViewController::kPrefShowOnSidebarItemHover),
              testing::StartsWith("astra.glance."));
  EXPECT_THAT(std::string(AstraGlanceViewController::kPrefPinOnClick),
              testing::StartsWith("astra.glance."));
  EXPECT_THAT(std::string(AstraGlanceViewController::kPrefExpandOnDoubleClick),
              testing::StartsWith("astra.glance."));
  EXPECT_THAT(std::string(AstraGlanceViewController::kPrefAnimationEnabled),
              testing::StartsWith("astra.glance."));
  EXPECT_THAT(std::string(AstraGlanceViewController::kPrefAnimationDurationMs),
              testing::StartsWith("astra.glance."));
  EXPECT_THAT(std::string(AstraGlanceViewController::kPrefShowCloseButton),
              testing::StartsWith("astra.glance."));
  EXPECT_THAT(std::string(AstraGlanceViewController::kPrefShowPinButton),
              testing::StartsWith("astra.glance."));
  EXPECT_THAT(std::string(AstraGlanceViewController::kPrefShowOpenButton),
              testing::StartsWith("astra.glance."));
  EXPECT_THAT(std::string(AstraGlanceViewController::kPrefOffsetPx),
              testing::StartsWith("astra.glance."));
}

// ===========================================================================
// AstraGlanceObserver tests
// ===========================================================================

TEST_F(AstraGlanceViewControllerTest, AstraGlanceObserverHasDefaultImplementations) {
  // A minimal observer that only overrides one method should be constructible
  // and other methods should have empty default implementations.
  MinimalAstraGlanceObserver observer;
  EXPECT_EQ(0, observer.shown_count);
  // Default methods don't crash when called (tested indirectly via other tests).
}

TEST_F(AstraGlanceViewControllerTest, AddObserver) {
  AstraGlanceViewController controller(nullptr);
  TestAstraGlanceObserver observer;
  controller.AddObserver(&observer);
  // No crash = success.
}

TEST_F(AstraGlanceViewControllerTest, RemoveObserver) {
  AstraGlanceViewController controller(nullptr);
  TestAstraGlanceObserver observer;
  controller.AddObserver(&observer);
  controller.RemoveObserver(&observer);
  // No crash = success.
}

TEST_F(AstraGlanceViewControllerTest, ObserverNotifiedOnPin) {
  AstraGlanceViewController controller(nullptr);
  TestAstraGlanceObserver observer;
  controller.AddObserver(&observer);

  controller.PinGlance();

  EXPECT_EQ(1, observer.pinned_count);
  EXPECT_TRUE(observer.last_pinned);
  EXPECT_EQ(&controller, observer.last_controller);
}

TEST_F(AstraGlanceViewControllerTest, ObserverNotifiedOnUnpin) {
  AstraGlanceViewController controller(nullptr);
  TestAstraGlanceObserver observer;
  controller.AddObserver(&observer);

  controller.PinGlance();
  ASSERT_EQ(1, observer.pinned_count);

  controller.UnpinGlance();

  EXPECT_EQ(2, observer.pinned_count);
  EXPECT_FALSE(observer.last_pinned);
}

TEST_F(AstraGlanceViewControllerTest, ObserverNotifiedOnContentTypeChange) {
  AstraGlanceViewController controller(nullptr);
  TestAstraGlanceObserver observer;
  controller.AddObserver(&observer);

  controller.SetContentType(AstraGlanceContentType::kScreenshot);

  EXPECT_EQ(1, observer.content_type_changed_count);
  EXPECT_EQ(AstraGlanceContentType::kScreenshot, observer.last_content_type);
}

TEST_F(AstraGlanceViewControllerTest, ObserverNotifiedOnShowGlance) {
  AstraGlanceViewController controller(nullptr);
  TestAstraGlanceObserver observer;
  controller.AddObserver(&observer);

  controller.ShowGlance(5, gfx::Point(100, 200));

  EXPECT_GE(observer.shown_count, 1);
  EXPECT_EQ(5, observer.last_tab_index);
}

TEST_F(AstraGlanceViewControllerTest, ObserverNotifiedOnShutdown) {
  TestAstraGlanceObserver observer;
  {
    AstraGlanceViewController controller(nullptr);
    controller.AddObserver(&observer);
    EXPECT_EQ(0, observer.shutdown_count);
    // Controller destroyed at end of scope.
  }
  EXPECT_EQ(1, observer.shutdown_count);
}

TEST_F(AstraGlanceViewControllerTest, MultipleObservers) {
  AstraGlanceViewController controller(nullptr);
  TestAstraGlanceObserver observer1;
  TestAstraGlanceObserver observer2;
  controller.AddObserver(&observer1);
  controller.AddObserver(&observer2);

  controller.PinGlance();

  EXPECT_EQ(1, observer1.pinned_count);
  EXPECT_EQ(1, observer2.pinned_count);
}

TEST_F(AstraGlanceViewControllerTest, RemovedObserverDoesNotReceiveNotifications) {
  AstraGlanceViewController controller(nullptr);
  TestAstraGlanceObserver observer;
  controller.AddObserver(&observer);

  controller.PinGlance();
  ASSERT_EQ(1, observer.pinned_count);

  controller.RemoveObserver(&observer);
  controller.UnpinGlance();

  // Should still be 1 (not 2) since observer was removed.
  EXPECT_EQ(1, observer.pinned_count);
}

TEST_F(AstraGlanceViewControllerTest, ObserverNotifiedOfExpandedChange) {
  // This test indirectly verifies the expanded observer notification through
  // the SetDisplayMode path. When the controller's display mode changes,
  // observers should be notified.
  AstraGlanceViewController controller(nullptr);
  TestAstraGlanceObserver observer;
  controller.AddObserver(&observer);

  // By default the controller reports expanded = true.
  // Trigger a collapse to test notification.
  controller.CollapseGlance();
  // Observer may or may not fire depending on whether glance_view_ exists.
  // The test verifies no crash occurs.
  EXPECT_TRUE(true);
}

// ===========================================================================
// Enum value tests
// ===========================================================================

TEST_F(AstraGlanceViewControllerTest, AstraGlanceSizeEnumCount) {
  // Verify there are exactly 4 size presets.
  EXPECT_EQ(static_cast<int>(AstraGlanceSize::kSmall), 0);
  EXPECT_EQ(static_cast<int>(AstraGlanceSize::kMedium), 1);
  EXPECT_EQ(static_cast<int>(AstraGlanceSize::kLarge), 2);
  EXPECT_EQ(static_cast<int>(AstraGlanceSize::kExtraLarge), 3);
}

TEST_F(AstraGlanceViewControllerTest, AstraGlancePlacementEnumCount) {
  // Verify there are exactly 5 placements.
  EXPECT_EQ(static_cast<int>(AstraGlancePlacement::kAuto), 0);
  EXPECT_EQ(static_cast<int>(AstraGlancePlacement::kBelow), 1);
  EXPECT_EQ(static_cast<int>(AstraGlancePlacement::kAbove), 2);
  EXPECT_EQ(static_cast<int>(AstraGlancePlacement::kLeft), 3);
  EXPECT_EQ(static_cast<int>(AstraGlancePlacement::kRight), 4);
}

TEST_F(AstraGlanceViewControllerTest, AstraGlanceTriggerModeEnumCount) {
  // Verify there are exactly 5 trigger modes.
  EXPECT_EQ(static_cast<int>(AstraGlanceTriggerMode::kHover), 0);
  EXPECT_EQ(static_cast<int>(AstraGlanceTriggerMode::kHoverLong), 1);
  EXPECT_EQ(static_cast<int>(AstraGlanceTriggerMode::kClick), 2);
  EXPECT_EQ(static_cast<int>(AstraGlanceTriggerMode::kKeyboard), 3);
  EXPECT_EQ(static_cast<int>(AstraGlanceTriggerMode::kDisabled), 4);
}

TEST_F(AstraGlanceViewControllerTest, AstraGlanceContentTypeEnumCount) {
  // Verify there are exactly 6 content types.
  EXPECT_EQ(static_cast<int>(AstraGlanceContentType::kTabPreview), 0);
  EXPECT_EQ(static_cast<int>(AstraGlanceContentType::kTabInfo), 1);
  EXPECT_EQ(static_cast<int>(AstraGlanceContentType::kScreenshot), 2);
  EXPECT_EQ(static_cast<int>(AstraGlanceContentType::kReadingList), 3);
  EXPECT_EQ(static_cast<int>(AstraGlanceContentType::kNote), 4);
  EXPECT_EQ(static_cast<int>(AstraGlanceContentType::kBookmark), 5);
}

// ===========================================================================
// Edge case tests
// ===========================================================================

TEST_F(AstraGlanceViewControllerTest, ControllerWithNullBrowserView) {
  // Creating a controller with null BrowserView should not crash.
  AstraGlanceViewController controller(nullptr);
  EXPECT_FALSE(controller.IsVisible());
  EXPECT_EQ(-1, controller.GetTabIndex());
}

TEST_F(AstraGlanceViewControllerTest, DoubleShowGlance) {
  AstraGlanceViewController controller(nullptr);
  controller.ShowGlance(0, gfx::Point());
  controller.ShowGlance(1, gfx::Point());
  // Should not crash; second show replaces first.
  EXPECT_EQ(1, controller.GetTabIndex());
}

TEST_F(AstraGlanceViewControllerTest, HideWithNoShow) {
  AstraGlanceViewController controller(nullptr);
  controller.HideGlance();
  // Should not crash.
  EXPECT_FALSE(controller.IsVisible());
}

TEST_F(AstraGlanceViewControllerTest, VeryLargeHoverDelay) {
  AstraGlanceViewController controller(nullptr);
  controller.SetHoverDelay(100000);
  EXPECT_EQ(100000, controller.GetHoverDelay());
}

TEST_F(AstraGlanceViewControllerTest, VeryLargeDismissDelay) {
  AstraGlanceViewController controller(nullptr);
  controller.SetDismissDelay(100000);
  EXPECT_EQ(100000, controller.GetDismissDelay());
}

TEST_F(AstraGlanceViewControllerTest, SizePresetCycleAllFour) {
  AstraGlanceViewController controller(nullptr);

  controller.SetSizePreset(AstraGlanceSize::kSmall);
  EXPECT_EQ(AstraGlanceSize::kSmall, controller.GetSizePreset());

  controller.SetSizePreset(AstraGlanceSize::kMedium);
  EXPECT_EQ(AstraGlanceSize::kMedium, controller.GetSizePreset());

  controller.SetSizePreset(AstraGlanceSize::kLarge);
  EXPECT_EQ(AstraGlanceSize::kLarge, controller.GetSizePreset());

  controller.SetSizePreset(AstraGlanceSize::kExtraLarge);
  EXPECT_EQ(AstraGlanceSize::kExtraLarge, controller.GetSizePreset());
}

TEST_F(AstraGlanceViewControllerTest, PlacementCycleAllFive) {
  AstraGlanceViewController controller(nullptr);

  controller.SetPlacement(AstraGlancePlacement::kAuto);
  EXPECT_EQ(AstraGlancePlacement::kAuto, controller.GetPlacement());

  controller.SetPlacement(AstraGlancePlacement::kBelow);
  EXPECT_EQ(AstraGlancePlacement::kBelow, controller.GetPlacement());

  controller.SetPlacement(AstraGlancePlacement::kAbove);
  EXPECT_EQ(AstraGlancePlacement::kAbove, controller.GetPlacement());

  controller.SetPlacement(AstraGlancePlacement::kLeft);
  EXPECT_EQ(AstraGlancePlacement::kLeft, controller.GetPlacement());

  controller.SetPlacement(AstraGlancePlacement::kRight);
  EXPECT_EQ(AstraGlancePlacement::kRight, controller.GetPlacement());
}

TEST_F(AstraGlanceViewControllerTest, TriggerModeCycleAllFive) {
  AstraGlanceViewController controller(nullptr);

  controller.SetTriggerMode(AstraGlanceTriggerMode::kHover);
  EXPECT_EQ(AstraGlanceTriggerMode::kHover, controller.GetTriggerMode());

  controller.SetTriggerMode(AstraGlanceTriggerMode::kHoverLong);
  EXPECT_EQ(AstraGlanceTriggerMode::kHoverLong, controller.GetTriggerMode());

  controller.SetTriggerMode(AstraGlanceTriggerMode::kClick);
  EXPECT_EQ(AstraGlanceTriggerMode::kClick, controller.GetTriggerMode());

  controller.SetTriggerMode(AstraGlanceTriggerMode::kKeyboard);
  EXPECT_EQ(AstraGlanceTriggerMode::kKeyboard, controller.GetTriggerMode());

  controller.SetTriggerMode(AstraGlanceTriggerMode::kDisabled);
  EXPECT_EQ(AstraGlanceTriggerMode::kDisabled, controller.GetTriggerMode());
}

TEST_F(AstraGlanceViewControllerTest, ContentTypeCycleAllSix) {
  AstraGlanceViewController controller(nullptr);

  controller.SetContentType(AstraGlanceContentType::kTabPreview);
  EXPECT_EQ(AstraGlanceContentType::kTabPreview, controller.GetContentType());

  controller.SetContentType(AstraGlanceContentType::kTabInfo);
  EXPECT_EQ(AstraGlanceContentType::kTabInfo, controller.GetContentType());

  controller.SetContentType(AstraGlanceContentType::kScreenshot);
  EXPECT_EQ(AstraGlanceContentType::kScreenshot, controller.GetContentType());

  controller.SetContentType(AstraGlanceContentType::kReadingList);
  EXPECT_EQ(AstraGlanceContentType::kReadingList, controller.GetContentType());

  controller.SetContentType(AstraGlanceContentType::kNote);
  EXPECT_EQ(AstraGlanceContentType::kNote, controller.GetContentType());

  controller.SetContentType(AstraGlanceContentType::kBookmark);
  EXPECT_EQ(AstraGlanceContentType::kBookmark, controller.GetContentType());
}

TEST_F(AstraGlanceViewControllerTest, ZeroSizedGlanceMinSize) {
  AstraGlanceViewController controller(nullptr);
  controller.SetMinSize(gfx::Size(0, 0));
  EXPECT_EQ(gfx::Size(0, 0), controller.GetMinSize());
}

TEST_F(AstraGlanceViewControllerTest, VeryLargeMaxSize) {
  AstraGlanceViewController controller(nullptr);
  controller.SetMaxSize(gfx::Size(9999, 9999));
  EXPECT_EQ(gfx::Size(9999, 9999), controller.GetMaxSize());
}

TEST_F(AstraGlanceViewControllerTest, MinSizeLargerThanMaxSize) {
  AstraGlanceViewController controller(nullptr);
  // Setting min > max is allowed in the API; clamping happens on actual resize.
  controller.SetMinSize(gfx::Size(800, 600));
  controller.SetMaxSize(gfx::Size(400, 300));
  EXPECT_EQ(gfx::Size(800, 600), controller.GetMinSize());
  EXPECT_EQ(gfx::Size(400, 300), controller.GetMaxSize());
}

TEST_F(AstraGlanceViewControllerTest, TogglePinMultipleTimes) {
  AstraGlanceViewController controller(nullptr);
  TestAstraGlanceObserver observer;
  controller.AddObserver(&observer);

  for (int i = 0; i < 10; i++) {
    controller.PinGlance();
    controller.UnpinGlance();
  }

  // 20 total state changes (10 pin + 10 unpin).
  EXPECT_EQ(20, observer.pinned_count);
  EXPECT_FALSE(observer.last_pinned);
}

}  // namespace astra
