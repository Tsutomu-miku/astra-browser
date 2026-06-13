// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/workspace/astra_workspace_switcher_bubble.h"

#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

// ===========================================================================
// AstraWorkspaceSwitcherBubbleTest
// ===========================================================================

class AstraWorkspaceSwitcherBubbleTest : public testing::Test {
 protected:
  void SetUp() override {
    anchor_view_ = std::make_unique<views::View>();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<views::View> anchor_view_;
};

// Test switcher item creation.
TEST_F(AstraWorkspaceSwitcherBubbleTest, SwitcherItemCreation) {
  AstraWorkspace ws;
  ws.id = "ws1";
  ws.name = u"Work";
  ws.description = u"Work workspace";
  ws.color = "#5B8FF9";
  ws.tab_count = 12;
  ws.window_count = 2;
  ws.is_active = true;

  bool clicked = false;
  std::string clicked_id;

  auto item = std::make_unique<AstraWorkspaceSwitcherItemView>(
      ws,
      base::BindRepeating(
          [](bool* clicked, std::string* id, const std::string& ws_id) {
            *clicked = true;
            *id = ws_id;
          },
          &clicked, &clicked_id));

  EXPECT_EQ("ws1", item->workspace_id());
  EXPECT_EQ(u"Work", item->workspace_name());
  EXPECT_TRUE(item->IsActive());
}

// Test switcher item active state.
TEST_F(AstraWorkspaceSwitcherBubbleTest, SwitcherItemActiveState) {
  AstraWorkspace ws;
  ws.id = "ws2";
  ws.name = u"Personal";
  ws.color = "#61DDAA";
  ws.is_active = false;

  auto item = std::make_unique<AstraWorkspaceSwitcherItemView>(
      ws, base::DoNothing());

  EXPECT_FALSE(item->IsActive());

  item->SetIsActive(true);
  EXPECT_TRUE(item->IsActive());

  item->SetIsActive(false);
  EXPECT_FALSE(item->IsActive());
}

// Test switcher item hover state.
TEST_F(AstraWorkspaceSwitcherBubbleTest, SwitcherItemHoverState) {
  AstraWorkspace ws;
  ws.id = "ws3";
  ws.name = u"Research";
  ws.color = "#F6BD16";

  auto item = std::make_unique<AstraWorkspaceSwitcherItemView>(
      ws, base::DoNothing());

  item->SetIsHovered(true);
  item->SetIsHovered(false);
  // Should not crash.
}

// Test switcher bubble creation (no widget).
TEST_F(AstraWorkspaceSwitcherBubbleTest, BubbleCreation) {
  bool selected_called = false;
  std::string selected_id;
  bool new_called = false;

  auto* bubble = new AstraWorkspaceSwitcherBubble(
      anchor_view_.get(),
      nullptr,  // service
      base::BindRepeating(
          [](bool* called, std::string* id, const std::string& ws_id) {
            *called = true;
            *id = ws_id;
          },
          &selected_called, &selected_id),
      base::BindRepeating(
          [](bool* called) { *called = true; },
          &new_called));

  EXPECT_NE(nullptr, bubble);
}

// Test setting workspaces on bubble.
TEST_F(AstraWorkspaceSwitcherBubbleTest, SetWorkspaces) {
  auto* bubble = new AstraWorkspaceSwitcherBubble(
      anchor_view_.get(), nullptr,
      base::DoNothing(), base::DoNothing());

  std::vector<AstraWorkspace> workspaces;

  AstraWorkspace ws1;
  ws1.id = "ws1";
  ws1.name = u"Work";
  ws1.color = "#5B8FF9";
  ws1.tab_count = 10;
  ws1.is_active = true;
  workspaces.push_back(ws1);

  AstraWorkspace ws2;
  ws2.id = "ws2";
  ws2.name = u"Personal";
  ws2.color = "#61DDAA";
  ws2.tab_count = 5;
  ws2.is_active = false;
  workspaces.push_back(ws2);

  AstraWorkspace ws3;
  ws3.id = "ws3";
  ws3.name = u"Research";
  ws3.color = "#F6BD16";
  ws3.tab_count = 20;
  ws3.is_active = false;
  workspaces.push_back(ws3);

  bubble->SetWorkspaces(workspaces);
  bubble->SetActiveWorkspaceId("ws1");

  // Should not crash.
}

// Test setting active workspace.
TEST_F(AstraWorkspaceSwitcherBubbleTest, SetActiveWorkspace) {
  auto* bubble = new AstraWorkspaceSwitcherBubble(
      anchor_view_.get(), nullptr,
      base::DoNothing(), base::DoNothing());

  std::vector<AstraWorkspace> workspaces;

  AstraWorkspace ws1;
  ws1.id = "ws1";
  ws1.name = u"First";
  ws1.color = "#5B8FF9";
  ws1.is_active = true;
  workspaces.push_back(ws1);

  AstraWorkspace ws2;
  ws2.id = "ws2";
  ws2.name = u"Second";
  ws2.color = "#61DDAA";
  ws2.is_active = false;
  workspaces.push_back(ws2);

  bubble->SetWorkspaces(workspaces);
  bubble->SetActiveWorkspaceId("ws2");
  // Should update active state on items.
}

// Test search/filter.
TEST_F(AstraWorkspaceSwitcherBubbleTest, SearchFilter) {
  auto* bubble = new AstraWorkspaceSwitcherBubble(
      anchor_view_.get(), nullptr,
      base::DoNothing(), base::DoNothing());

  std::vector<AstraWorkspace> workspaces;

  AstraWorkspace ws1;
  ws1.id = "ws1";
  ws1.name = u"Work Projects";
  ws1.color = "#5B8FF9";
  workspaces.push_back(ws1);

  AstraWorkspace ws2;
  ws2.id = "ws2";
  ws2.name = u"Personal";
  ws2.color = "#61DDAA";
  workspaces.push_back(ws2);

  bubble->SetWorkspaces(workspaces);
  bubble->SetSearchQuery(u"Work");
  // Should filter to show only "Work Projects".
}

// Test bubble title.
TEST_F(AstraWorkspaceSwitcherBubbleTest, WindowTitle) {
  auto* bubble = new AstraWorkspaceSwitcherBubble(
      anchor_view_.get(), nullptr,
      base::DoNothing(), base::DoNothing());

  EXPECT_EQ(u"Switch Workspace", bubble->GetWindowTitle());
}

}  // namespace astra
