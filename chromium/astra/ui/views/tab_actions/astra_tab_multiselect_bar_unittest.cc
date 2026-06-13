// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/tab_actions/astra_tab_multiselect_bar.h"

#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

// ===========================================================================
// AstraTabMultiSelectBarTest
// ===========================================================================

class AstraTabMultiSelectBarTest : public testing::Test {
 protected:
  void SetUp() override {
    bar_ = std::make_unique<AstraTabMultiSelectBar>();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<AstraTabMultiSelectBar> bar_;
};

// Test creation.
TEST_F(AstraTabMultiSelectBarTest, Creation) {
  EXPECT_EQ(0, bar_->selected_tab_count());
}

// Test setting selected tab count.
TEST_F(AstraTabMultiSelectBarTest, SelectedCount) {
  bar_->SetSelectedTabCount(5);
  EXPECT_EQ(5, bar_->selected_tab_count());

  bar_->SetSelectedTabCount(0);
  EXPECT_EQ(0, bar_->selected_tab_count());

  bar_->SetSelectedTabCount(1);
  EXPECT_EQ(1, bar_->selected_tab_count());
}

// Test close callback.
TEST_F(AstraTabMultiSelectBarTest, CloseCallback) {
  bool called = false;
  bar_->SetCloseCallback(
      base::BindRepeating([](bool* called) { *called = true; }, &called));

  // Simulate button click via callback.
  bar_->SetCloseCallback(
      base::BindRepeating([](bool* called) { *called = true; }, &called));

  EXPECT_FALSE(called);
  // The actual click would go through the button; we just verify
  // callback can be set without crashing.
}

// Test pin callback.
TEST_F(AstraTabMultiSelectBarTest, PinCallback) {
  bool called = false;
  bar_->SetPinCallback(
      base::BindRepeating([](bool* called) { *called = true; }, &called));
  // Should not crash.
}

// Test group callback.
TEST_F(AstraTabMultiSelectBarTest, GroupCallback) {
  bool called = false;
  bar_->SetGroupCallback(
      base::BindRepeating([](bool* called) { *called = true; }, &called));
}

// Test bookmark callback.
TEST_F(AstraTabMultiSelectBarTest, BookmarkCallback) {
  bool called = false;
  bar_->SetBookmarkCallback(
      base::BindRepeating([](bool* called) { *called = true; }, &called));
}

// Test move to workspace callback.
TEST_F(AstraTabMultiSelectBarTest, MoveToWorkspaceCallback) {
  bool called = false;
  std::string target_id;
  bar_->SetMoveToWorkspaceCallback(
      base::BindRepeating(
          [](bool* called, std::string* id, const std::string& ws_id) {
            *called = true;
            *id = ws_id;
          },
          &called, &target_id));
}

// Test deselect all callback.
TEST_F(AstraTabMultiSelectBarTest, DeselectAllCallback) {
  bool called = false;
  bar_->SetDeselectAllCallback(
      base::BindRepeating([](bool* called) { *called = true; }, &called));
}

// Test button visibility.
TEST_F(AstraTabMultiSelectBarTest, ButtonVisibility) {
  bar_->SetPinButtonVisible(false);
  bar_->SetPinButtonVisible(true);

  bar_->SetGroupButtonVisible(false);
  bar_->SetGroupButtonVisible(true);

  bar_->SetBookmarkButtonVisible(false);
  bar_->SetBookmarkButtonVisible(true);

  bar_->SetMoveButtonVisible(false);
  bar_->SetMoveButtonVisible(true);

  // Should not crash.
}

// Test visibility.
TEST_F(AstraTabMultiSelectBarTest, Visibility) {
  bar_->SetVisible(false);
  EXPECT_FALSE(bar_->GetVisible());

  bar_->SetVisible(true);
  EXPECT_TRUE(bar_->GetVisible());
}

// Test preferred size.
TEST_F(AstraTabMultiSelectBarTest, PreferredSize) {
  gfx::Size size = bar_->CalculatePreferredSize();
  EXPECT_GT(size.height(), 0);
}

// Test theme change doesn't crash.
TEST_F(AstraTabMultiSelectBarTest, ThemeChange) {
  // OnThemeChanged is called automatically when added to a widget.
  // We just verify the method exists and doesn't crash when called.
  bar_->OnThemeChanged();
}

}  // namespace astra
