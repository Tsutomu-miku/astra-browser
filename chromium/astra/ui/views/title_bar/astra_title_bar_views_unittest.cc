// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/title_bar/astra_title_bar_view.h"

#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

class AstraTitleBarViewTest : public testing::Test {
 protected:
  void SetUp() override {}

  base::test::TaskEnvironment task_environment_;
};

// Test basic title bar creation.
TEST_F(AstraTitleBarViewTest, BasicCreation) {
  AstraTitleBarView title_bar;
  EXPECT_TRUE(title_bar.GetTitle().empty());
  EXPECT_EQ(32, title_bar.GetTitleBarHeight());
  EXPECT_TRUE(title_bar.AreWindowControlsVisible());
  EXPECT_TRUE(title_bar.IsWorkspaceVisible());
  EXPECT_FALSE(title_bar.IsAppIconVisible());
  EXPECT_FALSE(title_bar.IsDarkMode());
}

// Test title text.
TEST_F(AstraTitleBarViewTest, TitleText) {
  AstraTitleBarView title_bar;
  title_bar.SetTitle(u"Astra Browser - New Tab");
  EXPECT_EQ(u"Astra Browser - New Tab", title_bar.GetTitle());
}

// Test title bar height.
TEST_F(AstraTitleBarViewTest, Height) {
  AstraTitleBarView title_bar;
  EXPECT_EQ(32, title_bar.GetTitleBarHeight());

  title_bar.SetTitleBarHeight(40);
  EXPECT_EQ(40, title_bar.GetTitleBarHeight());
}

// Test background color.
TEST_F(AstraTitleBarViewTest, BackgroundColor) {
  AstraTitleBarView title_bar;
  EXPECT_EQ(SK_ColorWHITE, title_bar.GetBackgroundColor());

  title_bar.SetBackgroundColor(SK_ColorBLACK);
  EXPECT_EQ(SK_ColorBLACK, title_bar.GetBackgroundColor());
}

// Test dark mode.
TEST_F(AstraTitleBarViewTest, DarkMode) {
  AstraTitleBarView title_bar;
  EXPECT_FALSE(title_bar.IsDarkMode());

  title_bar.SetDarkMode(true);
  EXPECT_TRUE(title_bar.IsDarkMode());

  title_bar.SetDarkMode(false);
  EXPECT_FALSE(title_bar.IsDarkMode());
}

// Test app icon visibility.
TEST_F(AstraTitleBarViewTest, AppIconVisibility) {
  AstraTitleBarView title_bar;
  EXPECT_FALSE(title_bar.IsAppIconVisible());

  title_bar.SetAppIconVisible(true);
  EXPECT_TRUE(title_bar.IsAppIconVisible());
}

// Test workspace indicator.
TEST_F(AstraTitleBarViewTest, WorkspaceIndicator) {
  AstraTitleBarView title_bar;
  EXPECT_TRUE(title_bar.IsWorkspaceVisible());

  title_bar.SetWorkspaceVisible(false);
  EXPECT_FALSE(title_bar.IsWorkspaceVisible());

  title_bar.SetWorkspaceName(u"My Workspace");
  // Name is set even if not visible.

  title_bar.SetWorkspaceColor(SK_ColorGREEN);
  // Color is set.
}

// Test window controls visibility.
TEST_F(AstraTitleBarViewTest, WindowControlsVisibility) {
  AstraTitleBarView title_bar;
  EXPECT_TRUE(title_bar.AreWindowControlsVisible());
  EXPECT_NE(nullptr, title_bar.close_button());
  EXPECT_NE(nullptr, title_bar.maximize_button());
  EXPECT_NE(nullptr, title_bar.minimize_button());

  title_bar.SetWindowControlsVisible(false);
  EXPECT_FALSE(title_bar.AreWindowControlsVisible());
}

// Test window control types enum.
TEST_F(AstraTitleBarViewTest, WindowControlTypes) {
  // Verify all control types exist.
  AstraWindowControlType types[] = {
      AstraWindowControlType::kMinimize,
      AstraWindowControlType::kMaximize,
      AstraWindowControlType::kRestore,
      AstraWindowControlType::kClose,
  };
  EXPECT_EQ(4u, sizeof(types) / sizeof(types[0]));
}

// Test delegate integration.
TEST_F(AstraTitleBarViewTest, DelegateIntegration) {
  class TestDelegate : public AstraTitleBarDelegate {
   public:
    bool minimize_clicked = false;
    bool maximize_clicked = false;
    bool close_clicked = false;
    bool double_clicked = false;
    bool app_icon_clicked = false;
    bool workspace_clicked = false;
    bool is_maximized = false;
    std::u16string workspace_name = u"Test Workspace";
    SkColor workspace_color = SK_ColorBLUE;
    std::u16string tab_title = u"My Page";

    void OnWindowControlClicked(AstraWindowControlType type) override {
      switch (type) {
        case AstraWindowControlType::kMinimize:
          minimize_clicked = true;
          break;
        case AstraWindowControlType::kMaximize:
        case AstraWindowControlType::kRestore:
          maximize_clicked = true;
          break;
        case AstraWindowControlType::kClose:
          close_clicked = true;
          break;
      }
    }

    void OnTitleBarDoubleClicked() override { double_clicked = true; }
    void OnAppIconClicked() override { app_icon_clicked = true; }
    void OnWorkspaceClicked() override { workspace_clicked = true; }
    bool IsWindowMaximized() const override { return is_maximized; }
    std::u16string GetWorkspaceName() const override { return workspace_name; }
    SkColor GetWorkspaceColor() const override { return workspace_color; }
    std::u16string GetActiveTabTitle() const override { return tab_title; }
  };

  TestDelegate delegate;
  AstraTitleBarView title_bar(&delegate);

  // Title should be set from delegate.
  EXPECT_EQ(u"My Page", title_bar.GetTitle());

  // Test that delegate is stored.
  EXPECT_EQ(&delegate, title_bar.delegate());

  // Test setting delegate.
  TestDelegate delegate2;
  title_bar.SetDelegate(&delegate2);
  EXPECT_EQ(&delegate2, title_bar.delegate());
}

// Test maximize button update.
TEST_F(AstraTitleBarViewTest, MaximizeButtonUpdate) {
  AstraTitleBarView title_bar;

  // Update to maximized state.
  title_bar.UpdateMaximizeButton(true);
  // Button tooltip changes — verify via accessible name would require
  // querying the button, but the method should at least not crash.

  // Update to restored state.
  title_bar.UpdateMaximizeButton(false);
}

// Test preferred size.
TEST_F(AstraTitleBarViewTest, PreferredSize) {
  AstraTitleBarView title_bar;
  gfx::Size pref = title_bar.CalculatePreferredSize(views::SizeBounds());
  EXPECT_EQ(32, pref.height());
}

// Test that the title bar has all expected child views.
TEST_F(AstraTitleBarViewTest, ChildViewsExist) {
  AstraTitleBarView title_bar;

  // These should all exist (non-null).
  EXPECT_NE(nullptr, title_bar.minimize_button());
  EXPECT_NE(nullptr, title_bar.maximize_button());
  EXPECT_NE(nullptr, title_bar.close_button());
}

}  // namespace astra
