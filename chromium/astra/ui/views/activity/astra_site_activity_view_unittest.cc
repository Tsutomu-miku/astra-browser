// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/activity/astra_site_activity_view.h"

#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

// ===========================================================================
// AstraSiteActivityViewTest
// ===========================================================================

class AstraSiteActivityViewTest : public testing::Test {
 protected:
  void SetUp() override {
    anchor_view_ = std::make_unique<views::View>();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<views::View> anchor_view_;
};

// Test site activity row creation.
TEST_F(AstraSiteActivityViewTest, SiteRowCreation) {
  AstraSiteActivityRowView::SiteInfo info;
  info.domain = "example.com";
  info.category = "work";
  info.time_spent = base::Minutes(45);
  info.visit_count = 12;

  auto row = std::make_unique<AstraSiteActivityRowView>(
      info, base::Hours(2));

  EXPECT_EQ("example.com", row->domain());
}

// Test site row update.
TEST_F(AstraSiteActivityViewTest, SiteRowUpdate) {
  AstraSiteActivityRowView::SiteInfo info;
  info.domain = "test.com";
  info.category = "news";
  info.time_spent = base::Minutes(10);
  info.visit_count = 3;

  auto row = std::make_unique<AstraSiteActivityRowView>(
      info, base::Hours(1));

  AstraSiteActivityRowView::SiteInfo updated;
  updated.domain = "test.com";
  updated.category = "news";
  updated.time_spent = base::Minutes(30);
  updated.visit_count = 8;

  row->Update(updated, base::Hours(1));
  // Should not crash.
}

// Test various categories.
TEST_F(AstraSiteActivityViewTest, VariousCategories) {
  std::vector<std::string> categories = {
      "work", "social", "entertainment", "news",
      "productivity", "shopping", "education", "unknown"};

  for (const auto& cat : categories) {
    AstraSiteActivityRowView::SiteInfo info;
    info.domain = cat + ".com";
    info.category = cat;
    info.time_spent = base::Minutes(30);
    info.visit_count = 5;

    auto row = std::make_unique<AstraSiteActivityRowView>(
        info, base::Hours(2));
    EXPECT_EQ(cat + ".com", row->domain());
  }
}

// Test site activity view creation.
TEST_F(AstraSiteActivityViewTest, ViewCreation) {
  auto* view = new AstraSiteActivityView(anchor_view_.get());
  EXPECT_NE(nullptr, view);
}

// Test setting top sites.
TEST_F(AstraSiteActivityViewTest, SetTopSites) {
  auto* view = new AstraSiteActivityView(anchor_view_.get());

  std::vector<AstraSiteActivityRowView::SiteInfo> sites;

  AstraSiteActivityRowView::SiteInfo s1;
  s1.domain = "docs.google.com";
  s1.category = "work";
  s1.time_spent = base::Hours(1) + base::Minutes(20);
  s1.visit_count = 8;
  sites.push_back(s1);

  AstraSiteActivityRowView::SiteInfo s2;
  s2.domain = "github.com";
  s2.category = "work";
  s2.time_spent = base::Minutes(45);
  s2.visit_count = 12;
  sites.push_back(s2);

  AstraSiteActivityRowView::SiteInfo s3;
  s3.domain = "youtube.com";
  s3.category = "entertainment";
  s3.time_spent = base::Minutes(30);
  s3.visit_count = 5;
  sites.push_back(s3);

  view->SetTopSites(sites);
  // Should not crash.
}

// Test empty top sites list.
TEST_F(AstraSiteActivityViewTest, EmptyTopSites) {
  auto* view = new AstraSiteActivityView(anchor_view_.get());
  view->SetTopSites({});
  // Should not crash.
}

// Test setting categories.
TEST_F(AstraSiteActivityViewTest, SetCategories) {
  auto* view = new AstraSiteActivityView(anchor_view_.get());

  std::vector<AstraSiteActivityView::Category> categories = {
      {"work", base::Hours(3) + base::Minutes(45)},
      {"focus", base::Hours(1) + base::Minutes(30)},
      {"news", base::Minutes(45)},
      {"entertainment", base::Minutes(12)},
  };

  view->SetCategories(categories);
  // Should not crash.
}

// Test empty categories list.
TEST_F(AstraSiteActivityViewTest, EmptyCategories) {
  auto* view = new AstraSiteActivityView(anchor_view_.get());
  view->SetCategories({});
  // Should not crash with empty categories.
}

// Test total time.
TEST_F(AstraSiteActivityViewTest, TotalTime) {
  auto* view = new AstraSiteActivityView(anchor_view_.get());

  view->SetTotalTime(base::Hours(6) + base::Minutes(12));
  // Should not crash.
}

// Test zero total time.
TEST_F(AstraSiteActivityViewTest, ZeroTotalTime) {
  auto* view = new AstraSiteActivityView(anchor_view_.get());
  view->SetTotalTime(base::TimeDelta());
  // Should handle zero time gracefully.
}

// Test time range changes.
TEST_F(AstraSiteActivityViewTest, TimeRangeChanges) {
  auto* view = new AstraSiteActivityView(anchor_view_.get());

  view->SetTimeRange(AstraSiteActivityView::TimeRange::kDay);
  view->SetTimeRange(AstraSiteActivityView::TimeRange::kWeek);
  view->SetTimeRange(AstraSiteActivityView::TimeRange::kMonth);
  // Should not crash.
}

// Test time range callback.
TEST_F(AstraSiteActivityViewTest, TimeRangeCallback) {
  auto* view = new AstraSiteActivityView(anchor_view_.get());

  bool triggered = false;
  AstraSiteActivityView::TimeRange last_range =
      AstraSiteActivityView::TimeRange::kDay;

  view->SetTimeRangeChangedCallback(
      base::BindRepeating(
          [](bool* t, AstraSiteActivityView::TimeRange* last,
             AstraSiteActivityView::TimeRange range) {
            *t = true;
            *last = range;
          },
          &triggered, &last_range));
}

// Test window title.
TEST_F(AstraSiteActivityViewTest, WindowTitle) {
  auto* view = new AstraSiteActivityView(anchor_view_.get());
  EXPECT_EQ(u"Site Activity", view->GetWindowTitle());
}

// Test theme change doesn't crash.
TEST_F(AstraSiteActivityViewTest, ThemeChange) {
  auto* view = new AstraSiteActivityView(anchor_view_.get());
  view->OnThemeChanged();
}

// Test theme change on site row.
TEST_F(AstraSiteActivityViewTest, SiteRowThemeChange) {
  AstraSiteActivityRowView::SiteInfo info;
  info.domain = "theme.test";
  info.category = "work";
  info.time_spent = base::Minutes(30);
  info.visit_count = 5;

  auto row = std::make_unique<AstraSiteActivityRowView>(
      info, base::Hours(1));
  row->OnThemeChanged();
}

// Test format duration helper.
TEST_F(AstraSiteActivityViewTest, FormatDuration) {
  // Seconds only.
  EXPECT_EQ(u"30s",
            AstraSiteActivityView::FormatDuration(base::Seconds(30)));

  // Minutes only.
  EXPECT_EQ(u"5m",
            AstraSiteActivityView::FormatDuration(base::Minutes(5)));

  // Hours and minutes.
  EXPECT_EQ(u"2h 30m",
            AstraSiteActivityView::FormatDuration(
                base::Hours(2) + base::Minutes(30)));

  // Hours only.
  EXPECT_EQ(u"3h",
            AstraSiteActivityView::FormatDuration(base::Hours(3)));

  // Zero.
  EXPECT_EQ(u"0m",
            AstraSiteActivityView::FormatDuration(base::TimeDelta()));
}

// Test category emoji helper.
TEST_F(AstraSiteActivityViewTest, CategoryEmoji) {
  EXPECT_EQ("💼", AstraSiteActivityView::CategoryEmoji("work"));
  EXPECT_EQ("💬", AstraSiteActivityView::CategoryEmoji("social"));
  EXPECT_EQ("🎮", AstraSiteActivityView::CategoryEmoji("entertainment"));
  EXPECT_EQ("📰", AstraSiteActivityView::CategoryEmoji("news"));
  EXPECT_EQ("⚡", AstraSiteActivityView::CategoryEmoji("productivity"));
  EXPECT_EQ("🛒", AstraSiteActivityView::CategoryEmoji("shopping"));
  EXPECT_EQ("📚", AstraSiteActivityView::CategoryEmoji("education"));
  EXPECT_EQ("🎯", AstraSiteActivityView::CategoryEmoji("focus"));
  EXPECT_EQ("📄", AstraSiteActivityView::CategoryEmoji("unknown"));
  EXPECT_EQ("📄", AstraSiteActivityView::CategoryEmoji(""));
}

// Test many sites (scrollable).
TEST_F(AstraSiteActivityViewTest, ManySites) {
  auto* view = new AstraSiteActivityView(anchor_view_.get());

  std::vector<AstraSiteActivityRowView::SiteInfo> sites;
  for (int i = 0; i < 15; ++i) {
    AstraSiteActivityRowView::SiteInfo s;
    s.domain = "site" + std::to_string(i) + ".com";
    s.category = (i % 3 == 0) ? "work" :
                 (i % 3 == 1) ? "news" : "entertainment";
    s.time_spent = base::Minutes(5 + i * 3);
    s.visit_count = i + 1;
    sites.push_back(s);
  }

  view->SetTopSites(sites);
  // Should handle many sites with scrolling.
}

}  // namespace astra
