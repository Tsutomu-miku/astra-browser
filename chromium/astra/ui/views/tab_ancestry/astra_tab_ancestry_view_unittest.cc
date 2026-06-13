// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/tab_ancestry/astra_tab_ancestry_view.h"

#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

// ===========================================================================
// AstraTabAncestryViewTest
// ===========================================================================

class AstraTabAncestryViewTest : public testing::Test {
 protected:
  void SetUp() override {
    anchor_view_ = std::make_unique<views::View>();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<views::View> anchor_view_;
};

// Test single ancestry node creation.
TEST_F(AstraTabAncestryViewTest, NodeCreation) {
  AstraTabAncestryNodeView::TabNode node;
  node.tab_id = "tab_root";
  node.title = u"Google Search";
  node.domain = "google.com";
  node.depth = 0;

  auto view = std::make_unique<AstraTabAncestryNodeView>(
      node, base::DoNothing(), base::DoNothing());

  EXPECT_EQ("tab_root", view->tab_id());
  EXPECT_TRUE(view->expanded());
}

// Test node with children.
TEST_F(AstraTabAncestryViewTest, NodeWithChildren) {
  AstraTabAncestryNodeView::TabNode root;
  root.tab_id = "root";
  root.title = u"Root";
  root.domain = "root.com";
  root.depth = 0;

  AstraTabAncestryNodeView::TabNode child1;
  child1.tab_id = "child1";
  child1.title = u"Child 1";
  child1.domain = "child1.com";
  child1.depth = 1;
  root.children.push_back(child1);

  AstraTabAncestryNodeView::TabNode child2;
  child2.tab_id = "child2";
  child2.title = u"Child 2";
  child2.domain = "child2.com";
  child2.depth = 1;
  root.children.push_back(child2);

  auto view = std::make_unique<AstraTabAncestryNodeView>(
      root, base::DoNothing(), base::DoNothing());

  EXPECT_EQ("root", view->tab_id());
  EXPECT_TRUE(view->expanded());
}

// Test node expand/collapse.
TEST_F(AstraTabAncestryViewTest, NodeExpandCollapse) {
  AstraTabAncestryNodeView::TabNode root;
  root.tab_id = "root";
  root.title = u"Root";
  root.depth = 0;

  AstraTabAncestryNodeView::TabNode child;
  child.tab_id = "child";
  child.title = u"Child";
  child.depth = 1;
  root.children.push_back(child);

  auto view = std::make_unique<AstraTabAncestryNodeView>(
      root, base::DoNothing(), base::DoNothing());

  EXPECT_TRUE(view->expanded());
  view->SetExpanded(false);
  EXPECT_FALSE(view->expanded());
  view->SetExpanded(true);
  EXPECT_TRUE(view->expanded());
}

// Test active tab state.
TEST_F(AstraTabAncestryViewTest, ActiveTabState) {
  AstraTabAncestryNodeView::TabNode node;
  node.tab_id = "tab1";
  node.title = u"Active Tab";
  node.is_active = true;

  auto view = std::make_unique<AstraTabAncestryNodeView>(
      node, base::DoNothing(), base::DoNothing());

  EXPECT_EQ("tab1", view->tab_id());
}

// Test deep nesting.
TEST_F(AstraTabAncestryViewTest, DeepNesting) {
  AstraTabAncestryNodeView::TabNode root;
  root.tab_id = "level0";
  root.title = u"Level 0";
  root.depth = 0;

  AstraTabAncestryNodeView::TabNode* current = &root;
  for (int i = 1; i <= 5; ++i) {
    AstraTabAncestryNodeView::TabNode child;
    child.tab_id = "level" + std::to_string(i);
    child.title = base::UTF8ToUTF16("Level " + std::to_string(i));
    child.depth = i;
    current->children.push_back(child);
    current = &current->children.back();
  }

  auto view = std::make_unique<AstraTabAncestryNodeView>(
      root, base::DoNothing(), base::DoNothing());

  EXPECT_EQ("level0", view->tab_id());
  EXPECT_TRUE(view->expanded());
}

// Test ancestry view creation.
TEST_F(AstraTabAncestryViewTest, ViewCreation) {
  auto* view = new AstraTabAncestryView(anchor_view_.get());
  EXPECT_NE(nullptr, view);
}

// Test setting root nodes.
TEST_F(AstraTabAncestryViewTest, SetRootNodes) {
  auto* view = new AstraTabAncestryView(anchor_view_.get());

  std::vector<AstraTabAncestryNodeView::TabNode> roots;

  AstraTabAncestryNodeView::TabNode root1;
  root1.tab_id = "root1";
  root1.title = u"Google Search";
  root1.domain = "google.com";
  root1.depth = 0;
  roots.push_back(root1);

  AstraTabAncestryNodeView::TabNode root2;
  root2.tab_id = "root2";
  root2.title = u"Direct Nav";
  root2.domain = "example.com";
  root2.depth = 0;
  roots.push_back(root2);

  view->SetRootNodes(roots);
  // Should not crash.
}

// Test empty root nodes.
TEST_F(AstraTabAncestryViewTest, EmptyRootNodes) {
  auto* view = new AstraTabAncestryView(anchor_view_.get());
  view->SetRootNodes({});
  // Should not crash with empty list.
}

// Test stats updates.
TEST_F(AstraTabAncestryViewTest, StatsUpdates) {
  auto* view = new AstraTabAncestryView(anchor_view_.get());

  view->SetTabCount(12);
  view->SetBranchCount(3);
  view->SetMaxDepth(4);
  // Should not crash.
}

// Test active tab.
TEST_F(AstraTabAncestryViewTest, ActiveTab) {
  auto* view = new AstraTabAncestryView(anchor_view_.get());
  view->SetActiveTabId("tab_42");
}

// Test tab activated callback.
TEST_F(AstraTabAncestryViewTest, TabActivatedCallback) {
  auto* view = new AstraTabAncestryView(anchor_view_.get());

  bool triggered = false;
  std::string activated_id;

  view->SetTabActivatedCallback(
      base::BindRepeating(
          [](bool* t, std::string* id, const std::string& tab_id) {
            *t = true;
            *id = tab_id;
          },
          &triggered, &activated_id));
}

// Test collapse/expand all callbacks.
TEST_F(AstraTabAncestryViewTest, CollapseExpandAllCallbacks) {
  auto* view = new AstraTabAncestryView(anchor_view_.get());

  bool collapse_triggered = false;
  view->SetCollapseAllCallback(
      base::BindRepeating(
          [](bool* t) { *t = true; },
          &collapse_triggered));

  bool expand_triggered = false;
  view->SetExpandAllCallback(
      base::BindRepeating(
          [](bool* t) { *t = true; },
          &expand_triggered));
}

// Test window title.
TEST_F(AstraTabAncestryViewTest, WindowTitle) {
  auto* view = new AstraTabAncestryView(anchor_view_.get());
  EXPECT_EQ(u"Tab Family Tree", view->GetWindowTitle());
}

// Test theme change doesn't crash.
TEST_F(AstraTabAncestryViewTest, ThemeChange) {
  auto* view = new AstraTabAncestryView(anchor_view_.get());
  view->OnThemeChanged();
}

// Test theme change on node view.
TEST_F(AstraTabAncestryViewTest, NodeThemeChange) {
  AstraTabAncestryNodeView::TabNode node;
  node.tab_id = "theme_test";
  node.title = u"Theme Test";
  node.depth = 0;

  auto view = std::make_unique<AstraTabAncestryNodeView>(
      node, base::DoNothing(), base::DoNothing());
  view->OnThemeChanged();
}

// Test complex tree with multiple branches.
TEST_F(AstraTabAncestryViewTest, ComplexTree) {
  auto* view = new AstraTabAncestryView(anchor_view_.get());

  std::vector<AstraTabAncestryNodeView::TabNode> roots;

  // Root 1 with two levels of children.
  AstraTabAncestryNodeView::TabNode root1;
  root1.tab_id = "r1";
  root1.title = u"Root 1";
  root1.depth = 0;

  for (int i = 0; i < 3; ++i) {
    AstraTabAncestryNodeView::TabNode child;
    child.tab_id = "r1_c" + std::to_string(i);
    child.title = base::UTF8ToUTF16("Child " + std::to_string(i));
    child.depth = 1;

    // Add grandchildren.
    for (int j = 0; j < 2; ++j) {
      AstraTabAncestryNodeView::TabNode grandchild;
      grandchild.tab_id = "r1_c" + std::to_string(i) +
                           "_gc" + std::to_string(j);
      grandchild.title = base::UTF8ToUTF16("Grandchild " + std::to_string(j));
      grandchild.depth = 2;
      child.children.push_back(grandchild);
    }

    root1.children.push_back(child);
  }
  roots.push_back(root1);

  // Root 2 with no children.
  AstraTabAncestryNodeView::TabNode root2;
  root2.tab_id = "r2";
  root2.title = u"Root 2";
  root2.depth = 0;
  roots.push_back(root2);

  view->SetRootNodes(roots);
  view->SetTabCount(10);
  view->SetBranchCount(4);
  view->SetMaxDepth(3);
}

}  // namespace astra
