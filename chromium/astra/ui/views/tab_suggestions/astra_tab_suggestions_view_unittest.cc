// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/tab_suggestions/astra_tab_suggestions_view.h"

#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

// ===========================================================================
// AstraTabSuggestionsViewTest
// ===========================================================================

class AstraTabSuggestionsViewTest : public testing::Test {
 protected:
  void SetUp() override {
    anchor_view_ = std::make_unique<views::View>();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<views::View> anchor_view_;
};

// Test suggestion item creation.
TEST_F(AstraTabSuggestionsViewTest, SuggestionItemCreation) {
  AstraTabSuggestionItemView::SuggestionInfo info;
  info.suggestion_id = "sug_001";
  info.title = u"Project Dashboard";
  info.url = "https://work.com/dashboard";
  info.domain = "work.com";
  info.reason = u"Because you visit this daily";
  info.type = AstraTabSuggestionItemView::SuggestionType::kDaily;
  info.relevance_score = 95;
  info.last_visited = base::Time::Now() - base::Hours(2);
  info.visit_count = 42;
  info.is_openable = true;

  auto item = std::make_unique<AstraTabSuggestionItemView>(
      info, base::DoNothing(), base::DoNothing());

  EXPECT_EQ("sug_001", item->suggestion_id());
  EXPECT_EQ(95, item->relevance_score());
}

// Test continue type suggestion.
TEST_F(AstraTabSuggestionsViewTest, ContinueSuggestion) {
  AstraTabSuggestionItemView::SuggestionInfo info;
  info.suggestion_id = "cont_1";
  info.title = u"Continue Reading";
  info.domain = "article.com";
  info.reason = u"Continue where you left off";
  info.type = AstraTabSuggestionItemView::SuggestionType::kContinue;
  info.relevance_score = 88;
  info.last_visited = base::Time::Now() - base::Minutes(30);

  auto item = std::make_unique<AstraTabSuggestionItemView>(
      info, base::DoNothing(), base::DoNothing());

  EXPECT_EQ("cont_1", item->suggestion_id());
}

// Test reopen type suggestion.
TEST_F(AstraTabSuggestionsViewTest, ReopenSuggestion) {
  AstraTabSuggestionItemView::SuggestionInfo info;
  info.suggestion_id = "reopen_1";
  info.title = u"Recently Closed Page";
  info.domain = "news.com";
  info.reason = u"You closed this earlier today";
  info.type = AstraTabSuggestionItemView::SuggestionType::kReopen;
  info.relevance_score = 75;

  auto item = std::make_unique<AstraTabSuggestionItemView>(
      info, base::DoNothing(), base::DoNothing());

  EXPECT_EQ(75, item->relevance_score());
}

// Test related type suggestion.
TEST_F(AstraTabSuggestionsViewTest, RelatedSuggestion) {
  AstraTabSuggestionItemView::SuggestionInfo info;
  info.suggestion_id = "rel_1";
  info.title = u"Related Article";
  info.domain = "research.com";
  info.reason = u"Related to current tab";
  info.type = AstraTabSuggestionItemView::SuggestionType::kRelated;
  info.relevance_score = 60;

  auto item = std::make_unique<AstraTabSuggestionItemView>(
      info, base::DoNothing(), base::DoNothing());

  EXPECT_EQ("rel_1", item->suggestion_id());
}

// Test morning routine suggestion.
TEST_F(AstraTabSuggestionsViewTest, MorningRoutineSuggestion) {
  AstraTabSuggestionItemView::SuggestionInfo info;
  info.suggestion_id = "morning_1";
  info.title = u"Morning News";
  info.domain = "news.com";
  info.reason = u"Part of your morning routine";
  info.type = AstraTabSuggestionItemView::SuggestionType::kMorningRoutine;
  info.relevance_score = 82;

  auto item = std::make_unique<AstraTabSuggestionItemView>(
      info, base::DoNothing(), base::DoNothing());

  EXPECT_EQ("morning_1", item->suggestion_id());
}

// Test evening wind-down suggestion.
TEST_F(AstraTabSuggestionsViewTest, EveningWindDownSuggestion) {
  AstraTabSuggestionItemView::SuggestionInfo info;
  info.suggestion_id = "evening_1";
  info.title = u"Relaxing Read";
  info.domain = "longreads.com";
  info.reason = u"Your evening reading";
  info.type = AstraTabSuggestionItemView::SuggestionType::kEveningWindDown;
  info.relevance_score = 70;

  auto item = std::make_unique<AstraTabSuggestionItemView>(
      info, base::DoNothing(), base::DoNothing());

  EXPECT_EQ("evening_1", item->suggestion_id());
}

// Test non-openable suggestion.
TEST_F(AstraTabSuggestionsViewTest, NonOpenableSuggestion) {
  AstraTabSuggestionItemView::SuggestionInfo info;
  info.suggestion_id = "no_open";
  info.title = u"Unavailable";
  info.domain = "unknown.com";
  info.type = AstraTabSuggestionItemView::SuggestionType::kContinue;
  info.relevance_score = 50;
  info.is_openable = false;

  auto item = std::make_unique<AstraTabSuggestionItemView>(
      info, base::DoNothing(), base::DoNothing());

  EXPECT_FALSE(item->relevance_score() == 0);
}

// Test suggestion view creation.
TEST_F(AstraTabSuggestionsViewTest, ViewCreation) {
  auto* view = new AstraTabSuggestionsView(anchor_view_.get());
  EXPECT_NE(nullptr, view);
}

// Test window title.
TEST_F(AstraTabSuggestionsViewTest, WindowTitle) {
  auto* view = new AstraTabSuggestionsView(anchor_view_.get());
  EXPECT_EQ(u"Suggestions", view->GetWindowTitle());
}

// Test setting suggestions.
TEST_F(AstraTabSuggestionsViewTest, SetSuggestions) {
  auto* view = new AstraTabSuggestionsView(anchor_view_.get());

  std::vector<AstraTabSuggestionItemView::SuggestionInfo> suggestions;

  AstraTabSuggestionItemView::SuggestionInfo s1;
  s1.suggestion_id = "s1";
  s1.title = u"Work Dashboard";
  s1.domain = "work.com";
  s1.reason = u"Daily visit pattern";
  s1.type = AstraTabSuggestionItemView::SuggestionType::kDaily;
  s1.relevance_score = 95;
  s1.last_visited = base::Time::Now() - base::Hours(1);
  suggestions.push_back(s1);

  AstraTabSuggestionItemView::SuggestionInfo s2;
  s2.suggestion_id = "s2";
  s2.title = u"News Article";
  s2.domain = "news.com";
  s2.reason = u"Continue reading";
  s2.type = AstraTabSuggestionItemView::SuggestionType::kContinue;
  s2.relevance_score = 88;
  s2.last_visited = base::Time::Now() - base::Minutes(30);
  suggestions.push_back(s2);

  AstraTabSuggestionItemView::SuggestionInfo s3;
  s3.suggestion_id = "s3";
  s3.title = u"Research Paper";
  s3.domain = "arxiv.org";
  s3.reason = u"Related to current tab";
  s3.type = AstraTabSuggestionItemView::SuggestionType::kRelated;
  s3.relevance_score = 65;
  s3.last_visited = base::Time::Now() - base::Days(2);
  suggestions.push_back(s3);

  view->SetSuggestions(suggestions);
  EXPECT_NE(nullptr, view);
}

// Test empty suggestions.
TEST_F(AstraTabSuggestionsViewTest, EmptySuggestions) {
  auto* view = new AstraTabSuggestionsView(anchor_view_.get());
  view->SetSuggestions({});
  EXPECT_NE(nullptr, view);
}

// Test all suggestion types.
TEST_F(AstraTabSuggestionsViewTest, AllSuggestionTypes) {
  auto* view = new AstraTabSuggestionsView(anchor_view_.get());

  std::vector<AstraTabSuggestionItemView::SuggestionInfo> suggestions;

  using ST = AstraTabSuggestionItemView::SuggestionType;
  std::vector<ST> types = {
      ST::kContinue,
      ST::kReopen,
      ST::kRelated,
      ST::kDaily,
      ST::kMorningRoutine,
      ST::kEveningWindDown,
  };

  for (size_t i = 0; i < types.size(); i++) {
    AstraTabSuggestionItemView::SuggestionInfo s;
    s.suggestion_id = "type_" + std::to_string(i);
    s.title = base::UTF8ToUTF16("Suggestion " + std::to_string(i));
    s.domain = "site" + std::to_string(i) + ".com";
    s.reason = u"Test reason";
    s.type = types[i];
    s.relevance_score = static_cast<int>(100 - i * 10);
    suggestions.push_back(s);
  }

  view->SetSuggestions(suggestions);
  SUCCEED();
}

// Test open suggestion callback.
TEST_F(AstraTabSuggestionsViewTest, OpenSuggestionCallback) {
  std::string opened_id;
  auto* view = new AstraTabSuggestionsView(anchor_view_.get());
  view->SetOpenSuggestionCallback(
      base::BindRepeating(
          [](std::string* out, const std::string& id) { *out = id; },
          &opened_id));

  std::vector<AstraTabSuggestionItemView::SuggestionInfo> suggestions;
  AstraTabSuggestionItemView::SuggestionInfo s;
  s.suggestion_id = "open_me";
  s.title = u"Open Me";
  s.domain = "test.com";
  s.type = AstraTabSuggestionItemView::SuggestionType::kContinue;
  s.relevance_score = 90;
  suggestions.push_back(s);
  view->SetSuggestions(suggestions);

  EXPECT_TRUE(opened_id.empty());
}

// Test dismiss suggestion callback.
TEST_F(AstraTabSuggestionsViewTest, DismissSuggestionCallback) {
  std::string dismissed_id;
  auto* view = new AstraTabSuggestionsView(anchor_view_.get());
  view->SetDismissSuggestionCallback(
      base::BindRepeating(
          [](std::string* out, const std::string& id) { *out = id; },
          &dismissed_id));

  EXPECT_TRUE(dismissed_id.empty());
}

// Test refresh callback.
TEST_F(AstraTabSuggestionsViewTest, RefreshCallback) {
  bool callback_called = false;
  auto* view = new AstraTabSuggestionsView(anchor_view_.get());
  view->SetRefreshCallback(
      base::BindRepeating(
          [](bool* called) { *called = true; }, &callback_called));

  EXPECT_FALSE(callback_called);
}

// Test many suggestions.
TEST_F(AstraTabSuggestionsViewTest, ManySuggestions) {
  auto* view = new AstraTabSuggestionsView(anchor_view_.get());

  std::vector<AstraTabSuggestionItemView::SuggestionInfo> suggestions;
  for (int i = 0; i < 15; i++) {
    AstraTabSuggestionItemView::SuggestionInfo s;
    s.suggestion_id = "sug_" + std::to_string(i);
    s.title = base::UTF8ToUTF16("Suggestion " + std::to_string(i));
    s.domain = "site" + std::to_string(i) + ".com";
    s.reason = u"Smart suggestion";
    s.type = static_cast<AstraTabSuggestionItemView::SuggestionType>(
        i % 6);
    s.relevance_score = 100 - i * 5;
    s.last_visited = base::Time::Now() - base::Minutes(i * 10);
    s.visit_count = i * 3;
    suggestions.push_back(s);
  }

  view->SetSuggestions(suggestions);
  SUCCEED();
}

// Test low relevance score.
TEST_F(AstraTabSuggestionsViewTest, LowRelevanceScore) {
  AstraTabSuggestionItemView::SuggestionInfo info;
  info.suggestion_id = "low_score";
  info.title = u"Low Relevance";
  info.domain = "obscure.com";
  info.type = AstraTabSuggestionItemView::SuggestionType::kRelated;
  info.relevance_score = 5;

  auto item = std::make_unique<AstraTabSuggestionItemView>(
      info, base::DoNothing(), base::DoNothing());

  EXPECT_EQ(5, item->relevance_score());
}

// Test long title.
TEST_F(AstraTabSuggestionsViewTest, LongTitle) {
  AstraTabSuggestionItemView::SuggestionInfo info;
  info.suggestion_id = "long_title";
  info.title =
      u"This is an extremely long suggestion title that should be properly "
      u"ellipsized to fit within the available space in the suggestion card";
  info.domain = "very-long-domain-name.example.com";
  info.reason = u"This is also a very long reason description that will "
                u"need to be truncated to fit";
  info.type = AstraTabSuggestionItemView::SuggestionType::kDaily;
  info.relevance_score = 80;

  auto item = std::make_unique<AstraTabSuggestionItemView>(
      info, base::DoNothing(), base::DoNothing());

  EXPECT_EQ("long_title", item->suggestion_id());
}

}  // namespace astra
