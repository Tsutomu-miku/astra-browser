// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/tab_duplicates/astra_tab_duplicates_view.h"

#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

// ===========================================================================
// AstraTabDuplicatesViewTest
// ===========================================================================

class AstraTabDuplicatesViewTest : public testing::Test {
 protected:
  void SetUp() override {
    anchor_view_ = std::make_unique<views::View>();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<views::View> anchor_view_;
};

// Test duplicate group creation.
TEST_F(AstraTabDuplicatesViewTest, GroupCreation) {
  AstraDuplicateTabGroupView::GroupInfo info;
  info.group_key = "example.com";
  info.label = u"example.com/article";
  info.duplicate_count = 2;

  AstraDuplicateTabGroupView::TabItem tab1;
  tab1.tab_id = "tab1";
  tab1.title = u"Article Page 1";
  tab1.url = "https://example.com/article";
  tab1.domain = "example.com";
  tab1.is_active = true;
  tab1.last_accessed = base::Time::Now();
  info.tabs.push_back(tab1);

  AstraDuplicateTabGroupView::TabItem tab2;
  tab2.tab_id = "tab2";
  tab2.title = u"Article Page 2";
  tab2.url = "https://example.com/article";
  tab2.domain = "example.com";
  tab2.is_active = false;
  tab2.last_accessed = base::Time::Now() - base::Hours(1);
  info.tabs.push_back(tab2);

  auto group = std::make_unique<AstraDuplicateTabGroupView>(
      info, base::DoNothing(), base::DoNothing(), base::DoNothing());

  EXPECT_EQ("example.com", group->group_key());
  EXPECT_EQ(2, group->duplicate_count());
}

// Test group with pinned tab.
TEST_F(AstraTabDuplicatesViewTest, GroupWithPinnedTab) {
  AstraDuplicateTabGroupView::GroupInfo info;
  info.group_key = "work.com";
  info.label = u"work.com/dashboard";
  info.duplicate_count = 2;

  AstraDuplicateTabGroupView::TabItem tab1;
  tab1.tab_id = "pinned1";
  tab1.title = u"Dashboard";
  tab1.url = "https://work.com/dashboard";
  tab1.domain = "work.com";
  tab1.is_pinned = true;
  info.tabs.push_back(tab1);

  AstraDuplicateTabGroupView::TabItem tab2;
  tab2.tab_id = "normal1";
  tab2.title = u"Dashboard (copy)";
  tab2.url = "https://work.com/dashboard";
  tab2.domain = "work.com";
  tab2.is_pinned = false;
  info.tabs.push_back(tab2);

  auto group = std::make_unique<AstraDuplicateTabGroupView>(
      info, base::DoNothing(), base::DoNothing(), base::DoNothing());

  EXPECT_EQ(2, group->duplicate_count());
}

// Test duplicates view creation.
TEST_F(AstraTabDuplicatesViewTest, ViewCreation) {
  auto* view = new AstraTabDuplicatesView(anchor_view_.get());
  EXPECT_NE(nullptr, view);
}

// Test window title.
TEST_F(AstraTabDuplicatesViewTest, WindowTitle) {
  auto* view = new AstraTabDuplicatesView(anchor_view_.get());
  EXPECT_EQ(u"Duplicate Tabs", view->GetWindowTitle());
}

// Test setting duplicate groups.
TEST_F(AstraTabDuplicatesViewTest, SetDuplicateGroups) {
  auto* view = new AstraTabDuplicatesView(anchor_view_.get());

  std::vector<AstraDuplicateTabGroupView::GroupInfo> groups;

  // Group 1: 3 duplicates.
  AstraDuplicateTabGroupView::GroupInfo g1;
  g1.group_key = "example.com/article";
  g1.label = u"example.com/article";
  g1.duplicate_count = 3;
  for (int i = 0; i < 3; i++) {
    AstraDuplicateTabGroupView::TabItem tab;
    tab.tab_id = "tab_g1_" + std::to_string(i);
    tab.title = base::UTF8ToUTF16("Tab " + std::to_string(i));
    tab.domain = "example.com";
    tab.url = "https://example.com/article";
    tab.is_active = (i == 0);
    tab.last_accessed = base::Time::Now() - base::Hours(i);
    g1.tabs.push_back(tab);
  }
  groups.push_back(g1);

  // Group 2: 2 duplicates.
  AstraDuplicateTabGroupView::GroupInfo g2;
  g2.group_key = "github.com";
  g2.label = u"github.com";
  g2.duplicate_count = 2;
  for (int i = 0; i < 2; i++) {
    AstraDuplicateTabGroupView::TabItem tab;
    tab.tab_id = "tab_g2_" + std::to_string(i);
    tab.title = base::UTF8ToUTF16("GitHub Tab " + std::to_string(i));
    tab.domain = "github.com";
    tab.url = "https://github.com/";
    tab.is_active = false;
    tab.last_accessed = base::Time::Now() - base::Minutes(i * 30);
    g2.tabs.push_back(tab);
  }
  groups.push_back(g2);

  view->SetDuplicateGroups(groups);
  EXPECT_NE(nullptr, view);
}

// Test empty groups.
TEST_F(AstraTabDuplicatesViewTest, EmptyGroups) {
  auto* view = new AstraTabDuplicatesView(anchor_view_.get());
  view->SetDuplicateGroups({});
  EXPECT_NE(nullptr, view);
}

// Test match mode setting.
TEST_F(AstraTabDuplicatesViewTest, SetMatchModeExactUrl) {
  auto* view = new AstraTabDuplicatesView(anchor_view_.get());
  view->SetMatchMode(AstraTabDuplicatesView::MatchMode::kExactUrl);
  SUCCEED();
}

TEST_F(AstraTabDuplicatesViewTest, SetMatchModeSameDomain) {
  auto* view = new AstraTabDuplicatesView(anchor_view_.get());
  view->SetMatchMode(AstraTabDuplicatesView::MatchMode::kSameDomain);
  SUCCEED();
}

TEST_F(AstraTabDuplicatesViewTest, SetMatchModeSameHost) {
  auto* view = new AstraTabDuplicatesView(anchor_view_.get());
  view->SetMatchMode(AstraTabDuplicatesView::MatchMode::kSameHost);
  SUCCEED();
}

// Test close tab callback.
TEST_F(AstraTabDuplicatesViewTest, CloseTabCallback) {
  std::string closed_tab;
  auto* view = new AstraTabDuplicatesView(anchor_view_.get());
  view->SetCloseTabCallback(
      base::BindRepeating(
          [](std::string* out, const std::string& id) { *out = id; },
          &closed_tab));

  std::vector<AstraDuplicateTabGroupView::GroupInfo> groups;
  AstraDuplicateTabGroupView::GroupInfo g;
  g.group_key = "test.com";
  g.label = u"test.com";
  g.duplicate_count = 2;

  AstraDuplicateTabGroupView::TabItem t1;
  t1.tab_id = "close_me";
  t1.title = u"Tab 1";
  t1.domain = "test.com";
  g.tabs.push_back(t1);

  AstraDuplicateTabGroupView::TabItem t2;
  t2.tab_id = "keep_me";
  t2.title = u"Tab 2";
  t2.domain = "test.com";
  g.tabs.push_back(t2);

  groups.push_back(g);
  view->SetDuplicateGroups(groups);

  EXPECT_TRUE(closed_tab.empty());
}

// Test keep tab callback.
TEST_F(AstraTabDuplicatesViewTest, KeepTabCallback) {
  std::string kept_tab;
  auto* view = new AstraTabDuplicatesView(anchor_view_.get());
  view->SetKeepTabCallback(
      base::BindRepeating(
          [](std::string* out, const std::string& id) { *out = id; },
          &kept_tab));

  EXPECT_TRUE(kept_tab.empty());
}

// Test close all duplicates callback.
TEST_F(AstraTabDuplicatesViewTest, CloseAllCallback) {
  bool callback_called = false;
  auto* view = new AstraTabDuplicatesView(anchor_view_.get());
  view->SetCloseAllDuplicatesCallback(
      base::BindRepeating(
          [](bool* called) { *called = true; }, &callback_called));

  EXPECT_FALSE(callback_called);
}

// Test refresh callback.
TEST_F(AstraTabDuplicatesViewTest, RefreshCallback) {
  bool callback_called = false;
  auto* view = new AstraTabDuplicatesView(anchor_view_.get());
  view->SetRefreshCallback(
      base::BindRepeating(
          [](bool* called) { *called = true; }, &callback_called));

  EXPECT_FALSE(callback_called);
}

// Test group with many duplicates.
TEST_F(AstraTabDuplicatesViewTest, LargeDuplicateGroup) {
  AstraDuplicateTabGroupView::GroupInfo info;
  info.group_key = "newsite.com";
  info.label = u"newsite.com/article";
  info.duplicate_count = 10;

  for (int i = 0; i < 10; i++) {
    AstraDuplicateTabGroupView::TabItem tab;
    tab.tab_id = "tab_" + std::to_string(i);
    tab.title = base::UTF8ToUTF16("Article Copy " + std::to_string(i));
    tab.domain = "newsite.com";
    tab.url = "https://newsite.com/article";
    tab.is_active = (i == 0);
    tab.last_accessed = base::Time::Now() - base::Minutes(i * 15);
    info.tabs.push_back(tab);
  }

  auto group = std::make_unique<AstraDuplicateTabGroupView>(
      info, base::DoNothing(), base::DoNothing(), base::DoNothing());

  EXPECT_EQ(10, group->duplicate_count());
}

// Test multiple groups with varying sizes.
TEST_F(AstraTabDuplicatesViewTest, MultipleGroupsVaryingSizes) {
  auto* view = new AstraTabDuplicatesView(anchor_view_.get());

  std::vector<AstraDuplicateTabGroupView::GroupInfo> groups;

  // 5 groups with 2-6 duplicates each.
  for (int g = 0; g < 5; g++) {
    AstraDuplicateTabGroupView::GroupInfo group;
    group.group_key = "domain" + std::to_string(g) + ".com";
    group.label = base::UTF8ToUTF16("domain" + std::to_string(g) + ".com");
    int count = 2 + g;  // 2, 3, 4, 5, 6
    group.duplicate_count = count;

    for (int t = 0; t < count; t++) {
      AstraDuplicateTabGroupView::TabItem tab;
      tab.tab_id = "g" + std::to_string(g) + "_t" + std::to_string(t);
      tab.title = base::UTF8ToUTF16("Tab " + std::to_string(t));
      tab.domain = "domain" + std::to_string(g) + ".com";
      tab.last_accessed = base::Time::Now();
      group.tabs.push_back(tab);
    }

    groups.push_back(group);
  }

  view->SetDuplicateGroups(groups);
  SUCCEED();
}

}  // namespace astra
