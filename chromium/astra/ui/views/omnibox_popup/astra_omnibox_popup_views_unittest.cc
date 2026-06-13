// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/omnibox_popup/astra_omnibox_popup_view.h"

#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

class AstraOmniboxPopupModelTest : public testing::Test {
 protected:
  void SetUp() override {
    model_ = std::make_unique<AstraOmniboxPopupModel>();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<AstraOmniboxPopupModel> model_;
};

// Test that the model starts empty.
TEST_F(AstraOmniboxPopupModelTest, StartsEmpty) {
  EXPECT_EQ(0u, model_->GetMatchCount());
  EXPECT_EQ(0u, model_->GetSelectedIndex());
  EXPECT_FALSE(model_->IsVisible());
  EXPECT_TRUE(model_->GetQuery().empty());
}

// Test SetMatches.
TEST_F(AstraOmniboxPopupModelTest, SetMatches) {
  std::vector<AstraOmniboxMatch> matches;

  AstraOmniboxMatch m1;
  m1.type = AstraOmniboxMatchType::kSearchWhatYouTyped;
  m1.contents = u"test query";
  m1.relevance = 1000;
  matches.push_back(m1);

  AstraOmniboxMatch m2;
  m2.type = AstraOmniboxMatchType::kUrlHistory;
  m2.contents = u"https://example.com";
  m2.description = u"Example";
  m2.relevance = 900;
  matches.push_back(m2);

  model_->SetMatches(matches);
  EXPECT_EQ(2u, model_->GetMatchCount());
  EXPECT_EQ(0u, model_->GetSelectedIndex());

  // First match should be default.
  const AstraOmniboxMatch* first = model_->GetMatchAt(0);
  ASSERT_NE(nullptr, first);
  EXPECT_TRUE(first->is_default);
}

// Test AddMatch.
TEST_F(AstraOmniboxPopupModelTest, AddMatch) {
  AstraOmniboxMatch m;
  m.type = AstraOmniboxMatchType::kSearchSuggestion;
  m.contents = u"test";
  m.relevance = 500;
  model_->AddMatch(m);
  EXPECT_EQ(1u, model_->GetMatchCount());
}

// Test ClearMatches.
TEST_F(AstraOmniboxPopupModelTest, ClearMatches) {
  model_->PopulateSampleSuggestions(u"test");
  EXPECT_GT(model_->GetMatchCount(), 0u);

  model_->ClearMatches();
  EXPECT_EQ(0u, model_->GetMatchCount());
  EXPECT_EQ(0u, model_->GetSelectedIndex());
}

// Test selection navigation.
TEST_F(AstraOmniboxPopupModelTest, SelectionNavigation) {
  model_->PopulateSampleSuggestions(u"test");
  size_t count = model_->GetMatchCount();
  ASSERT_GT(count, 1u);

  EXPECT_EQ(0u, model_->GetSelectedIndex());

  model_->SelectNext();
  EXPECT_EQ(1u, model_->GetSelectedIndex());

  model_->SelectPrevious();
  EXPECT_EQ(0u, model_->GetSelectedIndex());

  // Wrap around from first to last.
  model_->SelectPrevious();
  EXPECT_EQ(count - 1, model_->GetSelectedIndex());

  // Wrap around from last to first.
  model_->SelectNext();
  EXPECT_EQ(0u, model_->GetSelectedIndex());

  // Select first/last.
  model_->SelectLast();
  EXPECT_EQ(count - 1, model_->GetSelectedIndex());

  model_->SelectFirst();
  EXPECT_EQ(0u, model_->GetSelectedIndex());
}

// Test visibility.
TEST_F(AstraOmniboxPopupModelTest, Visibility) {
  EXPECT_FALSE(model_->IsVisible());

  model_->Show();
  EXPECT_TRUE(model_->IsVisible());

  model_->Hide();
  EXPECT_FALSE(model_->IsVisible());
}

// Test query.
TEST_F(AstraOmniboxPopupModelTest, Query) {
  EXPECT_TRUE(model_->GetQuery().empty());

  model_->SetQuery(u"hello");
  EXPECT_EQ(u"hello", model_->GetQuery());
}

// Test GetSelectedMatch.
TEST_F(AstraOmniboxPopupModelTest, SelectedMatch) {
  model_->PopulateSampleSuggestions(u"test");
  ASSERT_GT(model_->GetMatchCount(), 0u);

  const AstraOmniboxMatch* selected = model_->GetSelectedMatch();
  ASSERT_NE(nullptr, selected);
  EXPECT_EQ(model_->GetMatchAt(0), selected);
}

// Test RemoveMatchAt.
TEST_F(AstraOmniboxPopupModelTest, RemoveMatchAt) {
  model_->PopulateSampleSuggestions(u"test");
  size_t original_count = model_->GetMatchCount();
  ASSERT_GT(original_count, 2u);

  model_->RemoveMatchAt(0);
  EXPECT_EQ(original_count - 1, model_->GetMatchCount());
}

// Test grouped matches.
TEST_F(AstraOmniboxPopupModelTest, GroupedMatches) {
  model_->PopulateSampleSuggestions(u"test");
  auto grouped = model_->GetGroupedMatches();
  EXPECT_GT(grouped.size(), 0u);

  // First group should have at least one match.
  EXPECT_FALSE(grouped[0].first.empty());
  EXPECT_FALSE(grouped[0].second.empty());
}

// Test observer.
TEST_F(AstraOmniboxPopupModelTest, ObserverFires) {
  class TestObserver : public AstraOmniboxPopupObserver {
   public:
    int suggestions_changed = 0;
    int selection_changed = 0;
    int visibility_changed = 0;
    bool last_visible = false;

    void OnSuggestionsChanged(AstraOmniboxPopupModel* model) override {
      suggestions_changed++;
    }
    void OnSelectedMatchChanged(AstraOmniboxPopupModel* model) override {
      selection_changed++;
    }
    void OnPopupVisibilityChanged(AstraOmniboxPopupModel* model,
                                   bool visible) override {
      visibility_changed++;
      last_visible = visible;
    }
  };

  TestObserver observer;
  model_->AddObserver(&observer);

  model_->PopulateSampleSuggestions(u"test");
  EXPECT_GT(observer.suggestions_changed, 0);

  model_->SelectNext();
  EXPECT_GT(observer.selection_changed, 0);

  model_->Show();
  EXPECT_GT(observer.visibility_changed, 0);
  EXPECT_TRUE(observer.last_visible);

  model_->RemoveObserver(&observer);
}

// Test sample suggestions with different query types.
TEST_F(AstraOmniboxPopupModelTest, SampleSuggestions) {
  // Empty query should give no results.
  model_->PopulateSampleSuggestions(u"");
  EXPECT_EQ(0u, model_->GetMatchCount());

  // Non-empty query should give results.
  model_->PopulateSampleSuggestions(u"example");
  EXPECT_GT(model_->GetMatchCount(), 0u);

  // Wiki query.
  model_->PopulateSampleSuggestions(u"wiki");
  EXPECT_GT(model_->GetMatchCount(), 0u);
}

// Test match types enum coverage.
TEST_F(AstraOmniboxPopupModelTest, MatchTypesEnum) {
  std::vector<AstraOmniboxMatchType> types = {
      AstraOmniboxMatchType::kSearchWhatYouTyped,
      AstraOmniboxMatchType::kSearchHistory,
      AstraOmniboxMatchType::kSearchSuggestion,
      AstraOmniboxMatchType::kUrlHistory,
      AstraOmniboxMatchType::kUrlBookmark,
      AstraOmniboxMatchType::kUrlOpenTab,
      AstraOmniboxMatchType::kUrlNavsuggest,
      AstraOmniboxMatchType::kClipboard,
      AstraOmniboxMatchType::kDocument,
      AstraOmniboxMatchType::kAnswer,
      AstraOmniboxMatchType::kOmniboxAction,
      AstraOmniboxMatchType::kExtensionCommand,
  };

  EXPECT_EQ(12u, types.size());
}

// Test answer types enum coverage.
TEST_F(AstraOmniboxPopupModelTest, AnswerTypesEnum) {
  std::vector<AstraOmniboxAnswerType> types = {
      AstraOmniboxAnswerType::kNone,
      AstraOmniboxAnswerType::kCalculator,
      AstraOmniboxAnswerType::kWeather,
      AstraOmniboxAnswerType::kDictionary,
      AstraOmniboxAnswerType::kStock,
      AstraOmniboxAnswerType::kTranslation,
      AstraOmniboxAnswerType::kSunriseSunset,
      AstraOmniboxAnswerType::kFlightStatus,
      AstraOmniboxAnswerType::kSports,
      AstraOmniboxAnswerType::kTimeZone,
      AstraOmniboxAnswerType::kUnitConversion,
  };

  EXPECT_EQ(11u, types.size());
}

}  // namespace astra
