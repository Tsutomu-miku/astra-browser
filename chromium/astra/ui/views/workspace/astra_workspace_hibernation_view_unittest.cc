// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/workspace/astra_workspace_hibernation_view.h"

#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

// ===========================================================================
// AstraWorkspaceHibernationViewTest
// ===========================================================================

class AstraWorkspaceHibernationViewTest : public testing::Test {
 protected:
  void SetUp() override {
    anchor_view_ = std::make_unique<views::View>();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<views::View> anchor_view_;
};

// Test hibernated workspace item creation.
TEST_F(AstraWorkspaceHibernationViewTest, HibernatedItemCreation) {
  AstraWorkspace ws;
  ws.id = "hibernated_1";
  ws.name = u"Old Projects";
  ws.color = "#7262FD";
  ws.tab_count = 15;
  ws.window_count = 2;
  ws.last_used_time = base::Time::Now() - base::Days(3);
  ws.is_hibernated = true;

  auto item = std::make_unique<AstraHibernatedWorkspaceItemView>(
      ws, base::DoNothing(), base::DoNothing());

  EXPECT_EQ("hibernated_1", item->workspace_id());
}

// Test hibernation view creation.
TEST_F(AstraWorkspaceHibernationViewTest, ViewCreation) {
  auto* view = new AstraWorkspaceHibernationView(anchor_view_.get());
  EXPECT_NE(nullptr, view);
}

// Test setting hibernated workspaces.
TEST_F(AstraWorkspaceHibernationViewTest, SetHibernatedWorkspaces) {
  auto* view = new AstraWorkspaceHibernationView(anchor_view_.get());

  std::vector<AstraWorkspace> workspaces;

  AstraWorkspace ws1;
  ws1.id = "ws1";
  ws1.name = u"Project Alpha";
  ws1.color = "#5B8FF9";
  ws1.tab_count = 10;
  ws1.is_hibernated = true;
  ws1.last_used_time = base::Time::Now() - base::Days(2);
  workspaces.push_back(ws1);

  AstraWorkspace ws2;
  ws2.id = "ws2";
  ws2.name = u"Project Beta";
  ws2.color = "#61DDAA";
  ws2.tab_count = 5;
  ws2.is_hibernated = true;
  ws2.last_used_time = base::Time::Now() - base::Days(7);
  workspaces.push_back(ws2);

  view->SetHibernatedWorkspaces(workspaces);
  // Should not crash.
}

// Test auto-hibernate settings.
TEST_F(AstraWorkspaceHibernationViewTest, AutoHibernateSettings) {
  auto* view = new AstraWorkspaceHibernationView(anchor_view_.get());

  view->SetAutoHibernateEnabled(true);
  view->SetAutoHibernateHours(24);
  view->SetAutoHibernateHours(48);
  view->SetAutoHibernateEnabled(false);
  // Should not crash.
}

// Test restore callback.
TEST_F(AstraWorkspaceHibernationViewTest, RestoreCallback) {
  auto* view = new AstraWorkspaceHibernationView(anchor_view_.get());

  bool restored = false;
  std::string restored_id;

  view->SetRestoreCallback(
      base::BindRepeating(
          [](bool* restored, std::string* id,
             const std::string& ws_id) {
            *restored = true;
            *id = ws_id;
          },
          &restored, &restored_id));

  // Callback can be set without crashing.
}

// Test delete callback.
TEST_F(AstraWorkspaceHibernationViewTest, DeleteCallback) {
  auto* view = new AstraWorkspaceHibernationView(anchor_view_.get());

  bool deleted = false;
  std::string deleted_id;

  view->SetDeleteCallback(
      base::BindRepeating(
          [](bool* deleted, std::string* id,
             const std::string& ws_id) {
            *deleted = true;
            *id = ws_id;
          },
          &deleted, &deleted_id));
}

// Test window title.
TEST_F(AstraWorkspaceHibernationViewTest, WindowTitle) {
  auto* view = new AstraWorkspaceHibernationView(anchor_view_.get());
  EXPECT_EQ(u"Hibernated Workspaces", view->GetWindowTitle());
}

// Test empty hibernated list.
TEST_F(AstraWorkspaceHibernationViewTest, EmptyHibernatedList) {
  auto* view = new AstraWorkspaceHibernationView(anchor_view_.get());
  view->SetHibernatedWorkspaces({});
  // Should not crash with empty list.
}

// Test theme change doesn't crash.
TEST_F(AstraWorkspaceHibernationViewTest, ThemeChange) {
  auto* view = new AstraWorkspaceHibernationView(anchor_view_.get());
  view->OnThemeChanged();
}

}  // namespace astra
