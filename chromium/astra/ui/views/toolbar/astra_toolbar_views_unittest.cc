// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/toolbar/astra_toolbar_view.h"

#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

class AstraToolbarViewTest : public testing::Test {
 protected:
  void SetUp() override {}

  base::test::TaskEnvironment task_environment_;
};

// Test basic toolbar creation.
TEST_F(AstraToolbarViewTest, BasicCreation) {
  AstraToolbarView toolbar;
  EXPECT_EQ(48, toolbar.GetToolbarHeight());
  EXPECT_TRUE(toolbar.IsSecure());
  EXPECT_FALSE(toolbar.IsLoading());
  EXPECT_EQ(nullptr, toolbar.delegate());
}

// Test toolbar with delegate.
TEST_F(AstraToolbarViewTest, WithDelegate) {
  class TestDelegate : public AstraToolbarDelegate {
   public:
    void OnBack() override { back_called = true; }
    void OnForward() override { forward_called = true; }
    void OnReload() override { reload_called = true; }
    void OnHome() override { home_called = true; }
    void OnAppMenu() override { app_menu_called = true; }

    bool back_called = false;
    bool forward_called = false;
    bool reload_called = false;
    bool home_called = false;
    bool app_menu_called = false;
  };

  TestDelegate delegate;
  AstraToolbarView toolbar(&delegate);

  EXPECT_EQ(&delegate, toolbar.delegate());
}

// Test all toolbar buttons exist.
TEST_F(AstraToolbarViewTest, ButtonsExist) {
  AstraToolbarView toolbar;

  EXPECT_NE(nullptr, toolbar.back_button());
  EXPECT_NE(nullptr, toolbar.forward_button());
  EXPECT_NE(nullptr, toolbar.reload_button());
  EXPECT_NE(nullptr, toolbar.home_button());
  EXPECT_NE(nullptr, toolbar.app_menu_button());
}

// Test omnibox.
TEST_F(AstraToolbarViewTest, Omnibox) {
  AstraToolbarView toolbar;

  EXPECT_NE(nullptr, toolbar.omnibox());

  toolbar.SetOmniboxText(u"https://example.com");
  EXPECT_EQ(u"https://example.com", toolbar.GetOmniboxText());

  toolbar.SetOmniboxPlaceholder(u"Search or type URL");
}

// Test URL getter/setter.
TEST_F(AstraToolbarViewTest, Url) {
  AstraToolbarView toolbar;

  toolbar.SetUrl(u"https://astra-browser.com");
  EXPECT_EQ(u"https://astra-browser.com", toolbar.GetUrl());
}

// Test security state.
TEST_F(AstraToolbarViewTest, SecurityState) {
  AstraToolbarView toolbar;
  EXPECT_TRUE(toolbar.IsSecure());

  toolbar.SetSecure(false);
  EXPECT_FALSE(toolbar.IsSecure());

  toolbar.SetSecure(true);
  EXPECT_TRUE(toolbar.IsSecure());
}

// Test loading state.
TEST_F(AstraToolbarViewTest, LoadingState) {
  AstraToolbarView toolbar;
  EXPECT_FALSE(toolbar.IsLoading());

  toolbar.SetLoading(true);
  EXPECT_TRUE(toolbar.IsLoading());

  toolbar.SetLoading(false);
  EXPECT_FALSE(toolbar.IsLoading());
}

// Test toolbar height.
TEST_F(AstraToolbarViewTest, Height) {
  AstraToolbarView toolbar;
  EXPECT_EQ(48, toolbar.GetToolbarHeight());

  toolbar.SetToolbarHeight(40);
  EXPECT_EQ(40, toolbar.GetToolbarHeight());
}

// Test preferred size.
TEST_F(AstraToolbarViewTest, PreferredSize) {
  AstraToolbarView toolbar;
  gfx::Size pref = toolbar.CalculatePreferredSize(views::SizeBounds());
  EXPECT_EQ(48, pref.height());
}

// Test set delegate.
TEST_F(AstraToolbarViewTest, SetDelegate) {
  AstraToolbarView toolbar;
  EXPECT_EQ(nullptr, toolbar.delegate());

  class TestDelegate : public AstraToolbarDelegate {
   public:
    void OnBack() override {}
    void OnForward() override {}
    void OnReload() override {}
    void OnHome() override {}
  };

  TestDelegate delegate;
  toolbar.SetDelegate(&delegate);
  EXPECT_EQ(&delegate, toolbar.delegate());
}

// Test page actions container exists.
TEST_F(AstraToolbarViewTest, PageActionsContainer) {
  AstraToolbarView toolbar;
  EXPECT_NE(nullptr, toolbar.page_actions_for_test());
}

// Test UpdateNavigationButtonStates.
TEST_F(AstraToolbarViewTest, NavigationButtonStates) {
  class TestDelegate : public AstraToolbarDelegate {
   public:
    void OnBack() override {}
    void OnForward() override {}
    void OnReload() override {}
    void OnHome() override {}

    bool can_go_back = true;
    bool can_go_forward = false;

    bool CanGoBack() const override { return can_go_back; }
    bool CanGoForward() const override { return can_go_forward; }
  };

  TestDelegate delegate;
  AstraToolbarView toolbar(&delegate);

  toolbar.UpdateNavigationButtonStates();

  // Back should be enabled, forward disabled.
  EXPECT_TRUE(toolbar.back_button()->GetEnabled());
  EXPECT_FALSE(toolbar.forward_button()->GetEnabled());

  delegate.can_go_back = false;
  delegate.can_go_forward = true;
  toolbar.UpdateNavigationButtonStates();

  EXPECT_FALSE(toolbar.back_button()->GetEnabled());
  EXPECT_TRUE(toolbar.forward_button()->GetEnabled());
}

}  // namespace astra
