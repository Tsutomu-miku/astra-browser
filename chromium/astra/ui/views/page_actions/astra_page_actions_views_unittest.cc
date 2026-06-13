// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/page_actions/astra_page_actions_view.h"

#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

class AstraPageActionsModelTest : public testing::Test {
 protected:
  void SetUp() override {
    model_ = std::make_unique<AstraPageActionsModel>();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<AstraPageActionsModel> model_;
};

// Test that the model starts empty.
TEST_F(AstraPageActionsModelTest, StartsEmpty) {
  EXPECT_EQ(0u, model_->GetAllActions().size());
  EXPECT_EQ(0u, model_->GetVisibleActionCount());
  EXPECT_EQ(0u, model_->GetPinnedActionCount());
}

// Test PopulateDefaultActions adds the expected actions.
TEST_F(AstraPageActionsModelTest, PopulateDefaultActions) {
  model_->PopulateDefaultActions();
  EXPECT_GT(model_->GetAllActions().size(), 0u);
  EXPECT_GT(model_->GetVisibleActionCount(), 0u);
  EXPECT_GE(model_->GetPinnedActionCount(), 0u);
}

// Test SetAction adds a new action.
TEST_F(AstraPageActionsModelTest, SetActionAddsNew) {
  AstraPageActionItem item;
  item.type = AstraPageActionType::kBookmarkStar;
  item.label = u"Bookmark";
  item.visible = true;
  item.pinned = true;

  model_->SetAction(item);
  EXPECT_EQ(1u, model_->GetAllActions().size());
  EXPECT_EQ(1u, model_->GetVisibleActionCount());
  EXPECT_EQ(1u, model_->GetPinnedActionCount());

  const AstraPageActionItem* found = model_->GetAction(AstraPageActionType::kBookmarkStar);
  ASSERT_NE(nullptr, found);
  EXPECT_EQ(u"Bookmark", found->label);
}

// Test SetAction updates an existing action.
TEST_F(AstraPageActionsModelTest, SetActionUpdatesExisting) {
  AstraPageActionItem item;
  item.type = AstraPageActionType::kBookmarkStar;
  item.label = u"Bookmark";
  model_->SetAction(item);

  item.label = u"Updated Bookmark";
  model_->SetAction(item);

  EXPECT_EQ(1u, model_->GetAllActions().size());
  const AstraPageActionItem* found = model_->GetAction(AstraPageActionType::kBookmarkStar);
  ASSERT_NE(nullptr, found);
  EXPECT_EQ(u"Updated Bookmark", found->label);
}

// Test RemoveAction.
TEST_F(AstraPageActionsModelTest, RemoveAction) {
  AstraPageActionItem item;
  item.type = AstraPageActionType::kBookmarkStar;
  model_->SetAction(item);
  EXPECT_EQ(1u, model_->GetAllActions().size());

  model_->RemoveAction(AstraPageActionType::kBookmarkStar);
  EXPECT_EQ(0u, model_->GetAllActions().size());
  EXPECT_EQ(nullptr, model_->GetAction(AstraPageActionType::kBookmarkStar));
}

// Test SetActionState.
TEST_F(AstraPageActionsModelTest, SetActionState) {
  AstraPageActionItem item;
  item.type = AstraPageActionType::kBookmarkStar;
  item.state = AstraPageActionState::kDefault;
  model_->SetAction(item);

  model_->SetActionState(AstraPageActionType::kBookmarkStar,
                         AstraPageActionState::kActive);

  const AstraPageActionItem* found = model_->GetAction(AstraPageActionType::kBookmarkStar);
  ASSERT_NE(nullptr, found);
  EXPECT_EQ(AstraPageActionState::kActive, found->state);
}

// Test SetActionBadge.
TEST_F(AstraPageActionsModelTest, SetActionBadge) {
  AstraPageActionItem item;
  item.type = AstraPageActionType::kSavePassword;
  model_->SetAction(item);

  model_->SetActionBadge(AstraPageActionType::kSavePassword, u"42", SK_ColorBLUE);

  const AstraPageActionItem* found = model_->GetAction(AstraPageActionType::kSavePassword);
  ASSERT_NE(nullptr, found);
  EXPECT_EQ(u"42", found->badge_text);
  EXPECT_EQ(SK_ColorBLUE, found->badge_color);
}

// Test SetActionVisible.
TEST_F(AstraPageActionsModelTest, SetActionVisible) {
  AstraPageActionItem item;
  item.type = AstraPageActionType::kTranslate;
  item.visible = true;
  model_->SetAction(item);
  EXPECT_EQ(1u, model_->GetVisibleActionCount());

  model_->SetActionVisible(AstraPageActionType::kTranslate, false);
  EXPECT_EQ(0u, model_->GetVisibleActionCount());
}

// Test SetActionPinned.
TEST_F(AstraPageActionsModelTest, SetActionPinned) {
  AstraPageActionItem item;
  item.type = AstraPageActionType::kBookmarkStar;
  item.visible = true;
  item.pinned = false;
  model_->SetAction(item);
  EXPECT_EQ(0u, model_->GetPinnedActionCount());

  model_->SetActionPinned(AstraPageActionType::kBookmarkStar, true);
  EXPECT_EQ(1u, model_->GetPinnedActionCount());
}

// Test extension actions.
TEST_F(AstraPageActionsModelTest, ExtensionActions) {
  model_->SetExtensionAction("ext_123", u"My Extension", "ext_icon", false);

  const AstraPageActionItem* found = model_->GetExtensionAction("ext_123");
  ASSERT_NE(nullptr, found);
  EXPECT_EQ(u"My Extension", found->extension_name);
  EXPECT_EQ("ext_123", found->extension_id);
  EXPECT_EQ(AstraPageActionType::kExtensionAction, found->type);

  EXPECT_EQ(1u, model_->GetExtensionActions().size());

  model_->RemoveExtensionAction("ext_123");
  EXPECT_EQ(nullptr, model_->GetExtensionAction("ext_123"));
}

// Test max visible actions budget.
TEST_F(AstraPageActionsModelTest, MaxVisibleActions) {
  model_->PopulateDefaultActions();
  size_t original_pinned = model_->GetPinnedActions().size();

  model_->SetMaxVisibleActions(3);
  EXPECT_EQ(3u, model_->GetPinnedActions().size());
  EXPECT_GT(model_->GetOverflowActions().size(), 0u);

  model_->SetMaxVisibleActions(0);  // No limit.
  EXPECT_EQ(original_pinned, model_->GetPinnedActions().size());
}

// Test GetPinnedActions vs GetOverflowActions.
TEST_F(AstraPageActionsModelTest, PinnedVsOverflow) {
  for (int i = 0; i < 5; i++) {
    AstraPageActionItem item;
    item.type = static_cast<AstraPageActionType>(i + 1);
    item.visible = true;
    item.pinned = (i < 3);  // First 3 are pinned.
    item.order = i * 10;
    model_->SetAction(item);
  }

  EXPECT_EQ(3u, model_->GetPinnedActions().size());
  EXPECT_EQ(2u, model_->GetOverflowActions().size());
}

// Test ClearAllActions.
TEST_F(AstraPageActionsModelTest, ClearAllActions) {
  model_->PopulateDefaultActions();
  EXPECT_GT(model_->GetAllActions().size(), 0u);

  model_->ClearAllActions();
  EXPECT_EQ(0u, model_->GetAllActions().size());
}

// Test compact mode.
TEST_F(AstraPageActionsModelTest, CompactMode) {
  EXPECT_FALSE(model_->GetCompactMode());

  model_->SetCompactMode(true);
  EXPECT_TRUE(model_->GetCompactMode());

  model_->SetCompactMode(false);
  EXPECT_FALSE(model_->GetCompactMode());
}

// Test that Astra-specific action types exist.
TEST_F(AstraPageActionsModelTest, AstraSpecificActionTypes) {
  model_->PopulateDefaultActions();

  // These should be in the default set.
  EXPECT_NE(nullptr, model_->GetAction(AstraPageActionType::kSidebar));
  EXPECT_NE(nullptr, model_->GetAction(AstraPageActionType::kReadingList));
  EXPECT_NE(nullptr, model_->GetAction(AstraPageActionType::kNote));
  EXPECT_NE(nullptr, model_->GetAction(AstraPageActionType::kScreenshot));
  EXPECT_NE(nullptr, model_->GetAction(AstraPageActionType::kFavorite));
  EXPECT_NE(nullptr, model_->GetAction(AstraPageActionType::kFocusMode));
  EXPECT_NE(nullptr, model_->GetAction(AstraPageActionType::kSplitView));
  EXPECT_NE(nullptr, model_->GetAction(AstraPageActionType::kCommandPalette));
}

// Test standard Chromium action types exist.
TEST_F(AstraPageActionsModelTest, StandardActionTypes) {
  model_->PopulateDefaultActions();

  EXPECT_NE(nullptr, model_->GetAction(AstraPageActionType::kBookmarkStar));
  EXPECT_NE(nullptr, model_->GetAction(AstraPageActionType::kZoom));
  EXPECT_NE(nullptr, model_->GetAction(AstraPageActionType::kTranslate));
  EXPECT_NE(nullptr, model_->GetAction(AstraPageActionType::kFind));
  EXPECT_NE(nullptr, model_->GetAction(AstraPageActionType::kPrint));
}

// Test AstraPageActionView basic creation.
TEST(AstraPageActionViewTest, BasicCreation) {
  AstraPageActionView view(AstraPageActionType::kBookmarkStar);
  EXPECT_EQ(AstraPageActionType::kBookmarkStar, view.GetActionType());
}

// Test AstraPageActionView label.
TEST(AstraPageActionViewTest, Label) {
  AstraPageActionView view(AstraPageActionType::kBookmarkStar);
  view.SetLabel(u"Bookmark this page");
  EXPECT_EQ(u"Bookmark this page", view.GetLabel());
}

// Test AstraPageActionView badge.
TEST(AstraPageActionViewTest, Badge) {
  AstraPageActionView view(AstraPageActionType::kSavePassword);
  EXPECT_FALSE(view.HasBadge());

  view.SetBadgeText(u"42");
  EXPECT_TRUE(view.HasBadge());
  EXPECT_EQ(u"42", view.GetBadgeText());

  view.SetBadgeColor(SK_ColorGREEN);
  EXPECT_EQ(SK_ColorGREEN, view.GetBadgeColor());
}

// Test AstraPageActionView state.
TEST(AstraPageActionViewTest, State) {
  AstraPageActionView view(AstraPageActionType::kBookmarkStar);
  EXPECT_EQ(AstraPageActionState::kDefault, view.GetActionState());

  view.SetActionState(AstraPageActionState::kActive);
  EXPECT_EQ(AstraPageActionState::kActive, view.GetActionState());
}

// Test AstraPageActionView icon size.
TEST(AstraPageActionViewTest, IconSize) {
  AstraPageActionView view(AstraPageActionType::kBookmarkStar);
  EXPECT_EQ(20, view.GetIconSize());

  view.SetIconSize(16);
  EXPECT_EQ(16, view.GetIconSize());
}

// Test AstraPageActionView extension ID.
TEST(AstraPageActionViewTest, ExtensionId) {
  AstraPageActionView view(AstraPageActionType::kExtensionAction);
  EXPECT_TRUE(view.GetExtensionId().empty());

  view.SetExtensionId("ext_abc");
  EXPECT_EQ("ext_abc", view.GetExtensionId());
}

// Test AstraPageActionsView basic creation.
TEST(AstraPageActionsViewTest, BasicCreation) {
  AstraPageActionsView view;
  EXPECT_EQ(nullptr, view.GetModel());
  EXPECT_EQ(0u, view.GetPinnedActionCount());
  EXPECT_EQ(0u, view.GetTotalActionCount());
}

// Test AstraPageActionsView with model.
TEST(AstraPageActionsViewTest, WithModel) {
  auto model = std::make_unique<AstraPageActionsModel>();
  model->PopulateDefaultActions();

  AstraPageActionsView view(model.get());
  EXPECT_EQ(model.get(), view.GetModel());
  EXPECT_GT(view.GetTotalActionCount(), 0u);
}

// Test AstraPageActionsView icon size.
TEST(AstraPageActionsViewTest, IconSize) {
  AstraPageActionsView view;
  EXPECT_EQ(20, view.GetIconSize());

  view.SetIconSize(16);
  EXPECT_EQ(16, view.GetIconSize());
}

// Test AstraPageActionsView spacing.
TEST(AstraPageActionsViewTest, Spacing) {
  AstraPageActionsView view;
  EXPECT_EQ(2, view.GetSpacing());

  view.SetSpacing(4);
  EXPECT_EQ(4, view.GetSpacing());
}

// Test AstraPageActionsView overflow button exists.
TEST(AstraPageActionsViewTest, OverflowButtonExists) {
  AstraPageActionsView view;
  EXPECT_NE(nullptr, view.overflow_button());
}

// Test that the model observer fires on action changes.
TEST_F(AstraPageActionsModelTest, ObserverFires) {
  class TestObserver : public AstraPageActionsObserver {
   public:
    int actions_changed_count = 0;
    int action_changed_count = 0;
    AstraPageActionType last_changed_type = AstraPageActionType::kNone;

    void OnActionsChanged(AstraPageActionsModel* model) override {
      actions_changed_count++;
    }
    void OnActionChanged(AstraPageActionsModel* model,
                         AstraPageActionType type) override {
      action_changed_count++;
      last_changed_type = type;
    }
  };

  TestObserver observer;
  model_->AddObserver(&observer);

  // Adding an action should trigger OnActionsChanged.
  AstraPageActionItem item;
  item.type = AstraPageActionType::kBookmarkStar;
  model_->SetAction(item);
  EXPECT_EQ(1, observer.actions_changed_count);

  // Changing state should trigger OnActionChanged.
  model_->SetActionState(AstraPageActionType::kBookmarkStar,
                         AstraPageActionState::kActive);
  EXPECT_EQ(1, observer.action_changed_count);
  EXPECT_EQ(AstraPageActionType::kBookmarkStar, observer.last_changed_type);

  model_->RemoveObserver(&observer);
}

// Test WouldOverflow.
TEST_F(AstraPageActionsModelTest, WouldOverflow) {
  model_->SetMaxVisibleActions(3);
  EXPECT_FALSE(model_->WouldOverflow(0));
  EXPECT_FALSE(model_->WouldOverflow(2));
  EXPECT_TRUE(model_->WouldOverflow(3));
  EXPECT_TRUE(model_->WouldOverflow(10));

  model_->SetMaxVisibleActions(0);  // No limit.
  EXPECT_FALSE(model_->WouldOverflow(0));
  EXPECT_FALSE(model_->WouldOverflow(100));
}

}  // namespace astra
