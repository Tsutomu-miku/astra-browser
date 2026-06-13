// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/reading_progress/astra_reading_progress_view.h"

#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

// ===========================================================================
// AstraReadingProgressViewTest
// ===========================================================================

class AstraReadingProgressViewTest : public testing::Test {
 protected:
  void SetUp() override {
    anchor_view_ = std::make_unique<views::View>();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<views::View> anchor_view_;
};

// Test article item creation.
TEST_F(AstraReadingProgressViewTest, ArticleItemCreation) {
  AstraReadingProgressItemView::ArticleInfo info;
  info.article_id = "article_001";
  info.title = u"The Future of AI";
  info.domain = u"techreview.com";
  info.progress_percent = 62;
  info.total_words = 5000;
  info.read_words = 3100;
  info.time_remaining = base::Minutes(4);
  info.last_read = base::Time::Now();

  auto item = std::make_unique<AstraReadingProgressItemView>(
      info, base::DoNothing(), base::DoNothing());

  EXPECT_EQ("article_001", item->article_id());
  EXPECT_EQ(62, item->progress_percent());
}

// Test article with zero progress.
TEST_F(AstraReadingProgressViewTest, ArticleZeroProgress) {
  AstraReadingProgressItemView::ArticleInfo info;
  info.article_id = "article_002";
  info.title = u"New Article";
  info.domain = u"example.com";
  info.progress_percent = 0;
  info.total_words = 2000;
  info.read_words = 0;
  info.time_remaining = base::Minutes(10);
  info.last_read = base::Time::Now();

  auto item = std::make_unique<AstraReadingProgressItemView>(
      info, base::DoNothing(), base::DoNothing());

  EXPECT_EQ(0, item->progress_percent());
}

// Test article with 100% progress.
TEST_F(AstraReadingProgressViewTest, ArticleFullProgress) {
  AstraReadingProgressItemView::ArticleInfo info;
  info.article_id = "article_003";
  info.title = u"Finished Article";
  info.domain = u"example.com";
  info.progress_percent = 100;
  info.total_words = 3000;
  info.read_words = 3000;
  info.time_remaining = base::Minutes(0);
  info.last_read = base::Time::Now() - base::Hours(2);

  auto item = std::make_unique<AstraReadingProgressItemView>(
      info, base::DoNothing(), base::DoNothing());

  EXPECT_EQ(100, item->progress_percent());
}

// Test SetProgress.
TEST_F(AstraReadingProgressViewTest, SetProgress) {
  AstraReadingProgressItemView::ArticleInfo info;
  info.article_id = "article_004";
  info.title = u"Progress Test";
  info.domain = u"example.com";
  info.progress_percent = 25;
  info.total_words = 4000;
  info.read_words = 1000;
  info.time_remaining = base::Minutes(15);
  info.last_read = base::Time::Now();

  auto item = std::make_unique<AstraReadingProgressItemView>(
      info, base::DoNothing(), base::DoNothing());

  EXPECT_EQ(25, item->progress_percent());
  item->SetProgress(50);
  EXPECT_EQ(50, item->progress_percent());
}

// Test progress clamping.
TEST_F(AstraReadingProgressViewTest, ProgressClamping) {
  AstraReadingProgressItemView::ArticleInfo info;
  info.article_id = "article_005";
  info.title = u"Clamp Test";
  info.domain = u"example.com";
  info.progress_percent = 50;
  info.time_remaining = base::Minutes(5);
  info.last_read = base::Time::Now();

  auto item = std::make_unique<AstraReadingProgressItemView>(
      info, base::DoNothing(), base::DoNothing());

  item->SetProgress(-10);
  EXPECT_GE(item->progress_percent(), 0);

  item->SetProgress(200);
  EXPECT_LE(item->progress_percent(), 100);
}

// Test reading progress view creation.
TEST_F(AstraReadingProgressViewTest, ViewCreation) {
  auto* view = new AstraReadingProgressView(anchor_view_.get());
  EXPECT_NE(nullptr, view);
}

// Test window title.
TEST_F(AstraReadingProgressViewTest, WindowTitle) {
  auto* view = new AstraReadingProgressView(anchor_view_.get());
  EXPECT_EQ(u"Reading Progress", view->GetWindowTitle());
}

// Test setting articles.
TEST_F(AstraReadingProgressViewTest, SetArticles) {
  auto* view = new AstraReadingProgressView(anchor_view_.get());

  std::vector<AstraReadingProgressItemView::ArticleInfo> articles;

  AstraReadingProgressItemView::ArticleInfo a1;
  a1.article_id = "a1";
  a1.title = u"Deep Learning Fundamentals";
  a1.domain = u"arxiv.org";
  a1.progress_percent = 28;
  a1.total_words = 8000;
  a1.read_words = 2240;
  a1.time_remaining = base::Minutes(12);
  a1.last_read = base::Time::Now();
  articles.push_back(a1);

  AstraReadingProgressItemView::ArticleInfo a2;
  a2.article_id = "a2";
  a2.title = u"The Future of AI";
  a2.domain = u"techreview.com";
  a2.progress_percent = 62;
  a2.total_words = 5000;
  a2.read_words = 3100;
  a2.time_remaining = base::Minutes(4);
  a2.last_read = base::Time::Now() - base::Hours(1);
  articles.push_back(a2);

  AstraReadingProgressItemView::ArticleInfo a3;
  a3.article_id = "a3";
  a3.title = u"Quantum Computing Explained";
  a3.domain = u"quantum-magazine.com";
  a3.progress_percent = 15;
  a3.total_words = 6000;
  a3.read_words = 900;
  a3.time_remaining = base::Minutes(25);
  a3.last_read = base::Time::Now() - base::Days(1);
  articles.push_back(a3);

  view->SetArticles(articles);
  EXPECT_NE(nullptr, view);
}

// Test empty articles list.
TEST_F(AstraReadingProgressViewTest, EmptyArticles) {
  auto* view = new AstraReadingProgressView(anchor_view_.get());
  view->SetArticles({});
  EXPECT_NE(nullptr, view);
}

// Test setting weekly stats.
TEST_F(AstraReadingProgressViewTest, SetWeeklyStats) {
  auto* view = new AstraReadingProgressView(anchor_view_.get());

  AstraReadingProgressView::WeeklyStats stats;
  stats.articles_read = 12;
  stats.total_read_time = base::Hours(3) + base::Minutes(24);
  stats.current_streak_days = 7;
  stats.longest_streak_days = 14;

  view->SetWeeklyStats(stats);
  EXPECT_NE(nullptr, view);
}

// Test zero stats.
TEST_F(AstraReadingProgressViewTest, ZeroStats) {
  auto* view = new AstraReadingProgressView(anchor_view_.get());

  AstraReadingProgressView::WeeklyStats stats;
  stats.articles_read = 0;
  stats.total_read_time = base::TimeDelta();
  stats.current_streak_days = 0;
  stats.longest_streak_days = 0;

  view->SetWeeklyStats(stats);
  SUCCEED();
}

// Test open article callback.
TEST_F(AstraReadingProgressViewTest, OpenArticleCallback) {
  std::string opened_id;
  auto* view = new AstraReadingProgressView(anchor_view_.get());
  view->SetOpenArticleCallback(
      base::BindRepeating(
          [](std::string* out_id, const std::string& id) {
            *out_id = id;
          },
          &opened_id));

  std::vector<AstraReadingProgressItemView::ArticleInfo> articles;
  AstraReadingProgressItemView::ArticleInfo a;
  a.article_id = "open_me";
  a.title = u"Test Article";
  a.domain = u"test.com";
  a.progress_percent = 50;
  a.time_remaining = base::Minutes(5);
  a.last_read = base::Time::Now();
  articles.push_back(a);
  view->SetArticles(articles);

  EXPECT_TRUE(opened_id.empty());
}

// Test remove article callback.
TEST_F(AstraReadingProgressViewTest, RemoveArticleCallback) {
  std::string removed_id;
  auto* view = new AstraReadingProgressView(anchor_view_.get());
  view->SetRemoveArticleCallback(
      base::BindRepeating(
          [](std::string* out_id, const std::string& id) {
            *out_id = id;
          },
          &removed_id));

  std::vector<AstraReadingProgressItemView::ArticleInfo> articles;
  AstraReadingProgressItemView::ArticleInfo a;
  a.article_id = "remove_me";
  a.title = u"Removable Article";
  a.domain = u"test.com";
  a.progress_percent = 20;
  a.time_remaining = base::Minutes(8);
  a.last_read = base::Time::Now();
  articles.push_back(a);
  view->SetArticles(articles);

  EXPECT_TRUE(removed_id.empty());
}

// Test view all callback.
TEST_F(AstraReadingProgressViewTest, ViewAllCallback) {
  bool callback_called = false;
  auto* view = new AstraReadingProgressView(anchor_view_.get());
  view->SetViewAllCallback(
      base::BindRepeating(
          [](bool* called) { *called = true; }, &callback_called));

  EXPECT_FALSE(callback_called);
}

// Test long article title (elide behavior).
TEST_F(AstraReadingProgressViewTest, LongArticleTitle) {
  AstraReadingProgressItemView::ArticleInfo info;
  info.article_id = "long_title";
  info.title = u"This is an extremely long article title that should be "
                u"elided in the display to fit within the bubble width";
  info.domain = u"very-long-domain-name.com";
  info.progress_percent = 30;
  info.time_remaining = base::Minutes(20);
  info.last_read = base::Time::Now();

  auto item = std::make_unique<AstraReadingProgressItemView>(
      info, base::DoNothing(), base::DoNothing());

  EXPECT_EQ("long_title", item->article_id());
}

// Test article with zero time remaining.
TEST_F(AstraReadingProgressViewTest, ZeroTimeRemaining) {
  AstraReadingProgressItemView::ArticleInfo info;
  info.article_id = "done";
  info.title = u"Finished Article";
  info.domain = u"example.com";
  info.progress_percent = 100;
  info.time_remaining = base::TimeDelta();
  info.last_read = base::Time::Now();

  auto item = std::make_unique<AstraReadingProgressItemView>(
      info, base::DoNothing(), base::DoNothing());

  EXPECT_EQ(100, item->progress_percent());
}

// Test article with hour-level time remaining.
TEST_F(AstraReadingProgressViewTest, HourTimeRemaining) {
  AstraReadingProgressItemView::ArticleInfo info;
  info.article_id = "long_read";
  info.title = u"Very Long Article";
  info.domain = u"longform.org";
  info.progress_percent = 10;
  info.time_remaining = base::Hours(2) + base::Minutes(15);
  info.last_read = base::Time::Now();

  auto item = std::make_unique<AstraReadingProgressItemView>(
      info, base::DoNothing(), base::DoNothing());

  EXPECT_EQ("long_read", item->article_id());
}

}  // namespace astra
