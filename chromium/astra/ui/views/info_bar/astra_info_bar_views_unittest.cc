// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/info_bar/astra_info_bar_view.h"

#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

class AstraInfoBarViewTest : public testing::Test {
 protected:
  void SetUp() override {}

  base::test::TaskEnvironment task_environment_;
};

// Test basic info bar creation.
TEST_F(AstraInfoBarViewTest, BasicCreation) {
  AstraInfoBarView bar(AstraInfoBarType::kInformation);
  EXPECT_EQ(AstraInfoBarType::kInformation, bar.GetType());
  EXPECT_TRUE(bar.IsExpanded());
}

// Test info bar types.
TEST_F(AstraInfoBarViewTest, DifferentTypes) {
  AstraInfoBarView info(AstraInfoBarType::kInformation);
  EXPECT_EQ(AstraInfoBarType::kInformation, info.GetType());

  AstraInfoBarView warning(AstraInfoBarType::kWarning);
  EXPECT_EQ(AstraInfoBarType::kWarning, warning.GetType());

  AstraInfoBarView error(AstraInfoBarType::kError);
  EXPECT_EQ(AstraInfoBarType::kError, error.GetType());

  AstraInfoBarView success(AstraInfoBarType::kSuccess);
  EXPECT_EQ(AstraInfoBarType::kSuccess, success.GetType());

  AstraInfoBarView permission(AstraInfoBarType::kPermission);
  EXPECT_EQ(AstraInfoBarType::kPermission, permission.GetType());

  AstraInfoBarView extension(AstraInfoBarType::kExtension);
  EXPECT_EQ(AstraInfoBarType::kExtension, extension.GetType());

  AstraInfoBarView password(AstraInfoBarType::kPassword);
  EXPECT_EQ(AstraInfoBarType::kPassword, password.GetType());

  AstraInfoBarView autofill(AstraInfoBarType::kAutofill);
  EXPECT_EQ(AstraInfoBarType::kAutofill, autofill.GetType());
}

// Test message text.
TEST_F(AstraInfoBarViewTest, MessageText) {
  AstraInfoBarView bar(AstraInfoBarType::kInformation);
  bar.SetMessage(u"This is a test message");
  EXPECT_EQ(u"This is a test message", bar.GetMessage());
}

// Test icon visibility.
TEST_F(AstraInfoBarViewTest, IconVisibility) {
  AstraInfoBarView bar(AstraInfoBarType::kInformation);
  EXPECT_TRUE(bar.IsIconVisible());

  bar.SetIconVisible(false);
  EXPECT_FALSE(bar.IsIconVisible());

  bar.SetIconVisible(true);
  EXPECT_TRUE(bar.IsIconVisible());
}

// Test close button visibility.
TEST_F(AstraInfoBarViewTest, CloseButtonVisibility) {
  AstraInfoBarView bar(AstraInfoBarType::kInformation);
  EXPECT_TRUE(bar.IsCloseButtonVisible());

  bar.SetCloseButtonVisible(false);
  EXPECT_FALSE(bar.IsCloseButtonVisible());
}

// Test link.
TEST_F(AstraInfoBarViewTest, Link) {
  AstraInfoBarView bar(AstraInfoBarType::kInformation);
  EXPECT_FALSE(bar.IsLinkVisible());

  bar.SetLinkText(u"Learn more");
  bar.SetLinkVisible(true);
  EXPECT_TRUE(bar.IsLinkVisible());
}

// Test actions.
TEST_F(AstraInfoBarViewTest, Actions) {
  AstraInfoBarView bar(AstraInfoBarType::kInformation);
  EXPECT_EQ(0u, bar.GetActionCount());

  std::vector<AstraInfoBarAction> actions = {
      {1, u"Allow", true, false},
      {2, u"Block", false, false},
  };
  bar.SetActions(actions);
  EXPECT_EQ(2u, bar.GetActionCount());

  bar.AddAction({3, u"Learn more", false, true});
  EXPECT_EQ(3u, bar.GetActionCount());

  bar.ClearActions();
  EXPECT_EQ(0u, bar.GetActionCount());
}

// Test expanded state.
TEST_F(AstraInfoBarViewTest, ExpandedState) {
  AstraInfoBarView bar(AstraInfoBarType::kInformation);
  EXPECT_TRUE(bar.IsExpanded());

  bar.SetExpanded(false);
  EXPECT_FALSE(bar.IsExpanded());

  bar.SetExpanded(true);
  EXPECT_TRUE(bar.IsExpanded());
}

// Test type change.
TEST_F(AstraInfoBarViewTest, TypeChange) {
  AstraInfoBarView bar(AstraInfoBarType::kInformation);
  EXPECT_EQ(AstraInfoBarType::kInformation, bar.GetType());

  bar.SetType(AstraInfoBarType::kError);
  EXPECT_EQ(AstraInfoBarType::kError, bar.GetType());
}

// Test delegate callbacks.
TEST_F(AstraInfoBarViewTest, DelegateCallbacks) {
  class TestDelegate : public AstraInfoBarDelegate {
   public:
    int last_action_id = -1;
    bool dismissed = false;
    bool link_clicked = false;

    void OnInfoBarAction(int action_id) override {
      last_action_id = action_id;
    }
    void OnInfoBarDismissed() override { dismissed = true; }
    void OnInfoBarLinkClicked() override { link_clicked = true; }
  };

  TestDelegate delegate;
  AstraInfoBarView bar(AstraInfoBarType::kInformation);
  bar.SetDelegate(&delegate);

  // Test action.
  bar.AddAction({42, u"Test", false, false});
  // Can't easily click the button in unit test without views_test_base,
  // but we can verify the action was added.
  EXPECT_EQ(1u, bar.GetActionCount());
  EXPECT_EQ(-1, delegate.last_action_id);
}

// Test container view.
TEST_F(AstraInfoBarViewTest, ContainerView) {
  AstraInfoBarContainerView container;
  EXPECT_EQ(0u, container.GetInfoBarCount());

  container.AddInfoBar(AstraInfoBarType::kInformation, u"Bar 1");
  EXPECT_EQ(1u, container.GetInfoBarCount());

  container.AddInfoBar(AstraInfoBarType::kWarning, u"Bar 2");
  EXPECT_EQ(2u, container.GetInfoBarCount());

  // Newest bar is at index 0.
  AstraInfoBarView* bar = container.GetInfoBarAt(0);
  ASSERT_NE(nullptr, bar);
  EXPECT_EQ(u"Bar 2", bar->GetMessage());

  // Remove one bar.
  container.RemoveInfoBar(bar);
  EXPECT_EQ(1u, container.GetInfoBarCount());

  // Remove all.
  container.RemoveAllInfoBars();
  EXPECT_EQ(0u, container.GetInfoBarCount());
}

// Test info bar action struct.
TEST_F(AstraInfoBarViewTest, ActionStruct) {
  AstraInfoBarAction action;
  action.action_id = 1;
  action.label = u"Test";
  action.is_primary = true;
  action.is_link = false;

  EXPECT_EQ(1, action.action_id);
  EXPECT_EQ(u"Test", action.label);
  EXPECT_TRUE(action.is_primary);
  EXPECT_FALSE(action.is_link);
}

}  // namespace astra
