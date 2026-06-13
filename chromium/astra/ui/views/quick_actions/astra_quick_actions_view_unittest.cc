// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/quick_actions/astra_quick_actions_view.h"

#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

// ===========================================================================
// AstraQuickActionsViewTest
// ===========================================================================

class AstraQuickActionsViewTest : public testing::Test {
 protected:
  void SetUp() override {
    anchor_view_ = std::make_unique<views::View>();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<views::View> anchor_view_;
};

// Test quick action item creation.
TEST_F(AstraQuickActionsViewTest, ActionItemCreation) {
  AstraQuickActionItemView::ActionInfo info;
  info.action_id = "test-action";
  info.label = u"Test Action";
  info.icon = AstraQuickActionItemView::ActionIcon::kNewTab;

  auto item = std::make_unique<AstraQuickActionItemView>(
      info, base::DoNothing());

  EXPECT_EQ("test-action", item->action_id());
}

// Test quick action item active state.
TEST_F(AstraQuickActionsViewTest, ActionItemActiveState) {
  AstraQuickActionItemView::ActionInfo info;
  info.action_id = "focus-mode";
  info.label = u"Focus Mode";
  info.icon = AstraQuickActionItemView::ActionIcon::kFocusMode;

  auto item = std::make_unique<AstraQuickActionItemView>(
      info, base::DoNothing());

  item->SetActive(true);
  item->SetActive(false);
  // Should not crash.
}

// Test quick actions view creation.
TEST_F(AstraQuickActionsViewTest, ViewCreation) {
  auto* view = new AstraQuickActionsView(anchor_view_.get());
  EXPECT_NE(nullptr, view);
}

// Test action triggered callback.
TEST_F(AstraQuickActionsViewTest, ActionTriggeredCallback) {
  auto* view = new AstraQuickActionsView(anchor_view_.get());

  bool triggered = false;
  std::string triggered_action_id;

  view->SetActionTriggeredCallback(
      base::BindRepeating(
          [](bool* t, std::string* id, const std::string& action_id) {
            *t = true;
            *id = action_id;
          },
          &triggered, &triggered_action_id));

  // Callback can be set without crashing.
}

// Test setting action active by id.
TEST_F(AstraQuickActionsViewTest, SetActionActiveById) {
  auto* view = new AstraQuickActionsView(anchor_view_.get());

  view->SetActionActive("focus-mode", true);
  view->SetActionActive("focus-mode", false);
  view->SetActionActive("nonexistent-action", true);
  // Should not crash even for unknown actions.
}

// Test window title.
TEST_F(AstraQuickActionsViewTest, WindowTitle) {
  auto* view = new AstraQuickActionsView(anchor_view_.get());
  EXPECT_EQ(u"Quick Actions", view->GetWindowTitle());
}

// Test theme change doesn't crash.
TEST_F(AstraQuickActionsViewTest, ThemeChange) {
  auto* view = new AstraQuickActionsView(anchor_view_.get());
  view->OnThemeChanged();
}

// Test all action icon types can be created.
TEST_F(AstraQuickActionsViewTest, AllIconTypes) {
  std::vector<AstraQuickActionItemView::ActionIcon> icons = {
      AstraQuickActionItemView::ActionIcon::kNewTab,
      AstraQuickActionItemView::ActionIcon::kCloseTab,
      AstraQuickActionItemView::ActionIcon::kPinTab,
      AstraQuickActionItemView::ActionIcon::kMuteTab,
      AstraQuickActionItemView::ActionIcon::kDuplicateTab,
      AstraQuickActionItemView::ActionIcon::kSleepTab,
      AstraQuickActionItemView::ActionIcon::kBack,
      AstraQuickActionItemView::ActionIcon::kForward,
      AstraQuickActionItemView::ActionIcon::kReload,
      AstraQuickActionItemView::ActionIcon::kBookmark,
      AstraQuickActionItemView::ActionIcon::kFocusMode,
      AstraQuickActionItemView::ActionIcon::kWorkspace,
      AstraQuickActionItemView::ActionIcon::kSidebar,
      AstraQuickActionItemView::ActionIcon::kSplitView,
      AstraQuickActionItemView::ActionIcon::kReadingList,
      AstraQuickActionItemView::ActionIcon::kFind,
      AstraQuickActionItemView::ActionIcon::kPrint,
      AstraQuickActionItemView::ActionIcon::kZoomIn,
      AstraQuickActionItemView::ActionIcon::kZoomOut,
      AstraQuickActionItemView::ActionIcon::kFullscreen,
      AstraQuickActionItemView::ActionIcon::kDevTools,
  };

  for (size_t i = 0; i < icons.size(); ++i) {
    AstraQuickActionItemView::ActionInfo info;
    info.action_id = "icon-" + std::to_string(i);
    info.label = base::UTF8ToUTF16("Icon " + std::to_string(i));
    info.icon = icons[i];

    auto item = std::make_unique<AstraQuickActionItemView>(
        info, base::DoNothing());

    EXPECT_EQ(info.action_id, item->action_id());
  }
}

// Test multiple active actions.
TEST_F(AstraQuickActionsViewTest, MultipleActiveActions) {
  auto* view = new AstraQuickActionsView(anchor_view_.get());

  view->SetActionActive("focus-mode", true);
  view->SetActionActive("sidebar", true);
  view->SetActionActive("pin-tab", true);

  view->SetActionActive("focus-mode", false);
  // Other actions should remain active.
}

// Test action item mouse events don't crash.
TEST_F(AstraQuickActionsViewTest, ActionItemMouseEvents) {
  AstraQuickActionItemView::ActionInfo info;
  info.action_id = "test-mouse";
  info.label = u"Mouse Test";
  info.icon = AstraQuickActionItemView::ActionIcon::kNewTab;

  bool callback_run = false;
  auto item = std::make_unique<AstraQuickActionItemView>(
      info,
      base::BindRepeating(
          [](bool* run) { *run = true; },
          &callback_run));

  // Simulate mouse enter.
  ui::MouseEvent enter(ui::ET_MOUSE_ENTERED, gfx::Point(),
                       gfx::Point(), base::TimeTicks(), 0, 0);
  item->OnMouseEntered(enter);

  // Simulate mouse press.
  ui::MouseEvent press(ui::ET_MOUSE_PRESSED,
                       gfx::Point(20, 20), gfx::Point(20, 20),
                       base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON,
                       ui::EF_LEFT_MOUSE_BUTTON);
  item->OnMousePressed(press);

  // Simulate mouse release (should trigger callback).
  ui::MouseEvent release(ui::ET_MOUSE_RELEASED,
                         gfx::Point(20, 20), gfx::Point(20, 20),
                         base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON,
                         ui::EF_LEFT_MOUSE_BUTTON);
  item->OnMouseReleased(release);

  // Simulate mouse exit.
  ui::MouseEvent exit(ui::ET_MOUSE_EXITED, gfx::Point(),
                      gfx::Point(), base::TimeTicks(), 0, 0);
  item->OnMouseExited(exit);
}

}  // namespace astra
