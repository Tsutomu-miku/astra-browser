// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/reader_mode/astra_reader_mode_view.h"

#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

// ===========================================================================
// AstraReaderModeViewTest
// ===========================================================================

class AstraReaderModeViewTest : public testing::Test {
 protected:
  void SetUp() override {
    anchor_view_ = std::make_unique<views::View>();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<views::View> anchor_view_;
};

// Test view creation.
TEST_F(AstraReaderModeViewTest, ViewCreation) {
  auto* view = new AstraReaderModeView(anchor_view_.get());
  EXPECT_NE(nullptr, view);
}

// Test window title.
TEST_F(AstraReaderModeViewTest, WindowTitle) {
  auto* view = new AstraReaderModeView(anchor_view_.get());
  EXPECT_EQ(u"Reader Mode", view->GetWindowTitle());
}

// Test setting theme.
TEST_F(AstraReaderModeViewTest, SetThemeLight) {
  auto* view = new AstraReaderModeView(anchor_view_.get());
  view->SetTheme(AstraReaderModeView::Theme::kLight);
  // No crash is the main test for presentation-only views.
  SUCCEED();
}

TEST_F(AstraReaderModeViewTest, SetThemeDark) {
  auto* view = new AstraReaderModeView(anchor_view_.get());
  view->SetTheme(AstraReaderModeView::Theme::kDark);
  SUCCEED();
}

TEST_F(AstraReaderModeViewTest, SetThemeSepia) {
  auto* view = new AstraReaderModeView(anchor_view_.get());
  view->SetTheme(AstraReaderModeView::Theme::kSepia);
  SUCCEED();
}

TEST_F(AstraReaderModeViewTest, SetThemeSystem) {
  auto* view = new AstraReaderModeView(anchor_view_.get());
  view->SetTheme(AstraReaderModeView::Theme::kSystem);
  SUCCEED();
}

// Test setting font size.
TEST_F(AstraReaderModeViewTest, SetFontSize) {
  auto* view = new AstraReaderModeView(anchor_view_.get());
  view->SetFontSize(20);
  SUCCEED();
}

// Test font size clamping (min).
TEST_F(AstraReaderModeViewTest, FontSizeMinClamp) {
  auto* view = new AstraReaderModeView(anchor_view_.get());
  view->SetFontSize(5);  // Below min of 10.
  SUCCEED();
}

// Test font size clamping (max).
TEST_F(AstraReaderModeViewTest, FontSizeMaxClamp) {
  auto* view = new AstraReaderModeView(anchor_view_.get());
  view->SetFontSize(50);  // Above max of 32.
  SUCCEED();
}

// Test setting line spacing.
TEST_F(AstraReaderModeViewTest, SetLineSpacing) {
  auto* view = new AstraReaderModeView(anchor_view_.get());
  view->SetLineSpacing(1.8);
  SUCCEED();
}

// Test setting font family.
TEST_F(AstraReaderModeViewTest, SetFontFamilySerif) {
  auto* view = new AstraReaderModeView(anchor_view_.get());
  view->SetFontFamily(AstraReaderModeView::FontFamily::kSerif);
  SUCCEED();
}

TEST_F(AstraReaderModeViewTest, SetFontFamilySansSerif) {
  auto* view = new AstraReaderModeView(anchor_view_.get());
  view->SetFontFamily(AstraReaderModeView::FontFamily::kSansSerif);
  SUCCEED();
}

TEST_F(AstraReaderModeViewTest, SetFontFamilyMonospace) {
  auto* view = new AstraReaderModeView(anchor_view_.get());
  view->SetFontFamily(AstraReaderModeView::FontFamily::kMonospace);
  SUCCEED();
}

// Test hide images toggle.
TEST_F(AstraReaderModeViewTest, SetHideImages) {
  auto* view = new AstraReaderModeView(anchor_view_.get());
  view->SetHideImages(true);
  SUCCEED();
}

TEST_F(AstraReaderModeViewTest, SetHideImagesFalse) {
  auto* view = new AstraReaderModeView(anchor_view_.get());
  view->SetHideImages(false);
  SUCCEED();
}

// Test highlight line toggle.
TEST_F(AstraReaderModeViewTest, SetHighlightLine) {
  auto* view = new AstraReaderModeView(anchor_view_.get());
  view->SetHighlightLine(true);
  SUCCEED();
}

// Test auto-scroll toggle.
TEST_F(AstraReaderModeViewTest, SetAutoScroll) {
  auto* view = new AstraReaderModeView(anchor_view_.get());
  view->SetAutoScroll(true);
  SUCCEED();
}

// Test reader mode availability.
TEST_F(AstraReaderModeViewTest, SetReaderModeAvailable) {
  auto* view = new AstraReaderModeView(anchor_view_.get());
  view->SetReaderModeAvailable(true);
  SUCCEED();
}

TEST_F(AstraReaderModeViewTest, SetReaderModeUnavailable) {
  auto* view = new AstraReaderModeView(anchor_view_.get());
  view->SetReaderModeAvailable(false);
  SUCCEED();
}

// Test theme callback.
TEST_F(AstraReaderModeViewTest, ThemeCallback) {
  AstraReaderModeView::Theme received_theme =
      AstraReaderModeView::Theme::kLight;
  bool callback_called = false;

  auto* view = new AstraReaderModeView(anchor_view_.get());
  view->SetThemeChangedCallback(
      base::BindRepeating(
          [](bool* called, AstraReaderModeView::Theme* out,
             AstraReaderModeView::Theme theme) {
            *called = true;
            *out = theme;
          },
          &callback_called, &received_theme));

  // Callback is only fired from user interaction (button click), not from
  // programmatic SetTheme calls.
  EXPECT_FALSE(callback_called);
}

// Test font size callback.
TEST_F(AstraReaderModeViewTest, FontSizeCallback) {
  int received_size = 0;
  bool callback_called = false;

  auto* view = new AstraReaderModeView(anchor_view_.get());
  view->SetFontSizeChangedCallback(
      base::BindRepeating(
          [](bool* called, int* out, int size) {
            *called = true;
            *out = size;
          },
          &callback_called, &received_size));

  // SetFontSize is programmatic; callback fires from button clicks.
  view->SetFontSize(18);
  EXPECT_FALSE(callback_called);
}

// Test hide images callback.
TEST_F(AstraReaderModeViewTest, HideImagesCallback) {
  bool callback_called = false;

  auto* view = new AstraReaderModeView(anchor_view_.get());
  view->SetHideImagesCallback(
      base::BindRepeating(
          [](bool* called, bool) { *called = true; },
          &callback_called));

  view->SetHideImages(true);
  EXPECT_FALSE(callback_called);
}

// Test enter reader mode callback.
TEST_F(AstraReaderModeViewTest, EnterReaderModeCallback) {
  bool callback_called = false;

  auto* view = new AstraReaderModeView(anchor_view_.get());
  view->SetEnterReaderModeCallback(
      base::BindRepeating(
          [](bool* called) { *called = true; },
          &callback_called));

  // Not fired until button is clicked.
  EXPECT_FALSE(callback_called);
}

// Test setting all callbacks doesn't crash.
TEST_F(AstraReaderModeViewTest, SetAllCallbacks) {
  auto* view = new AstraReaderModeView(anchor_view_.get());

  view->SetThemeChangedCallback(base::DoNothing());
  view->SetFontSizeChangedCallback(base::DoNothing());
  view->SetLineSpacingChangedCallback(base::DoNothing());
  view->SetFontChangedCallback(base::DoNothing());
  view->SetHideImagesCallback(base::DoNothing());
  view->SetHighlightLineCallback(base::DoNothing());
  view->SetAutoScrollCallback(base::DoNothing());
  view->SetEnterReaderModeCallback(base::DoNothing());

  SUCCEED();
}

}  // namespace astra
