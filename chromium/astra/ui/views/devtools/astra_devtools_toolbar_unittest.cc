// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/devtools/astra_devtools_toolbar.h"

#include "base/memory/raw_ptr.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/event_constants.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/test/views_test_base.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace astra {

namespace {

// Mock delegate for testing AstraDevToolsToolbar.
class MockToolbarDelegate : public AstraDevToolsToolbar::Delegate {
 public:
  MockToolbarDelegate() = default;
  ~MockToolbarDelegate() override = default;

  MOCK_METHOD(void, OnFocusModeClicked, (), (override));
  MOCK_METHOD(void, OnWorkspaceInspectorClicked, (), (override));
};

}  // namespace

// =========================================================================
// AstraDevToolsToolbar tests
// =========================================================================

class AstraDevToolsToolbarTest : public views::ViewsTestBase {
 public:
  AstraDevToolsToolbarTest() = default;
  ~AstraDevToolsToolbarTest() override = default;

 protected:
  // testing::Test:
  void SetUp() override {
    views::ViewsTestBase::SetUp();
    widget_ = CreateTestWidget();
    toolbar_ = widget_->SetContentsView(
        std::make_unique<AstraDevToolsToolbar>(&delegate_));
  }

  void TearDown() override {
    widget_.reset();
    views::ViewsTestBase::TearDown();
  }

  testing::StrictMock<MockToolbarDelegate> delegate_;
  std::unique_ptr<views::Widget> widget_;
  raw_ptr<AstraDevToolsToolbar> toolbar_ = nullptr;
};

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TEST_F(AstraDevToolsToolbarTest, ConstructionCreatesToolbarView) {
  ASSERT_NE(toolbar_, nullptr);
  // The toolbar is a valid View instance hosted by the widget.
  EXPECT_EQ(toolbar_->parent(), widget_->GetRootView());
}

TEST_F(AstraDevToolsToolbarTest, ConstructionSetsLayout) {
  // The toolbar should have a layout manager (BoxLayout for horizontal buttons).
  EXPECT_NE(toolbar_->GetLayoutManager(), nullptr);
}

TEST_F(AstraDevToolsToolbarTest, ConstructionHasBackground) {
  // The toolbar should have a solid background (dark DevTools theme).
  EXPECT_NE(toolbar_->background(), nullptr);
}

// ---------------------------------------------------------------------------
// Button visibility and states
// ---------------------------------------------------------------------------

TEST_F(AstraDevToolsToolbarTest, FocusModeButtonExists) {
  views::LabelButton* button = toolbar_->focus_mode_button_for_testing();
  ASSERT_NE(button, nullptr);
  EXPECT_TRUE(button->GetVisible());
}

TEST_F(AstraDevToolsToolbarTest, WorkspaceInspectorButtonExists) {
  views::LabelButton* button = toolbar_->workspace_inspector_button_for_testing();
  ASSERT_NE(button, nullptr);
  EXPECT_TRUE(button->GetVisible());
}

TEST_F(AstraDevToolsToolbarTest, FocusModeButtonHasText) {
  views::LabelButton* button = toolbar_->focus_mode_button_for_testing();
  ASSERT_NE(button, nullptr);
  // The button should display "Focus Mode" label.
  EXPECT_FALSE(button->GetText().empty());
}

TEST_F(AstraDevToolsToolbarTest, WorkspaceInspectorButtonHasText) {
  views::LabelButton* button = toolbar_->workspace_inspector_button_for_testing();
  ASSERT_NE(button, nullptr);
  // The button should display "Workspace Inspector" label.
  EXPECT_FALSE(button->GetText().empty());
}

TEST_F(AstraDevToolsToolbarTest, ButtonsAreEnabledByDefault) {
  // Both buttons should be enabled after construction.
  EXPECT_TRUE(toolbar_->focus_mode_button_for_testing()->GetEnabled());
  EXPECT_TRUE(
      toolbar_->workspace_inspector_button_for_testing()->GetEnabled());
}

TEST_F(AstraDevToolsToolbarTest, ButtonsAreFocusable) {
  // Toolbar buttons should be focusable for keyboard navigation.
  EXPECT_EQ(toolbar_->focus_mode_button_for_testing()->GetFocusBehavior(),
            views::View::FocusBehavior::ALWAYS);
  EXPECT_EQ(
      toolbar_->workspace_inspector_button_for_testing()->GetFocusBehavior(),
      views::View::FocusBehavior::ALWAYS);
}

// ---------------------------------------------------------------------------
// Button click delegation
// ---------------------------------------------------------------------------

TEST_F(AstraDevToolsToolbarTest, FocusModeButtonClickCallsDelegate) {
  EXPECT_CALL(delegate_, OnFocusModeClicked()).Times(1);

  // Simulate a button click by triggering the pressed callback.
  toolbar_->focus_mode_button_for_testing()->OnKeyPressed(
      ui::KeyEvent(ui::ET_KEY_PRESSED, ui::VKEY_SPACE, ui::EF_NONE));
}

TEST_F(AstraDevToolsToolbarTest,
       WorkspaceInspectorButtonClickCallsDelegate) {
  EXPECT_CALL(delegate_, OnWorkspaceInspectorClicked()).Times(1);

  toolbar_->workspace_inspector_button_for_testing()->OnKeyPressed(
      ui::KeyEvent(ui::ET_KEY_PRESSED, ui::VKEY_SPACE, ui::EF_NONE));
}

// ---------------------------------------------------------------------------
// View hierarchy
// ---------------------------------------------------------------------------

TEST_F(AstraDevToolsToolbarTest, ToolbarContainsButtons) {
  // The toolbar should have both button accessors returning valid pointers.
  EXPECT_NE(toolbar_->focus_mode_button_for_testing(), nullptr);
  EXPECT_NE(toolbar_->workspace_inspector_button_for_testing(), nullptr);
}

TEST_F(AstraDevToolsToolbarTest, ButtonsAreChildrenOfToolbar) {
  // Both button pointers should be in the toolbar's child list.
  views::LabelButton* focus_button = toolbar_->focus_mode_button_for_testing();
  views::LabelButton* ws_button =
      toolbar_->workspace_inspector_button_for_testing();

  bool found_focus = false;
  bool found_ws = false;
  for (auto* child : toolbar_->children()) {
    if (child == focus_button)
      found_focus = true;
    if (child == ws_button)
      found_ws = true;
  }

  EXPECT_TRUE(found_focus);
  EXPECT_TRUE(found_ws);
}

// ---------------------------------------------------------------------------
// UpdateFromServices
// ---------------------------------------------------------------------------

TEST_F(AstraDevToolsToolbarTest, UpdateFromServicesDoesNotCrash) {
  // Calling UpdateFromServices should not crash, even with no inspected tab.
  toolbar_->UpdateFromServices();
  SUCCEED();
}

TEST_F(AstraDevToolsToolbarTest, SetInspectedWebContentsNullDoesNotCrash) {
  // Setting to null should be safe.
  toolbar_->SetInspectedWebContents(nullptr);
  SUCCEED();
}

}  // namespace astra
