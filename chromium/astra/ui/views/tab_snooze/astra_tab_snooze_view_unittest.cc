// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/tab_snooze/astra_tab_snooze_view.h"

#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

// ===========================================================================
// AstraTabSnoozeViewTest
// ===========================================================================

class AstraTabSnoozeViewTest : public testing::Test {
 protected:
  void SetUp() override {
    anchor_view_ = std::make_unique<views::View>();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<views::View> anchor_view_;
};

// Test snoozed tab item creation.
TEST_F(AstraTabSnoozeViewTest, SnoozedItemCreation) {
  AstraSnoozedTabItemView::SnoozedTab tab;
  tab.tab_id = "snoozed_1";
  tab.title = u"Example Page";
  tab.domain = "example.com";
  tab.url = "https://example.com/page";
  tab.snoozed_at = base::Time::Now() - base::Hours(1);
  tab.wake_at = base::Time::Now() + base::Hours(2);

  auto item = std::make_unique<AstraSnoozedTabItemView>(
      tab, base::DoNothing(), base::DoNothing(), base::DoNothing());

  EXPECT_EQ("snoozed_1", item->tab_id());
}

// Test snoozed tab that's ready now.
TEST_F(AstraTabSnoozeViewTest, SnoozedTabReadyNow) {
  AstraSnoozedTabItemView::SnoozedTab tab;
  tab.tab_id = "ready_now";
  tab.title = u"Ready Tab";
  tab.domain = "ready.com";
  tab.snoozed_at = base::Time::Now() - base::Hours(5);
  tab.wake_at = base::Time::Now() - base::Minutes(10);  // Past wake time.

  auto item = std::make_unique<AstraSnoozedTabItemView>(
      tab, base::DoNothing(), base::DoNothing(), base::DoNothing());

  EXPECT_EQ("ready_now", item->tab_id());
}

// Test snooze view in snooze tab mode.
TEST_F(AstraTabSnoozeViewTest, SnoozeTabModeCreation) {
  auto* view = new AstraTabSnoozeView(
      anchor_view_.get(), AstraTabSnoozeView::Mode::kSnoozeTab);
  EXPECT_NE(nullptr, view);
}

// Test snooze view in snoozed list mode.
TEST_F(AstraTabSnoozeViewTest, SnoozedListModeCreation) {
  auto* view = new AstraTabSnoozeView(
      anchor_view_.get(), AstraTabSnoozeView::Mode::kSnoozedList);
  EXPECT_NE(nullptr, view);
}

// Test setting snooze tab info.
TEST_F(AstraTabSnoozeViewTest, SetSnoozeTabInfo) {
  auto* view = new AstraTabSnoozeView(
      anchor_view_.get(), AstraTabSnoozeView::Mode::kSnoozeTab);

  view->SetSnoozeTabTitle(u"Interesting Article");
  view->SetSnoozeTabDomain("news.example.com");
  // Should not crash.
}

// Test setting snooze presets.
TEST_F(AstraTabSnoozeViewTest, SetSnoozePresets) {
  auto* view = new AstraTabSnoozeView(
      anchor_view_.get(), AstraTabSnoozeView::Mode::kSnoozeTab);

  std::vector<AstraTabSnoozeView::SnoozePreset> presets = {
      {"later_today", u"Later today", "⏰", base::Hours(3)},
      {"tomorrow", u"Tomorrow", "🌅", base::Hours(24)},
      {"weekend", u"This weekend", "📅", base::Days(3)},
      {"custom", u"Custom...", "🎯", base::Minutes(0)},
  };

  view->SetSnoozePresets(presets);
  // Should not crash.
}

// Test snooze selected callback.
TEST_F(AstraTabSnoozeViewTest, SnoozeSelectedCallback) {
  auto* view = new AstraTabSnoozeView(
      anchor_view_.get(), AstraTabSnoozeView::Mode::kSnoozeTab);

  bool triggered = false;
  std::string preset_id;

  view->SetSnoozeSelectedCallback(
      base::BindRepeating(
          [](bool* t, std::string* id, const std::string& pid) {
            *t = true;
            *id = pid;
          },
          &triggered, &preset_id));
}

// Test setting snoozed tabs list.
TEST_F(AstraTabSnoozeViewTest, SetSnoozedTabs) {
  auto* view = new AstraTabSnoozeView(
      anchor_view_.get(), AstraTabSnoozeView::Mode::kSnoozedList);

  std::vector<AstraSnoozedTabItemView::SnoozedTab> tabs;

  AstraSnoozedTabItemView::SnoozedTab tab1;
  tab1.tab_id = "tab1";
  tab1.title = u"Article One";
  tab1.domain = "news.com";
  tab1.wake_at = base::Time::Now() + base::Hours(2);
  tabs.push_back(tab1);

  AstraSnoozedTabItemView::SnoozedTab tab2;
  tab2.tab_id = "tab2";
  tab2.title = u"Research";
  tab2.domain = "research.org";
  tab2.wake_at = base::Time::Now() + base::Days(1);
  tabs.push_back(tab2);

  AstraSnoozedTabItemView::SnoozedTab tab3;
  tab3.tab_id = "tab3";
  tab3.title = u"Shopping List";
  tab3.domain = "shop.com";
  tab3.wake_at = base::Time::Now() + base::Days(3);
  tabs.push_back(tab3);

  view->SetSnoozedTabs(tabs);
  // Should not crash.
}

// Test empty snoozed tabs list.
TEST_F(AstraTabSnoozeViewTest, EmptySnoozedTabs) {
  auto* view = new AstraTabSnoozeView(
      anchor_view_.get(), AstraTabSnoozeView::Mode::kSnoozedList);
  view->SetSnoozedTabs({});
  // Should not crash.
}

// Test snoozed count.
TEST_F(AstraTabSnoozeViewTest, SnoozedCount) {
  auto* view = new AstraTabSnoozeView(
      anchor_view_.get(), AstraTabSnoozeView::Mode::kSnoozedList);

  view->SetSnoozedCount(5);
  view->SetSnoozedCount(0);
  // Should not crash.
}

// Test wake tab callback.
TEST_F(AstraTabSnoozeViewTest, WakeTabCallback) {
  auto* view = new AstraTabSnoozeView(
      anchor_view_.get(), AstraTabSnoozeView::Mode::kSnoozedList);

  bool triggered = false;
  std::string woken_id;

  view->SetWakeTabCallback(
      base::BindRepeating(
          [](bool* t, std::string* id, const std::string& tid) {
            *t = true;
            *id = tid;
          },
          &triggered, &woken_id));
}

// Test edit snooze callback.
TEST_F(AstraTabSnoozeViewTest, EditSnoozeCallback) {
  auto* view = new AstraTabSnoozeView(
      anchor_view_.get(), AstraTabSnoozeView::Mode::kSnoozedList);

  bool triggered = false;
  std::string edited_id;

  view->SetEditSnoozeCallback(
      base::BindRepeating(
          [](bool* t, std::string* id, const std::string& tid) {
            *t = true;
            *id = tid;
          },
          &triggered, &edited_id));
}

// Test dismiss snooze callback.
TEST_F(AstraTabSnoozeViewTest, DismissSnoozeCallback) {
  auto* view = new AstraTabSnoozeView(
      anchor_view_.get(), AstraTabSnoozeView::Mode::kSnoozedList);

  bool triggered = false;
  std::string dismissed_id;

  view->SetDismissSnoozeCallback(
      base::BindRepeating(
          [](bool* t, std::string* id, const std::string& tid) {
            *t = true;
            *id = tid;
          },
          &triggered, &dismissed_id));
}

// Test window title for snooze tab mode.
TEST_F(AstraTabSnoozeViewTest, WindowTitleSnoozeMode) {
  auto* view = new AstraTabSnoozeView(
      anchor_view_.get(), AstraTabSnoozeView::Mode::kSnoozeTab);
  EXPECT_EQ(u"Snooze Tab", view->GetWindowTitle());
}

// Test window title for snoozed list mode.
TEST_F(AstraTabSnoozeViewTest, WindowTitleListMode) {
  auto* view = new AstraTabSnoozeView(
      anchor_view_.get(), AstraTabSnoozeView::Mode::kSnoozedList);
  EXPECT_EQ(u"Snoozed Tabs", view->GetWindowTitle());
}

// Test theme change doesn't crash (snooze tab mode).
TEST_F(AstraTabSnoozeViewTest, ThemeChangeSnoozeMode) {
  auto* view = new AstraTabSnoozeView(
      anchor_view_.get(), AstraTabSnoozeView::Mode::kSnoozeTab);
  view->OnThemeChanged();
}

// Test theme change doesn't crash (snoozed list mode).
TEST_F(AstraTabSnoozeViewTest, ThemeChangeListMode) {
  auto* view = new AstraTabSnoozeView(
      anchor_view_.get(), AstraTabSnoozeView::Mode::kSnoozedList);
  view->OnThemeChanged();
}

// Test theme change on snoozed tab item.
TEST_F(AstraTabSnoozeViewTest, ItemThemeChange) {
  AstraSnoozedTabItemView::SnoozedTab tab;
  tab.tab_id = "theme_test";
  tab.title = u"Theme Test";
  tab.domain = "theme.test";
  tab.wake_at = base::Time::Now() + base::Hours(1);

  auto item = std::make_unique<AstraSnoozedTabItemView>(
      tab, base::DoNothing(), base::DoNothing(), base::DoNothing());
  item->OnThemeChanged();
}

// Test many snoozed tabs.
TEST_F(AstraTabSnoozeViewTest, ManySnoozedTabs) {
  auto* view = new AstraTabSnoozeView(
      anchor_view_.get(), AstraTabSnoozeView::Mode::kSnoozedList);

  std::vector<AstraSnoozedTabItemView::SnoozedTab> tabs;
  for (int i = 0; i < 12; ++i) {
    AstraSnoozedTabItemView::SnoozedTab tab;
    tab.tab_id = "tab_" + std::to_string(i);
    tab.title = base::UTF8ToUTF16("Tab " + std::to_string(i));
    tab.domain = "site" + std::to_string(i) + ".com";
    tab.wake_at = base::Time::Now() + base::Hours(i + 1);
    tabs.push_back(tab);
  }

  view->SetSnoozedTabs(tabs);
  // Should handle many tabs with scrolling.
}

}  // namespace astra
