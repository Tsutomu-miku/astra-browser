// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/tab_sleep/astra_tab_sleep_view.h"

#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

// ===========================================================================
// AstraTabSleepViewTest
// ===========================================================================

class AstraTabSleepViewTest : public testing::Test {
 protected:
  void SetUp() override {
    anchor_view_ = std::make_unique<views::View>();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<views::View> anchor_view_;
};

// Test sleeping tab item creation.
TEST_F(AstraTabSleepViewTest, SleepingItemCreation) {
  AstraSleepingTabItemView::TabInfo info;
  info.tab_id = "tab_123";
  info.title = u"Example Page";
  info.domain = "example.com";
  info.sleep_time = base::Time::Now() - base::Hours(3);
  info.memory_saved_bytes = 15LL * 1024 * 1024;

  auto item = std::make_unique<AstraSleepingTabItemView>(
      info, base::DoNothing(), base::DoNothing());

  EXPECT_EQ("tab_123", item->tab_id());
}

// Test tab sleep view creation.
TEST_F(AstraTabSleepViewTest, ViewCreation) {
  auto* view = new AstraTabSleepView(anchor_view_.get());
  EXPECT_NE(nullptr, view);
}

// Test setting sleeping tabs.
TEST_F(AstraTabSleepViewTest, SetSleepingTabs) {
  auto* view = new AstraTabSleepView(anchor_view_.get());

  std::vector<AstraSleepingTabItemView::TabInfo> tabs;

  AstraSleepingTabItemView::TabInfo tab1;
  tab1.tab_id = "tab1";
  tab1.title = u"Project Alpha";
  tab1.domain = "alpha.example.com";
  tab1.sleep_time = base::Time::Now() - base::Hours(2);
  tab1.memory_saved_bytes = 20LL * 1024 * 1024;
  tabs.push_back(tab1);

  AstraSleepingTabItemView::TabInfo tab2;
  tab2.tab_id = "tab2";
  tab2.title = u"Research Notes";
  tab2.domain = "research.org";
  tab2.sleep_time = base::Time::Now() - base::Days(1);
  tab2.memory_saved_bytes = 12LL * 1024 * 1024;
  tabs.push_back(tab2);

  view->SetSleepingTabs(tabs);
  // Should not crash.
}

// Test empty sleeping tabs list.
TEST_F(AstraTabSleepViewTest, EmptySleepingTabs) {
  auto* view = new AstraTabSleepView(anchor_view_.get());
  view->SetSleepingTabs({});
  // Should not crash with empty list.
}

// Test auto-sleep settings.
TEST_F(AstraTabSleepViewTest, AutoSleepSettings) {
  auto* view = new AstraTabSleepView(anchor_view_.get());

  view->SetAutoSleepEnabled(true);
  view->SetAutoSleepMinutes(30);
  view->SetAutoSleepMinutes(60);
  view->SetAutoSleepEnabled(false);
  // Should not crash.
}

// Test sleep indicator setting.
TEST_F(AstraTabSleepViewTest, ShowSleepIndicator) {
  auto* view = new AstraTabSleepView(anchor_view_.get());

  view->SetShowSleepIndicator(true);
  view->SetShowSleepIndicator(false);
  // Should not crash.
}

// Test stats updates.
TEST_F(AstraTabSleepViewTest, StatsUpdates) {
  auto* view = new AstraTabSleepView(anchor_view_.get());

  view->SetTotalMemorySaved(256LL * 1024 * 1024);
  view->SetSleepingTabCount(5);
  view->SetSleepingTabCount(0);
  view->SetTotalMemorySaved(0);
  // Should not crash.
}

// Test wake tab callback.
TEST_F(AstraTabSleepViewTest, WakeTabCallback) {
  auto* view = new AstraTabSleepView(anchor_view_.get());

  bool wake_triggered = false;
  std::string woken_tab_id;

  view->SetWakeTabCallback(
      base::BindRepeating(
          [](bool* triggered, std::string* id,
             const std::string& tab_id) {
            *triggered = true;
            *id = tab_id;
          },
          &wake_triggered, &woken_tab_id));

  // Callback can be set without crashing.
}

// Test close tab callback.
TEST_F(AstraTabSleepViewTest, CloseTabCallback) {
  auto* view = new AstraTabSleepView(anchor_view_.get());

  bool close_triggered = false;
  std::string closed_tab_id;

  view->SetCloseTabCallback(
      base::BindRepeating(
          [](bool* triggered, std::string* id,
             const std::string& tab_id) {
            *triggered = true;
            *id = tab_id;
          },
          &close_triggered, &closed_tab_id));
}

// Test wake all callback.
TEST_F(AstraTabSleepViewTest, WakeAllCallback) {
  auto* view = new AstraTabSleepView(anchor_view_.get());

  bool wake_all_triggered = false;

  view->SetWakeAllCallback(
      base::BindRepeating(
          [](bool* triggered) { *triggered = true; },
          &wake_all_triggered));
}

// Test sleep all callback.
TEST_F(AstraTabSleepViewTest, SleepAllCallback) {
  auto* view = new AstraTabSleepView(anchor_view_.get());

  bool sleep_all_triggered = false;

  view->SetSleepAllInactiveCallback(
      base::BindRepeating(
          [](bool* triggered) { *triggered = true; },
          &sleep_all_triggered));
}

// Test window title.
TEST_F(AstraTabSleepViewTest, WindowTitle) {
  auto* view = new AstraTabSleepView(anchor_view_.get());
  EXPECT_EQ(u"Sleeping Tabs", view->GetWindowTitle());
}

// Test theme change doesn't crash.
TEST_F(AstraTabSleepViewTest, ThemeChange) {
  auto* view = new AstraTabSleepView(anchor_view_.get());
  view->OnThemeChanged();
}

// Test sleeping tab item with null sleep time.
TEST_F(AstraTabSleepViewTest, ItemWithNullSleepTime) {
  AstraSleepingTabItemView::TabInfo info;
  info.tab_id = "tab_null";
  info.title = u"Null Time Tab";
  info.domain = "null.test";
  info.sleep_time = base::Time();
  info.memory_saved_bytes = 0;

  auto item = std::make_unique<AstraSleepingTabItemView>(
      info, base::DoNothing(), base::DoNothing());

  EXPECT_EQ("tab_null", item->tab_id());
}

// Test multiple tabs with various memory sizes.
TEST_F(AstraTabSleepViewTest, VariousMemorySizes) {
  auto* view = new AstraTabSleepView(anchor_view_.get());

  std::vector<AstraSleepingTabItemView::TabInfo> tabs;

  // Small tab (KB range).
  AstraSleepingTabItemView::TabInfo small;
  small.tab_id = "small";
  small.title = u"Small Tab";
  small.domain = "small.com";
  small.sleep_time = base::Time::Now() - base::Minutes(10);
  small.memory_saved_bytes = 512;  // 512 bytes
  tabs.push_back(small);

  // Medium tab (MB range).
  AstraSleepingTabItemView::TabInfo medium;
  medium.tab_id = "medium";
  medium.title = u"Medium Tab";
  medium.domain = "medium.com";
  medium.sleep_time = base::Time::Now() - base::Hours(5);
  medium.memory_saved_bytes = 25LL * 1024 * 1024;  // 25 MB
  tabs.push_back(medium);

  // Large tab (GB range).
  AstraSleepingTabItemView::TabInfo large;
  large.tab_id = "large";
  large.title = u"Large Tab";
  large.domain = "large.com";
  large.sleep_time = base::Time::Now() - base::Days(3);
  large.memory_saved_bytes = 3LL * 1024 * 1024 * 1024;  // 3 GB
  tabs.push_back(large);

  view->SetSleepingTabs(tabs);
  // Should not crash with various memory sizes.
}

}  // namespace astra
